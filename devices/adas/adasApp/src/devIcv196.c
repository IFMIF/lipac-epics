/******************************************************************************
 * CEA - Direction des Sciences de la Matiere - IRFU/SIS
 * CE-SACLAY, 91191 Gif-sur-Yvette, France
 *
 * $Id: devIcv196.c 23 2013-03-13 15:38:34Z lussi $
 *
 * ADAS ICV 196 Device Support
 *
 * 	Author:	Yves Lussignol
 * 	Date:	27/01/95
 *
 * who       when       what
 * --------  --------   ----------------------------------------------
 * ylussign  05/03/94   created
 * ylussign  01/17/95	added interrupt service on 16 first input signals
 * ylussign  27/05/97	added event number in bi INP field
 * jhosselet 07/12/06 	updated for 3.14
 * ylussign  07/11/07   merged device and driver support
 *                      automatic configuration of direction register
 *                      added support for records mbbiDirect/mbboDirect
 *                      added support for records longin/longout
 *                      fixed optimisation problem with Z8536
 * ylussign  09/11/07   field DPVT used to mark bad records
 * ylussign  04/07/08   - use Device Support Library devLib
 *                      - added RTEMS support
 * ylussign  01/12/08   added delay after each write in Z8536 registers
 * ylussign  24/11/09   - added include errlog.h
 *                      - added doxygen documentation
 */

/** 
 * @file
 * @brief ADAS ICV196 Device Support for EPICS R3.14.
 * 
 * ICV196 Device Support accepts up to 2 boards in a VME crate, 
 * starting from address @b 0x200000 with an increment of 0x100.
 * Each ICV196 uses two interrupt vectors, starting from vector 0xC4 
 * for the board 0.
 * 
 * It supports the following record types: BI, BO, MBBI, MBBO, 
 * MBBIDIRECT, MBBODIRECT, LONGIN, LONGOUT. The device type @b DTYP 
 * is @b ICV196 for all record types.
 * 
 * Signals 0 to 15 must always be configured as input because
 * they may be programmed to generate interrupts on a zero to one
 * input transition. The interrupt is enabled by giving an event 
 * number in the parameter string of the BI record @b INP field 
 * (eg: \#C0 S1 \@event 12). The interrupt service routine posts the 
 * event to allow records processing.
 *
 * Signals 16 to 95 may be configured as input or output by groups
 * of 8 signals. The configuration is automatically done and checked 
 * by the record/device init functions.
 *
 * The @b NOBT of records MBBI, MBBO, MBBIDIRECT and MBBODIRECT is 
 * limited to 16 bits by the record support.
 *
 * Records LONGIN and LONGOUT allow to read or write 32 bit patterns.
 * Three patterns are available through signal number S0 to S2,
 * starting at signals 0, 32 and 64. The first pattern (S0) must
 * always be configured as input.
 */

#ifdef vxWorks
#include <vxWorks.h>
#include <vxLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <vme.h>
#include <types.h>
#include <stdioLib.h>
#endif

#ifdef __rtems__
#include <rtems.h>
#include <bsp/VME.h>
#include <bsp/bspExt.h>
#define OK     0
#define ERROR  (-1)
#define taskDelay(a) (rtems_task_wake_after(a));
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alarm.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <dbScan.h>
#include <recGbl.h>
#include <recSup.h>
#include <devSup.h>
#include <biRecord.h>
#include <boRecord.h>
#include <mbbiRecord.h>
#include <mbboRecord.h>
#include <mbbiDirectRecord.h>
#include <mbboDirectRecord.h>
#include <longinRecord.h>
#include <longoutRecord.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <devLib.h>
#include <errlog.h>

/* VME ICV196 defines */

#define ICV196_BASE  (char *)0x200000	/* VME base address */
#define ICV196_MAX_CARDS	    2	/* max. number of boards in a VME crate */
#define ICV196_MAX_CHANS	   96	/* number of IO signals */
#define IT_LEVEL                    2	/* Interrupt level */
#define IT_VECTOR                0xC4	/* Interrupt vector for board 0 port A */

/* z8536 controller registers */

#define MIC   0x00    /* Master Interrupt Control Register */
#define MCC   0x01    /* Master Configuration Control Register */
#define IVA   0x02    /* Port A Interrupt Vector Register */
#define IVB   0x03    /* Port B Interrupt Vector Register */
#define PCSA  0x08    /* Port A Command and Status Register */
#define PCSB  0x09    /* Port B Command and Status Register */
#define PMSA  0x20    /* Port A Mode Specification Register */
#define DPPA  0x22    /* Port A Data Path Polarity Register */
#define DDA   0x23    /* Port A Data Direction Register */
#define PPA   0x25    /* Port A Pattern Polarity Register */
#define PTA   0x26    /* Port A Pattern Transition Register */
#define PMA   0x27    /* Port A Pattern Mask Register */
#define PMSB  0x28    /* Port B Mode Specification Register */
#define DPPB  0x2A    /* Port B Data Path Polarity Register */
#define DDB   0x2B    /* Port B Data Direction Register */
#define PPB   0x2D    /* Port B Pattern Polarity Register */
#define PTB   0x2E    /* Port B Pattern Transition Register */
#define PMB   0x2F    /* Port B Pattern Mask Register */

/* 
 * icv196 memory structure (256 bytes) 
 */
struct dio_icv196 {
    unsigned short clear;		/* clear module */
    unsigned short ports[6];		/* six 16 bits values (96 signals) */
    unsigned short dir;			/* direction register (12 bits) */
    char pad1[0x80 - 16];
    char null1;
    unsigned char z8536_portC;		/* z8536 port C */
    char null2;
    unsigned char z8536_portB;		/* z8536 port B */
    char null3;
    unsigned char z8536_portA;		/* z8536 port A */
    char null4;
    unsigned char z8536_control;	/* z8536 control register */
    char pad2[0xc0 - 0x80 - 8];
    char null5;
    unsigned char nit;			/* interrupt level register */
    char pad3[0x100 - 0xC0 - 2];
};

static struct dio_icv196 *ppdio_icv196[ICV196_MAX_CARDS]; /* pointers to icv196 modules */
static short events[ICV196_MAX_CARDS][16];                /* interrupt/event translation */
static unsigned short dirs[ICV196_MAX_CARDS] = {0,0};     /* direction register */
static unsigned short mdirs[ICV196_MAX_CARDS] = {0,0};    /* direction register modified */

/**
 * This IOC shell variable allows to print debug messages.
 * Valid range is:
 * - 0 no message is printed
 * - 1 messages at initialization are printed
 * - 2 initialization and I/O messages are printed
 */
int devIcv196Verbose = 0;
epicsExportAddress(int, devIcv196Verbose);

#define devMapAddr(a,b,c,d,e) ((pdevLibVirtualOS->pDevMapAddr)(a,b,c,d,e))
#define DELAY {int i; for (i=0;i<10000;i++);}



/*__________________________________________________________________
 *
 * Driver support functions
 */

/*
 * config_dir - Configures the Direction Register
 *
 * This function is called by device/record init functions to 
 * configure the Direction Register as input (<direction>=0) or 
 * output for <nobt> bits, starting from <signal>.
 */

static int
config_dir (
	int card,
	int signal,
	int nobt,
	int direction
	)
{
    int gr1, gr2, ngr, dir;
    unsigned short mask;
    
    /* 
     * convert first signal number and number of bits
     * to direction mask
     */
    gr1 = signal / 8;
    gr2 = (signal + nobt - 1) / 8;
    ngr = gr2 - gr1 + 1;
    mask = (1 << ngr) - 1;
    mask <<= gr1;
    
    /* 
     * check configuration consistency
     */
    if (mdirs[card] & mask)
    {
	dir = (dirs[card] & mask) ? 1 : 0;
	if (dir != direction)
	{
	    errlogPrintf("devIcv196: config_dir: card %d signal %d inconsistent direction\n",
			 card, signal);
	    return ERROR;
	}
    }
    
    /* 
     * change direction
     */
    mdirs[card] |= mask;
    if (direction)
    {
	dirs[card] |= mask;
    }

    if (devIcv196Verbose)
	printf ("devIcv196: config_dir: card %d mask=0x%04x direction=0x%03x\n",
		card, mask, dirs[card]);
    
    return OK;
}



/*
 * config_int - Set interrupt mode and event number for an input signal
 *
 * This function is called by init_bi_record() to enable interrupt for
 * an input signal (0-15) and to specify the associated event number.
 */

static int
config_int (
	int card,
	int signal,
	int event
	)
{
    int group, bit;
    volatile unsigned char *ctrl1, *ctrl2;
    unsigned char pt, pm, a, b;
    unsigned char mask;

    /* 
     * register event number 
     */
    events[card][signal] = event;

    /* 
     * convert signal number to 8 bit group number and bit mask 
     */
    group = signal / 8;
    bit = signal - (signal / 8) * 8;
    mask = 1 << bit;
     
    if (devIcv196Verbose)
	printf ("devIcv196: config_int: card %d signal %d IT group=%d mask=0x%02x\n",
		card, signal, group, mask);

    ctrl1 = &ppdio_icv196[card]->z8536_control;
    ctrl2 = &ppdio_icv196[card]->z8536_control;

    /* 
     * configure Z8536 controller
     */
    if ( group == 0 )  /* port A */
    {
	a = *ctrl2;
	*ctrl1 = PTA;		/* Pattern Transition Register */
	DELAY;
	pt = *ctrl2 | mask;
	*ctrl1 = PTA;
	DELAY;
	*ctrl2 = pt;		/* Zero to one transition */
	DELAY;

	*ctrl1 = PMA;		/* Pattern Mask Register */
	DELAY;
	pm = *ctrl2| mask;
	*ctrl1 = PMA;
	DELAY;
	*ctrl2 = pm;		/* Zero to one transition */
	DELAY;
	
	/* 
	 * verify
	 */
	*ctrl1 = PTA;
	DELAY;
	a = *ctrl2;
	*ctrl1 = PMA;
	DELAY;
	b = *ctrl2;
	if ((a != pt) || (b != pm))
	{
	    errlogPrintf ("devIcv196: init: error PTA=0x%02x PMA=0x%02x\n", a, b);
	    return ERROR;
	}
    } 
    else             /* port B */
    {
	*ctrl1 = PTB;		/* Pattern Transition Register */
	DELAY;
	pt = *ctrl2 | mask;
	*ctrl1 = PTB;
	DELAY;
	*ctrl2 = pt;		/* Zero to one transition */
	DELAY;

	*ctrl1 = PMB;		/* Pattern Mask Register */
	DELAY;
	pm = *ctrl2 | mask;
	*ctrl1 = PMB;
	DELAY;
	*ctrl2 = pm;		/* Zero to one transition */
	DELAY;
	
	/* 
	 * verify
	 */
	*ctrl1 = PTB;
	DELAY;
	a = *ctrl2;
	*ctrl1 = PMB;
	DELAY;
	b = *ctrl2;
	if ((a != pt) || (b != pm))
	{
	    errlogPrintf ("devIcv196: init: error PTB=0x%02x PMB=0x%02x\n",a,b);
	    return ERROR;
	}
    }
     
    return OK;
}



/*
 * read_bit - read a single bit
 */

static int
read_bit (
	int card,
	int signal,
	unsigned int *value
	)
{
    unsigned short mask;
    int port, bit;

    /* 
     * convert signal number to 16 bit port number and bit mask 
     */
    port = signal / 16;
    bit = signal % 16;
    mask = 1 << bit;

    /* 
     * read bit value 
     */
    *value = ppdio_icv196[card]->ports[port] & mask;

    if (devIcv196Verbose == 3)
	printf ("devIcv196: read_bit: card %d signal %d port=%d mask=0x%04x value=0x%04x\n", 
		card, signal, port, mask, *value);

    return OK;
}



/*
 * write_bit - write a single bit
 */

static int
write_bit (
	int card,
	int signal,
	unsigned int value
	)
{
    unsigned short mask;
    int port, bit;

    /* 
     * convert signal number to 16 bit port number and bit mask 
     */
    port = signal / 16;
    bit = signal % 16;
    mask = 1 << bit;

    /* 
     * write bit value 
     */
    if (value)
	ppdio_icv196[card]->ports[port] |= mask;
    else
	ppdio_icv196[card]->ports[port] &= ~mask;

    if (devIcv196Verbose == 3)
	printf ("devIcv196: write_bit: card %d signal %d port=%d mask=0x%04x value=0x%04x\n", 
		card, signal, port, mask, value);

     return OK;
}



/*
 * read_pattern - read a bit pattern
 */

static int
read_pattern (
	int card,
	int signal,
	unsigned int mask,
	unsigned int *value
	)
{
    int port;
    unsigned int work;

    /* 
     * convert lowest signal number to 16 bit port number
     */
    port = signal / 16;

    /*
     * read bit pattern 
     */
    if ( port < 5 )
	work = ((ppdio_icv196[card]->ports[port+1]) << 16) + ppdio_icv196[card]->ports[port];
    else
	work = ppdio_icv196[card]->ports[port];

    /*
     * mask record pattern 
     */
    *value = work & mask;

    if (devIcv196Verbose == 3)
	printf ("devIcv196: read_pattern: card %d signal %d port=%d mask=0x%08x value=0x%08x\n",
		card, signal, port, mask, *value);

    return OK;
}



/*
 * write_pattern - write a bit pattern
 */

static int
write_pattern (
	int card,
	int signal,
	unsigned int mask,
	unsigned int value
	)
{
    int port;
    unsigned int work;

    /* 
     * convert lowest signal number to 16 bit port number
     */
    port = signal / 16;

    /*
     * read bit pattern 
     */
    if ( port < 5 )
	work = ((ppdio_icv196[card]->ports[port+1]) << 16) + ppdio_icv196[card]->ports[port];
    else
	work = ppdio_icv196[card]->ports[port];

    /*
     * change record pattern 
     */
    work = (work & ~mask) | (value & mask);
    ppdio_icv196[card]->ports[port] = (unsigned short)(work & 0x0000ffff);   
    if ( port < 5 )
	ppdio_icv196[card]->ports[port+1] = (unsigned short)((work & 0xffff0000) >> 16);

    if (devIcv196Verbose == 3)
	printf ("devIcv196: write_pattern: card %d signal %d port=%d mask=0x%08x value=0x%08x\n",
		card, signal, port, mask, value);

    return OK;
}



/* 
 * Interrupt service routine
 *
 * Parameter cardPort contains the interrupting card and port number:
 *   0 = card 0, port A
 *   1 = card 0, port B
 *   2 = card 1, port A
 *   3 = card 1, port B
 */

static void 
int_service (
	int cardPort
	)
{
    unsigned char ch;
    volatile unsigned char *ctrl1, *ctrl2;
    int i, card, port;

    card = cardPort / 2;
    port = cardPort & 1;
    
    ctrl1 = &ppdio_icv196[card]->z8536_control;
    ctrl2 = &ppdio_icv196[card]->z8536_control;

    /*
     * read interrupting bit pattern 
     */
    ch = (port & 1) ? ppdio_icv196[card]->z8536_portB : ppdio_icv196[card]->z8536_portA;

    /*
     * find interrupting signal(s) and post associated event(s)
     */
    for (i = 0; i < 8; i++) 
    {
	if ((ch & 1) && (events[card][i+8*port])) 
	{
	    post_event (events[card][i+8*port]);
	}
	ch >>= 1;
    }

    /*
     * clear interrupt 
     */
    *ctrl1 = (port & 1) ? PCSB : PCSA;	/* Port A or B Command and Status Register */
    DELAY;
    *ctrl2 = 0x20;			/* clear IP and IUS */
    DELAY;
}



/*
 * Device initialization
 */

/* 
 * Z8536 Counter/Timer and Parallel I/O Unit
 *
 * All internal registers of the Z8536 are accessed by a two-step sequence
 * at the control address (A0=1,A1=1). First, write the address of the target
 * register to an internal Pointer Register; then read from or write to the
 * target register.
 *
 * An internal state machine determines if accesses are to the Pointer Register
 * or to an internal control register. Following any control read operation, the
 * state machine is in State 0 and the next control access is to the Pointer
 * Register. After a write to the Pointer Register the state machine is in State 1.
 * The next control access is to the internal register selected; then the state
 * machine returns to State 0. The state machine is in the Reset State after a
 * hardware reset or after writing a 1 to the reset bit in the Master Interrupt
 * Control Register (internal register #0). In this state all functions are
 * disabled except a write to the Reset bit.
 */
 
static long
init (
	int after
	)
{
    short dummy;
    int card;
    int i;
    unsigned char a, b, c, d;
    unsigned char it;
    volatile unsigned char *ctrl1, *ctrl2, *ctrl3, *ctrl4;
    struct dio_icv196 *pdio_icv196;
    unsigned int itVector;

    /*
     * before records init: initialize everything but direction register
     * after records init: initialize the direction register
     */
    if (after) {
	for (card = 0; card < ICV196_MAX_CARDS; card++) {
	    if (ppdio_icv196[card]) {
		ppdio_icv196[card]->dir = dirs[card];
	    }
	}
	if ( devIcv196Verbose )
	    printf ("\ndevIcv196: init: after done\n");
	return OK;
    }
    
    /* 
     * convert VME address A24/D16 to local address 
     */
    if (devMapAddr (atVMEA24,
		    0,
		   (size_t) ICV196_BASE,
		    0,
		   (volatile void **)&pdio_icv196) != OK)
    {
	errlogPrintf ("devIcv196: init: unable to map ICV196 base address\n");
	return ERROR;
    }

    /* 
     * determine which cards are present and initialize 
     */
    for (card = 0; card < ICV196_MAX_CARDS; card++, pdio_icv196++) {

	/* Do not read at address 0 (clear module) */
	if (devReadProbe (sizeof (short),
			 (volatile const void *)&pdio_icv196->ports,
			 (void *)&dummy) == OK)
	{
	    if (devIcv196Verbose)
		printf ("devIcv196: init: card %d present (0x%x)\n", 
			card, pdio_icv196);

	    ppdio_icv196[card] = pdio_icv196;
	    ctrl1 = &pdio_icv196->z8536_control;
	    ctrl2 = &pdio_icv196->z8536_control;
	    ctrl3 = &pdio_icv196->z8536_control;
	    ctrl4 = &pdio_icv196->z8536_control;

	    /* Interrupt Level Register */
	    pdio_icv196->nit = ~(1 << IT_LEVEL);

	    for (i = 0; i < 16; i++)
		events[card][i] = 0;

	    /* 
	     * reset the Z8536 
	     */
	    a = *ctrl1;
	    *ctrl2 = MIC;   /* select Master Interrupt Control Register */
	    DELAY;
	    *ctrl3 = 0x01;  /* set Reset bit to 1*/
	    DELAY;
	    *ctrl4 = 0x00;  /* set Reset bit to 0 */
	    DELAY;

	    /* 
	     * Port A
	     */
	    *ctrl1 = PMSA;  /* Port A Mode Specification Register */
	    DELAY;
	    *ctrl2 = 0x05;  /* Bit Port, OR mode, Latch on Pattern Match */
	    DELAY;

	    *ctrl1 = DPPA;  /* Port A Data Path Polarity Register */
	    DELAY;
	    *ctrl2 = 0x00;  /* non-inverting */
	    DELAY;

	    *ctrl1 = DDA;   /* Port A Data Direction Register */
	    DELAY;
	    *ctrl2 = 0xff;  /* 8 Input Bits */
	    DELAY;
	    
	    /* 
	     * verify
	     */
	    *ctrl1 = PMSA;
	    DELAY;
	    a = *ctrl2;
	    *ctrl1 = DPPA;
	    DELAY;
	    b = *ctrl2;
	    *ctrl1 = DDA;
	    DELAY;
	    c = *ctrl2;
	    if ((a != 0x05) || (b != 0x00) || (c != 0xff))
	    {
		errlogPrintf ("devIcv196: init: error PMSA=0x%02x DPPA=0x%02x DDA=0x%02x\n",a,b,c);
		return ERROR;
	    }

	    /* 
	     * Port B
	     */
	    *ctrl1 = PMSB;  /* Port B Mode Specification Register */
	    DELAY;
	    *ctrl2 = 0x04;  /* Bit Port, OR mode */
	    DELAY;

	    *ctrl1 = DPPB;  /* Port B Data Path Polarity Register */
	    DELAY;
	    *ctrl2 = 0x00;  /* non-inverting */
	    DELAY;

	    *ctrl1 = DDB;   /* Port B Data Direction Register */
	    DELAY;
	    *ctrl2 = 0xff;  /* 8 Input Bits */
	    DELAY;

	    /* 
	     * verify
	     */
	    *ctrl1 = PMSB;
	    DELAY;
	    a = *ctrl2;
	    *ctrl1 = DPPB;
	    DELAY;
	    b = *ctrl2;
	    *ctrl1 = DDB;
	    DELAY;
	    c = *ctrl2;
	    if ((a != 0x04) || (b != 0x00) || (c != 0xff))
	    {
		errlogPrintf ("devIcv196: init: error PMSB=0x%02x DPPB=0x%02x DDB=0x%02x\n",a,b,c);
		return ERROR;
	    }

	    /* 
	     * Pattern Definition Registers
	     *
	     * mask  transit  polarity  pattern specification
	     * 0        0        X      bit masked off
	     * 0        1        X      any transition
	     * 1        0        0      zero
	     * 1        0        1      one
	     * 1        1        0      one to zero transition
	     * 1        1        1      zero to one transition
	     */

	    /* 
	     * Port A interrupts
	     */
	    *ctrl1 = PPA;   /* Port A Pattern Polarity Register */
	    DELAY;
	    *ctrl2 = 0xff;
	    DELAY;
	    
	    *ctrl1 = PTA;   /* Port A Pattern Transition Register */
	    DELAY;
	    *ctrl2 = 0x00;  /* masked off */
	    DELAY;
	    
	    *ctrl1 = PMA;   /* Port A Pattern Mask Register */
	    DELAY;
	    *ctrl2 = 0x00;  /* masked off */
	    DELAY;

	    *ctrl1 = IVA;   /* Port A Interrupt Vector Register */
	    DELAY;
	    it = IT_VECTOR + 2*card;
	    *ctrl2 = it;  /* Interrupt Vector */
	    DELAY;

	    *ctrl1 = PCSA;  /* Port A Command and Status Register */
	    DELAY;
	    *ctrl2 = 0xc0;  /* Set Interrupt Enable */
	    DELAY;

	    /* 
	     * verify
	     */
	    *ctrl1 = PPA;
	    DELAY;
	    a = *ctrl2;
	    *ctrl1 = PTA;
	    DELAY;
	    b = *ctrl2;
	    *ctrl1 = PMA;
	    DELAY;
	    c = *ctrl2;
	    *ctrl1 = IVA;
	    DELAY;
	    d = *ctrl2;
	    if ((a != 0xff) || (b != 0x00) || (c != 0x00) || (d != it))
	    {
		errlogPrintf ("devIcv196: init: error PPA=0x%02x PTA=0x%02x PMA=0x%02x IVA=0x%02x\n",a,b,c,d);
		return ERROR;
	    }

	    /* 
	     * Port B interrupts
	     */
	    *ctrl1 = PPB;   /* Port B Pattern Polarity Register */
	    DELAY;
	    *ctrl2 = 0xff;
	    DELAY;
	    
	    *ctrl1 = PTB;   /* Port B Pattern Transition Register */
	    DELAY;
	    *ctrl2 = 0x00;  /* masked off */
	    DELAY;
	    
	    *ctrl1 = PMB;   /* Port B Pattern Mask Register */
	    DELAY;
	    *ctrl2 = 0x00;  /* masked off */
	    DELAY;

	    *ctrl1 = IVB;   /* Port B Interrupt Vector Register */
	    DELAY;
	    it = IT_VECTOR + 2*card + 1;
	    *ctrl2 = it;  /* Interrupt Vector */
	    DELAY;

	    *ctrl1 = PCSB;  /* Port B Command and Status Register */
	    DELAY;
	    *ctrl2 = 0xc0;  /* Set Interrupt Enable */
	    DELAY;

	    /* 
	     * verify
	     */
	    *ctrl1 = PPB;
	    DELAY;
	    a = *ctrl2;
	    *ctrl1 = PTB;
	    DELAY;
	    b = *ctrl2;
	    *ctrl1 = PMB;
	    DELAY;
	    c = *ctrl2;
	    *ctrl1 = IVB;
	    DELAY;
	    d = *ctrl2;
	    if ((a != 0xff) || (b != 0x00) || (c != 0x00) || (d != it))
	    {
		errlogPrintf ("devIcv196: init: error PPB=0x%02x PTB=0x%02x PMB=0x%02x IVB=0x%02x\n",a,b,c,d);
		return ERROR;
	    }

	    /* 
	     * Common 
	     */
	    *ctrl1 = MCC;   /* Master Configuration Control Register */
	    DELAY;
	    *ctrl2 = 0x84;  /* Port A and B Enable */
	    DELAY;
	    
	    *ctrl1 = MIC;   /* Master Interrupt Control Register */
	    DELAY;
	    *ctrl2 = 0x80;  /* Master Interrupt Enable */
	    DELAY;

	    /* 
	     * verify
	     */
	    *ctrl1 = MCC;
	    DELAY;
	    a = *ctrl2;
	    if (a != 0x84)
	    {
		errlogPrintf ("devIcv196: init: error MCC=0x%02x\n",a);
		return ERROR;
	    }

	    /* 
	     * connect Port A interrupt routine 
	     */
	    itVector = IT_VECTOR + 2*card;
	    if (devConnectInterruptVME (itVector,
					(void (*)(void *))int_service,
					(void *)(2*card)) != OK)
	    {
		errlogPrintf ("devIcv196: init: card %d ISR install error\n", card);
		return ERROR;
	    }
	    if (devIcv196Verbose)
		printf ("devIcv196: init: card %d ISR install ok, vector=0x%x\n", card, itVector);

	    /* 
	     * connect Port B interrupt routine 
	     */
	    itVector = IT_VECTOR + 2*card + 1;
	    if (devConnectInterruptVME (itVector,
					(void (*)(void *))int_service,
					(void *)(2*card + 1)) != OK)
	    {
		errlogPrintf ("devIcv196: init: card %d ISR install error\n", card);
		return ERROR;
	    }
	    if (devIcv196Verbose)
		printf ("devIcv196: init: card %d ISR install ok, vector=0x%x\n", card, itVector);

	    /* 
	     * enable a bus interrupt level 
	     */
	    if (devEnableInterruptLevelVME (IT_LEVEL) != OK)
	    {
		errlogPrintf ("devIcv196: init: card %d enable interrupt level error\n", card);
		return ERROR;
	    }
	    if (devIcv196Verbose)
		printf ("devIcv196: init: card %d enable interrupt level ok\n", card);
	} 
	else 
	{
	    ppdio_icv196[card] = 0;
	}
    }
    
    return OK;
}



/*
 * Generate device report
 */

static long
report (
	int interest
	)
{
    int card, port;

    for (card = 0; card < ICV196_MAX_CARDS; card++) 
    {
	if (ppdio_icv196[card])
	{
	    printf ("Report ICV196 card %d:\n", card);
	    printf ("- VME address = 0x%x\n", ppdio_icv196[card]);
	    printf ("- direction register = 0x%03x\n", dirs[card]);
	    printf ("- signals:\n");
	    for (port = 0; port < 3; port++)
		printf ("  J%d (%02d-%02d): 0x%04x%04x\n", port+1, (port+1)*32-1, port*32,
			ppdio_icv196[card]->ports[2*port+1], ppdio_icv196[card]->ports[2*port]);
	}
    }
     
    return OK;
}



/*__________________________________________________________________
 *
 *	bi Device Support
 */

static long 
init_bi_record (
    struct biRecord *pbi
    )
{
    struct vmeio *pvmeio;
    int event;
    char *pc;

    pbi->dpvt = (void *)0;
    
    switch (pbi->inp.type) 
    {
    case (VME_IO):

	pvmeio = (struct vmeio *)&(pbi->inp.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_bi_record: %s invalid card number %d\n",
			  pbi->name, pvmeio->card);
	    pbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_bi_record: %s invalid card number %d\n",
			  pbi->name, pvmeio->card);
	    pbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV196_MAX_CHANS)
	{
	    errlogPrintf ("devIcv196: init_bi_record: %s invalid signal number %d\n",
			  pbi->name, pvmeio->signal);
	    pbi->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check if "event n" is present in parameter field of signal 0-15 
	 */
	if ((pvmeio->signal < 16) && (pc = strstr (pvmeio->parm, "event"))) 
	{
	    pc += 5;
	    if (sscanf (pc, "%d", &event) != 1)
	    {
		errlogPrintf ("devIcv196: init_bi_record: %s invalid INP parameter\n",
			     pbi->name);
		pbi->dpvt = (void *)1;
		return ERROR;
	    }
	    
	    if ((event < 0) || (event > 255))
	    {
		errlogPrintf ("devIcv196: init_bi_record: %s invalid event value %d [0-255]\n",
			      pbi->name, event);
		pbi->dpvt = (void *)1;
		return ERROR;
	    }
	    
	    if (devIcv196Verbose)
		printf ("\ndevIcv196: init_bi_record: %s card %d signal %d interrupt event=%d\n", 
			pbi->name, pvmeio->card, pvmeio->signal, event);
	    
	    /*
	     * set interrupt
	     */
	    config_int (pvmeio->card, pvmeio->signal, event);
	}
	else
	{
	    if (devIcv196Verbose)
		printf ("\ndevIcv196: init_bi_record: %s card %d signal %d\n", 
			pbi->name, pvmeio->card, pvmeio->signal);
	}
	
	/*
	 * configure input bit
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, 1, 0));
	
    default:
    
	errlogPrintf ("devIcv196: init_bi_record: %s illegal INP field\n", pbi->name);
	pbi->dpvt = (void *)1;
	return ERROR;
    }
}



static long 
read_bi (
    struct biRecord *pbi
    )
{
    struct vmeio *pvmeio;
    int status;
    unsigned int value;

    if (pbi->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(pbi->inp.value);
    
    status = read_bit (pvmeio->card, pvmeio->signal, &value);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: read_bi: %s value=0x%04x\n", pbi->name, value);

    if (status == 0)
    {
	pbi->rval = value;
    } 

    return status;
}



/*
 * Create the dset for devBiIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_bi;
} devBiIcv196 = {
    5,
    report,
    init,
    init_bi_record,
    NULL,
    read_bi
};
epicsExportAddress(dset,devBiIcv196);



/*__________________________________________________________________
 *
 *	bo Device Support
 */

static long 
init_bo_record (
    struct boRecord *pbo
    )
{
    unsigned int value = 0;
    struct vmeio *pvmeio;

    pbo->dpvt = (void *)0;
    
    switch (pbo->out.type)
    {
    case (VME_IO):
	
	pvmeio = (struct vmeio *)&(pbo->out.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_bo_record: %s invalid card number %d\n",
			  pbo->name, pvmeio->card);
	    pbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_bo_record: %s invalid card number %d\n",
			  pbo->name, pvmeio->card);
	    pbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if ((pvmeio->signal <= 15) || (pvmeio->signal >= ICV196_MAX_CHANS))
	{
	    errlogPrintf ("devIcv196: init_bo_record: %s invalid signal number %d\n",
			  pbo->name, pvmeio->signal);
	    pbo->dpvt = (void *)1;
	    return ERROR;
	}

	/* 
	 * read current value 
	 */
	if (read_bit (pvmeio->card, pvmeio->signal, &value))
	{
	    pbo->dpvt = (void *)1;
	    return ERROR;
	}

	pbo->rval = value;

	if (devIcv196Verbose)
	    printf ("\ndevIcv196: init_bo_record: %s card %d signal %d rval=0x%04x\n",
		    pbo->name, pvmeio->card, pvmeio->signal, pbo->rval);

	/*
	 * configure output bit
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, 1, 1));
    
    default:
    
	errlogPrintf ("devIcv196: init_bo_record: %s illegal OUT field\n", pbo->name);
	pbo->rval = value;
	return ERROR;
    }
}



static long 
write_bo (
    struct boRecord *pbo
    )
{
    struct vmeio *pvmeio;
    int status;

    if (pbo->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(pbo->out.value);

    status = write_bit (pvmeio->card, pvmeio->signal, pbo->rval);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: write_bo: %s value=0x%04x\n", pbo->name, pbo->rval);

    return status;
}



/*
 * Create the dset for devBoIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_bo;
} devBoIcv196 = {
    5,
    NULL,
    NULL,
    init_bo_record,
    NULL,
    write_bo
};
epicsExportAddress(dset,devBoIcv196);



/*__________________________________________________________________
 *
 *	mbbi Device Support
 */


static long 
init_mbbi_record (
    struct mbbiRecord *pmbbi
    )
{
    struct vmeio *pvmeio;
    int port;

    pmbbi->dpvt = (void *)0;
    
    switch ( pmbbi->inp.type ) {

    case (VME_IO):

	pvmeio = (struct vmeio *)&(pmbbi->inp.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_mbbi_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_mbbi_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV196_MAX_CHANS)
	{
	    errlogPrintf ("devIcv196: init_mbbi_record: %s invalid signal number %d\n",
			  pmbbi->name, pvmeio->signal);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbi->nobt > 16)
	{
	    errlogPrintf ("devIcv196: init_mbbi_record: %s NOBT > 16\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbi->nobt) > ICV196_MAX_CHANS)
	{
	    errlogPrintf ("devIcv196: init_mbbi_record: %s invalid NOBT\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	port = pvmeio->signal / 16;
	pmbbi->shft = pvmeio->signal % 16;
	pmbbi->mask <<= pmbbi->shft;
	
	if (devIcv196Verbose)
	    printf ("\ndevIcv196: init_mbbi_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x\n", 
		    pmbbi->name, pvmeio->card, pvmeio->signal, pmbbi->nobt, pmbbi->shft, pmbbi->mask);

	/*
	 * configure input pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbi->nobt, 0));

    default:
    
	errlogPrintf ("devIcv196: init_mbbi_record: %s illegal INP field\n", pmbbi->name);
	pmbbi->dpvt = (void *)1;
	return ERROR;
    }
}



static long
read_mbbi (
    struct mbbiRecord *pmbbi
    )
{
    struct vmeio *pvmeio;
    int	status;
    unsigned int value;

    if (pmbbi->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(pmbbi->inp.value);

    status = read_pattern (pvmeio->card, pvmeio->signal, pmbbi->mask, &value);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: read_mbbi: %s value=0x%08x\n", pmbbi->name, value);

    if (status == 0)
    {
	pmbbi->rval = value;
    } 

    return status;
}



/*
 * Create the dset for devMbbiIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbi;
} devMbbiIcv196 = {
    5,
    NULL,
    NULL,
    init_mbbi_record,
    NULL,
    read_mbbi
};
epicsExportAddress(dset,devMbbiIcv196);



/*__________________________________________________________________
 *
 *	mbbo Device Support
 */

static long 
init_mbbo_record (
    struct mbboRecord *pmbbo
    )
{
    struct vmeio *pvmeio;
    unsigned int value;
    int port;

    pmbbo->dpvt = (void *)0;
    
    switch ( pmbbo->out.type ) {

    case (VME_IO):

	pvmeio = (struct vmeio *) &(pmbbo->out.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_mbbo_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_mbbo_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if ((pvmeio->signal <= 15) || (pvmeio->signal >= ICV196_MAX_CHANS))
	{
	    errlogPrintf ("devIcv196: init_mbbo_record: %s invalid signal number %d\n",
			  pmbbo->name, pvmeio->signal);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbo->nobt > 16)
	{
	    errlogPrintf ("devIcv196: init_mbbo_record: %s NOBT > 16\n", pmbbo->name);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbo->nobt) > ICV196_MAX_CHANS)
	{
	    errlogPrintf ("devIcv196: init_mbbo_record: %s invalid NOBT\n", pmbbo->name);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	port = pvmeio->signal / 16;
	pmbbo->shft = pvmeio->signal % 16;
	pmbbo->mask <<= pmbbo->shft;
	
	/* 
	 * read current value 
	 */
	if (read_pattern (pvmeio->card, pvmeio->signal, pmbbo->mask, &value))
	{
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}

	pmbbo->rbv = pmbbo->rval = value;

	if (devIcv196Verbose)
	    printf ("\ndevIcv196: init_mbbo_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x rval=0x%08x\n", 
		    pmbbo->name, pvmeio->card, pvmeio->signal, pmbbo->nobt, pmbbo->shft, pmbbo->mask, pmbbo->rval);

	/*
	 * configure output pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbo->nobt, 1));

    default:
    
	errlogPrintf ("devIcv196: init_mbbo_record: %s illegal OUT field\n", pmbbo->name);
	pmbbo->dpvt = (void *)1;
	return ERROR;
    }
}



static long
write_mbbo (
    struct mbboRecord *pmbbo
    )
{
    struct vmeio *pvmeio;
    int	status;

    if (pmbbo->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(pmbbo->out.value);

    status = write_pattern (pvmeio->card, pvmeio->signal, pmbbo->mask, pmbbo->rval);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: write_mbbo: %s value=0x%08x\n", pmbbo->name, pmbbo->rval);

    return status;
}



/*
 * Create the dset for devMbboIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo;
} devMbboIcv196 = {
    5,
    NULL,
    NULL,
    init_mbbo_record,
    NULL,
    write_mbbo
};
epicsExportAddress(dset,devMbboIcv196);



/*__________________________________________________________________
 *
 *	mbbiDirect Device Support
 */


static long 
init_mbbiDirect_record (
    struct mbbiDirectRecord *pmbbi
    )
{
    struct vmeio *pvmeio;
    int port;

    pmbbi->dpvt = (void *)0;
    
    switch ( pmbbi->inp.type ) {

    case (VME_IO):

	pvmeio = (struct vmeio *)&(pmbbi->inp.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_mbbiDirect_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_mbbiDirect_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV196_MAX_CHANS)
	{
	    errlogPrintf ("devIcv196: init_mbbiDirect_record: %s invalid signal number %d\n",
			  pmbbi->name, pvmeio->signal);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbi->nobt > 16)
	{
	    errlogPrintf ("devIcv196: init_mbbiDirect_record: %s NOBT > 16\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbi->nobt) > ICV196_MAX_CHANS)
	{
	    errlogPrintf ("devIcv196: init_mbbiDirect_record: %s invalid NOBT\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	port = pvmeio->signal / 16;
	pmbbi->shft = pvmeio->signal % 16;
	pmbbi->mask <<= pmbbi->shft;
	
	if (devIcv196Verbose)
	    printf ("\ndevIcv196: init_mbbiDirect_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x\n", 
		    pmbbi->name, pvmeio->card, pvmeio->signal, pmbbi->nobt, pmbbi->shft, pmbbi->mask);

	/*
	 * configure input pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbi->nobt, 0));

    default:
    
	errlogPrintf ("devIcv196: init_mbbiDirect_record: %s illegal INP field\n", pmbbi->name);
	pmbbi->dpvt = (void *)1;
	return ERROR;
    }
}



static long
read_mbbiDirect (
    struct mbbiDirectRecord *pmbbi
    )
{
    struct vmeio *pvmeio;
    int	status;
    unsigned int value;

    if (pmbbi->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(pmbbi->inp.value);

    status = read_pattern (pvmeio->card, pvmeio->signal, pmbbi->mask, &value);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: read_mbbiDirect: %s value=0x%08x\n", pmbbi->name, value);

    if (status == 0)
    {
	pmbbi->rval = value;
    } 

    return status;
}



/*
 * Create the dset for devMbbiDirectIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbi;
} devMbbiDirectIcv196 = {
    5,
    NULL,
    NULL,
    init_mbbiDirect_record,
    NULL,
    read_mbbiDirect
};
epicsExportAddress(dset,devMbbiDirectIcv196);



/*__________________________________________________________________
 *
 *	mbboDirect Device Support
 */

static long 
init_mbboDirect_record (
    struct mbboDirectRecord *pmbbo
    )
{
    struct vmeio *pvmeio;
    unsigned int value;
    int port;

    pmbbo->dpvt = (void *)0;
    
    switch ( pmbbo->out.type ) {

    case (VME_IO):

	pvmeio = (struct vmeio *) &(pmbbo->out.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_mbboDirect_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_mbboDirect_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if ((pvmeio->signal <= 15) || (pvmeio->signal >= ICV196_MAX_CHANS))
	{
	    errlogPrintf ("devIcv196: init_mbboDirect_record: %s invalid signal number %d\n",
			  pmbbo->name, pvmeio->signal);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbo->nobt > 16)
	{
	    errlogPrintf ("devIcv196: init_mbboDirect_record: %s NOBT > 16\n", pmbbo->name);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbo->nobt) > ICV196_MAX_CHANS)
	{
	    errlogPrintf ("devIcv196: init_mbboDirect_record: %s invalid NOBT\n", pmbbo->name);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	port = pvmeio->signal / 16;
	pmbbo->shft = pvmeio->signal % 16;
	pmbbo->mask <<= pmbbo->shft;
	
	/* 
	 * read current value 
	 */
	if (read_pattern (pvmeio->card, pvmeio->signal, pmbbo->mask, &value))
	{
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}

	pmbbo->rbv = pmbbo->rval = value;

	if (devIcv196Verbose)
	    printf ("\ndevIcv196: init_mbboDirect_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x rval=0x%08x\n", 
		    pmbbo->name, pvmeio->card, pvmeio->signal, pmbbo->nobt, pmbbo->shft, pmbbo->mask, pmbbo->rval);

	/*
	 * configure output pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbo->nobt, 1));

    default:
    
	errlogPrintf ("devIcv196: init_mbboDirect_record: %s illegal OUT field\n", pmbbo->name);
	pmbbo->dpvt = (void *)1;
	return ERROR;
    }
}



static long
write_mbboDirect (
    struct mbboDirectRecord *pmbbo
    )
{
    struct vmeio *pvmeio;
    int	status;

    if (pmbbo->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(pmbbo->out.value);

    status = write_pattern (pvmeio->card, pvmeio->signal, pmbbo->mask, pmbbo->rval);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: write_mbboDirect: %s value=0x%08x\n", pmbbo->name, pmbbo->rval);

    return status;
}



/*
 * Create the dset for devMbboDirectIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo;
} devMbboDirectIcv196 = {
    5,
    NULL,
    NULL,
    init_mbboDirect_record,
    NULL,
    write_mbboDirect
};
epicsExportAddress(dset,devMbboDirectIcv196);



/*__________________________________________________________________
 *
 *	longin Device Support
 */


static long 
init_longin_record (
    struct longinRecord *plongin
    )
{
    struct vmeio *pvmeio;

    plongin->dpvt = (void *)0;
    
    switch ( plongin->inp.type ) {

    case (VME_IO):

	pvmeio = (struct vmeio *)&(plongin->inp.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_longin_record: %s invalid card number %d\n",
			  plongin->name, pvmeio->card);
	    plongin->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_longin_record: %s invalid card number %d\n",
			  plongin->name, pvmeio->card);
	    plongin->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= 3)
	{
	    errlogPrintf ("devIcv196: init_longin_record: %s invalid signal number %d\n",
			  plongin->name, pvmeio->signal);
	    plongin->dpvt = (void *)1;
	    return ERROR;
	}

	if (devIcv196Verbose)
	    printf ("\ndevIcv196: init_longin_record: %s card %d signal %d\n", 
		    plongin->name, pvmeio->card, pvmeio->signal);

	/*
	 * configure input pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal * 32, 32, 0));

    default:
    
	errlogPrintf ("devIcv196: init_longin_record: %s illegal INP field\n", plongin->name);
	plongin->dpvt = (void *)1;
	return ERROR;
    }
}



static long
read_longin (
    struct longinRecord *plongin
    )
{
    struct vmeio *pvmeio;
    int	status;
    unsigned int value;

    if (plongin->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(plongin->inp.value);

    status = read_pattern (pvmeio->card, pvmeio->signal * 32, 0xFFFFFFFF, &value);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: read_longin: %s value=0x%08x\n", plongin->name, value);

    if (status == 0)
    {
	plongin->val = value;
    } 

    return status;
}



/*
 * Create the dset for devLonginIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_longin;
} devLonginIcv196 = {
    5,
    NULL,
    NULL,
    init_longin_record,
    NULL,
    read_longin
};
epicsExportAddress(dset,devLonginIcv196);



/*__________________________________________________________________
 *
 *	longout Device Support
 */

static long 
init_longout_record (
    struct longoutRecord *plongout
    )
{
    struct vmeio *pvmeio;
    unsigned int value;

    plongout->dpvt = (void *)0;
    
    switch ( plongout->out.type ) {

    case (VME_IO):

	pvmeio = (struct vmeio *) &(plongout->out.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV196_MAX_CARDS)
	{
	    errlogPrintf ("devIcv196: init_longout_record: %s invalid card number %d\n",
			  plongout->name, pvmeio->card);
	    plongout->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv196[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv196: init_longout_record: %s invalid card number %d\n",
			  plongout->name, pvmeio->card);
	    plongout->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if ((pvmeio->signal <= 1) || (pvmeio->signal >= 3))
	{
	    errlogPrintf ("devIcv196: init_longout_record: %s invalid signal number %d\n",
			  plongout->name, pvmeio->signal);
	    plongout->dpvt = (void *)1;
	    return ERROR;
	}

	/* 
	 * read current value 
	 */
	if (read_pattern (pvmeio->card, pvmeio->signal * 32, 0xFFFFFFFF, &value))
	{
	    plongout->dpvt = (void *)1;
	    return ERROR;
	}

	plongout->val = value;

	if (devIcv196Verbose)
	    printf ("\ndevIcv196: init_longout_record: %s card %d signal %d\n", 
		    plongout->name, pvmeio->card, pvmeio->signal);

	/*
	 * configure output pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal * 32, 32, 1));

    default:
    
	errlogPrintf ("devIcv196: init_longout_record: %s illegal OUT field\n", plongout->name);
	plongout->dpvt = (void *)1;
	return ERROR;
    }
}



static long
write_longout (
    struct longoutRecord *plongout
    )
{
    struct vmeio *pvmeio;
    int	status;

    if (plongout->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(plongout->out.value);

    status = write_pattern (pvmeio->card, pvmeio->signal * 32, 0xFFFFFFFF, plongout->val);

    if (devIcv196Verbose == 2)
	printf ("devIcv196: write_longout: %s value=0x%08x\n", plongout->name, plongout->val);

    return status;
}



/*
 * Create the dset for devLongoutIcv196
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_longout;
} devLongoutIcv196 = {
    5,
    NULL,
    NULL,
    init_longout_record,
    NULL,
    write_longout
};
epicsExportAddress(dset,devLongoutIcv196);
