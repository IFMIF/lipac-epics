/******************************************************************************
 * CEA - Direction des Sciences de la Matiere - IRFU/SIS
 * CE-SACLAY, 91191 Gif-sur-Yvette, France
 *
 * $Id: devIcv296.c 23 2013-03-13 15:38:34Z lussi $
 *
 * ADAS ICV 296 Device Support
 *
 * 	Author:	Yves Lussignol
 * 	Date:	09/11/07
 *
 * who       when       what
 * --------  --------   ----------------------------------------------
 * ylussign  09/11/07   created
 * ylussign  11/07/08   - use Device Support Library devLib
 *                      - added RTEMS support
 *                      - replaced D32 by D16 access
 * ylussign  24/11/09   - added include errlog.h
 *                      - added doxygen documentation
 */

/** 
 * @file
 * @brief ADAS ICV296 Device Support for EPICS R3.14.
 * 
 * ICV296 Device Support accepts up to 2 boards in a VME crate, 
 * starting from address @b 0x300000 with an increment of 0x20.
 * 
 * It supports the following record types: BI, BO, MBBI, MBBO, 
 * MBBIDIRECT, MBBODIRECT, LONGIN, LONGOUT. The device type @b DTYP
 * is @b ICV296 for all record types.
 * 
 * Signals 0 to 95 may be configured as input or output by groups
 * of 8 signals. The configuration is automatically done and checked 
 * by the record/device init functions.
 *
 * The @b NOBT of records MBBI, MBBO, MBBIDIRECT and MBBODIRECT is 
 * limited to 16 bits by the record support.
 *
 * Records LONGIN and LONGOUT allow to read or write 32 bit patterns.
 * Three patterns are available through signal number S0 to S2,
 * starting at signals 0, 32 and 64.
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

/* VME ICV296 defines */

#define ICV296_BASE  (char *)0x300000	/* VME base address */
#define ICV296_MAX_CARDS	    2	/* max. number of boards in a VME crate */
#define ICV296_MAX_CHANS	   96	/* number of IO signals */

/* 
 * icv296 memory structure (256 bytes) 
 */
struct dio_icv296 {
    unsigned short reset;		/* reset direction register */
    unsigned short dir;			/* direction register (12 bits) */
    unsigned short buffer[6];		/* signal buffer registers */
    unsigned short wdog;		/* keep watch-dog */
    unsigned short latch;		/* signal buffers soft latch */
    unsigned short direct[6];		/* signal direct registers */
};

static struct dio_icv296 *ppdio_icv296[ICV296_MAX_CARDS]; /* pointers to icv296 modules */
static unsigned short dirs[ICV296_MAX_CARDS] = {0,0};     /* direction register */
static unsigned short mdirs[ICV296_MAX_CARDS] = {0,0};    /* direction register modified */

/**
 * This IOC shell variable allows to print debug messages.
 * Valid range is:
 * - 0 no message is printed
 * - 1 messages at initialization are printed
 * - 2 initialization and I/O messages are printed
 */
int devIcv296Verbose = 0;
epicsExportAddress(int, devIcv296Verbose);

#define devMapAddr(a,b,c,d,e) ((pdevLibVirtualOS->pDevMapAddr)(a,b,c,d,e))



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
	    errlogPrintf("devIcv296: config_dir: card %d signal %d inconsistent direction\n",
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

    if (devIcv296Verbose)
	printf ("devIcv296: config_dir: card %d mask=0x%04x direction=0x%03x\n",
		card, mask, dirs[card]);
    
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
    int group, bit;

    /* 
     * convert signal number to 16 bit group number and bit mask 
     */
    group = (signal / 16) ^ 1;
    bit = signal % 16;
    mask = 1 << bit;

    /* 
     * read bit value 
     */
    *value = ppdio_icv296[card]->direct[group] & mask;

    if (devIcv296Verbose == 3)
	printf ("devIcv296: read_bit: card %d signal %d group=%d mask=0x%04x value=0x%04x\n", 
		card, signal, group, mask, *value);

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
    int group, bit;

    /* 
     * convert signal number to 16 bit group number and bit mask 
     */
    group = (signal / 16) ^ 1;
    bit = signal % 16;
    mask = 1 << bit;

    /* 
     * write bit value 
     */
    if (value)
	ppdio_icv296[card]->direct[group] |= mask;
    else
	ppdio_icv296[card]->direct[group] &= ~mask;

    if (devIcv296Verbose == 3)
	printf ("devIcv296: write_bit: card %d signal %d group=%d mask=0x%04x value=0x%04x\n", 
		card, signal, group, mask, value);

     return OK;
}



/*
 * read_pattern - read a bit pattern from the I/O buffer
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
    int low;
    int high;
    unsigned int work;

    /* 
     * convert lowest signal number to 16 bit port number
     */
    port = signal / 16;
    low = port ^ 1;
    high = (port + 1) ^ 1;

    /*
     * read bit pattern 
     */
    if ( port < 5 )
	work = ((ppdio_icv296[card]->direct[high]) << 16) + ppdio_icv296[card]->direct[low];
    else
	work = ppdio_icv296[card]->direct[low];

    /*
     * mask record pattern 
     */
    *value = work & mask;

    if (devIcv296Verbose == 3)
	printf ("devIcv296: read_pattern: card %d signal %d port=%d mask=0x%08x value=0x%08x\n",
		card, signal, port, mask, *value);

    return OK;
}



/*
 * write_pattern - write a bit pattern to the I/O buffer
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
    int low;
    int high;
    unsigned int work;

    /* 
     * convert lowest signal number to 16 bit port number
     */
    port = signal / 16;
    low = port ^ 1;
    high = (port + 1) ^ 1;

    /*
     * read bit pattern 
     */
    if ( port < 5 )
	work = ((ppdio_icv296[card]->direct[high]) << 16) + ppdio_icv296[card]->direct[low];
    else
	work = ppdio_icv296[card]->direct[low];

    /*
     * change record pattern 
     */
    work = (work & ~mask) | (value & mask);
    ppdio_icv296[card]->direct[low] = (unsigned short)(work & 0x0000ffff);   
    if ( port < 5 )
	ppdio_icv296[card]->direct[high] = (unsigned short)((work & 0xffff0000) >> 16);

    if (devIcv296Verbose == 3)
	printf ("devIcv296: write_pattern: card %d signal %d port=%d mask=0x%08x value=0x%08x\n",
		card, signal, port, mask, value);

    return OK;
}



/*
 * Device initialization
 */

static long
init (
	int after
	)
{
    short dummy;
    int card;
    struct dio_icv296 *pdio_icv296;

    /*
     * before records init: initialize everything but direction register
     * after records init: initialize the direction register
     */
    if (after) {
	for (card = 0; card < ICV296_MAX_CARDS; card++) {
	    if (ppdio_icv296[card]) {
		ppdio_icv296[card]->dir = dirs[card];
	    }
	}
	if ( devIcv296Verbose )
	    printf ("\ndevIcv296: init: after done\n");
	return OK;
    }
    
    /* 
     * convert VME address A24/D16 to local address 
     */
    if (devMapAddr (atVMEA24,
		    0,
		   (size_t) ICV296_BASE,
		    0,
		   (volatile void **)&pdio_icv296) != OK)
    {
	errlogPrintf ("devIcv296: init: unable to map ICV296 base address\n");
	return ERROR;
    }

    /* 
     * determine which cards are present and initialize 
     */
    for (card = 0; card < ICV296_MAX_CARDS; card++, pdio_icv296++) {

	/* Do not read at address 0 (reset direction register) */
	if (devReadProbe (sizeof (short),
			 (volatile const void *)&pdio_icv296->dir,
			 (void *)&dummy) == OK)
	{
	    if (devIcv296Verbose)
		printf ("devIcv296: init: card %d present (0x%x)\n", 
			card, pdio_icv296);

	    ppdio_icv296[card] = pdio_icv296;
	} 
	else 
	{
	    ppdio_icv296[card] = 0;
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

    for (card = 0; card < ICV296_MAX_CARDS; card++) 
    {
	if (ppdio_icv296[card])
	{
	    printf ("Report ICV296 card %d:\n", card);
	    printf ("- VME address = 0x%x\n", ppdio_icv296[card]);
	    printf ("- direction register = 0x%03x\n", dirs[card]);
	    printf ("- signals:\n");
	    for (port = 0; port < 3; port++)
		printf ("  J%d (%02d-%02d): 0x%04x%04x\n", port+1, (port+1)*32-1, port*32,
			ppdio_icv296[card]->direct[2*port], ppdio_icv296[card]->direct[2*port+1]);
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

    pbi->dpvt = (void *)0;
    
    switch (pbi->inp.type) 
    {
    case (VME_IO):

	pvmeio = (struct vmeio *)&(pbi->inp.value);
	
	/*
	 * check card availability
	 */
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_bi_record: %s invalid card number %d\n",
			  pbi->name, pvmeio->card);
	    pbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_bi_record: %s invalid card number %d\n",
			  pbi->name, pvmeio->card);
	    pbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_bi_record: %s invalid signal number %d\n",
			  pbi->name, pvmeio->signal);
	    pbi->dpvt = (void *)1;
	    return ERROR;
	}

	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_bi_record: %s card %d signal %d\n", 
		pbi->name, pvmeio->card, pvmeio->signal);
	
	/*
	 * configure input bit
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, 1, 0));
	
    default:
    
	errlogPrintf ("devIcv296: init_bi_record: %s illegal INP field\n", pbi->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: read_bi: %s value=0x%04x\n", pbi->name, value);

    if (status == 0)
    {
	pbi->rval = value;
    } 

    return status;
}



/*
 * Create the dset for devBiIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_bi;
} devBiIcv296 = {
    5,
    report,
    init,
    init_bi_record,
    NULL,
    read_bi
};
epicsExportAddress(dset,devBiIcv296);



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
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_bo_record: %s invalid card number %d\n",
			  pbo->name, pvmeio->card);
	    pbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_bo_record: %s invalid card number %d\n",
			  pbo->name, pvmeio->card);
	    pbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_bo_record: %s invalid signal number %d\n",
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

	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_bo_record: %s card %d signal %d rval=0x%04x\n",
		    pbo->name, pvmeio->card, pvmeio->signal, pbo->rval);

	/*
	 * configure output bit
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, 1, 1));
    
    default:
    
	errlogPrintf ("devIcv296: init_bo_record: %s illegal OUT field\n", pbo->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: write_bo: %s value=0x%04x\n", pbo->name, pbo->rval);

    return status;
}



/*
 * Create the dset for devBoIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_bo;
} devBoIcv296 = {
    5,
    NULL,
    NULL,
    init_bo_record,
    NULL,
    write_bo
};
epicsExportAddress(dset,devBoIcv296);



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
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_mbbi_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_mbbi_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbbi_record: %s invalid signal number %d\n",
			  pmbbi->name, pvmeio->signal);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbi->nobt > 16)
	{
	    errlogPrintf ("devIcv296: init_mbbi_record: %s NOBT > 16\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbi->nobt) > ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbbi_record: %s invalid NOBT\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	port = pvmeio->signal / 16;
	pmbbi->shft = pvmeio->signal % 16;
	pmbbi->mask <<= pmbbi->shft;
	
	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_mbbi_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x\n", 
		    pmbbi->name, pvmeio->card, pvmeio->signal, pmbbi->nobt, pmbbi->shft, pmbbi->mask);

	/*
	 * configure input pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbi->nobt, 0));

    default:
    
	errlogPrintf ("devIcv296: init_mbbi_record: %s illegal INP field\n", pmbbi->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: read_mbbi: %s value=0x%08x\n", pmbbi->name, value);

    if (status == 0)
    {
	pmbbi->rval = value;
    } 

    return status;
}



/*
 * Create the dset for devMbbiIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbi;
} devMbbiIcv296 = {
    5,
    NULL,
    NULL,
    init_mbbi_record,
    NULL,
    read_mbbi
};
epicsExportAddress(dset,devMbbiIcv296);



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
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_mbbo_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_mbbo_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbbo_record: %s invalid signal number %d\n",
			  pmbbo->name, pvmeio->signal);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbo->nobt > 16)
	{
	    errlogPrintf ("devIcv296: init_mbbo_record: %s NOBT > 16\n", pmbbo->name);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbo->nobt) > ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbbo_record: %s invalid NOBT\n", pmbbo->name);
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

	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_mbbo_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x rval=0x%08x\n", 
		    pmbbo->name, pvmeio->card, pvmeio->signal, pmbbo->nobt, pmbbo->shft, pmbbo->mask, pmbbo->rval);

	/*
	 * configure output pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbo->nobt, 1));

    default:
    
	errlogPrintf ("devIcv296: init_mbbo_record: %s illegal OUT field\n", pmbbo->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: write_mbbo: %s value=0x%08x\n", pmbbo->name, pmbbo->rval);

    return status;
}



/*
 * Create the dset for devMbboIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo;
} devMbboIcv296 = {
    5,
    NULL,
    NULL,
    init_mbbo_record,
    NULL,
    write_mbbo
};
epicsExportAddress(dset,devMbboIcv296);



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
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_mbbiDirect_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_mbbiDirect_record: %s invalid card number %d\n",
			  pmbbi->name, pvmeio->card);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbbiDirect_record: %s invalid signal number %d\n",
			  pmbbi->name, pvmeio->signal);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbi->nobt > 16)
	{
	    errlogPrintf ("devIcv296: init_mbbiDirect_record: %s NOBT > 16\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbi->nobt) > ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbbiDirect_record: %s invalid NOBT\n", pmbbi->name);
	    pmbbi->dpvt = (void *)1;
	    return ERROR;
	}
	
	port = pvmeio->signal / 16;
	pmbbi->shft = pvmeio->signal % 16;
	pmbbi->mask <<= pmbbi->shft;
	
	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_mbbiDirect_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x\n", 
		    pmbbi->name, pvmeio->card, pvmeio->signal, pmbbi->nobt, pmbbi->shft, pmbbi->mask);

	/*
	 * configure input pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbi->nobt, 0));

    default:
    
	errlogPrintf ("devIcv296: init_mbbiDirect_record: %s illegal INP field\n", pmbbi->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: read_mbbiDirect: %s value=0x%08x\n", pmbbi->name, value);

    if (status == 0)
    {
	pmbbi->rval = value;
    } 

    return status;
}



/*
 * Create the dset for devMbbiDirectIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbi;
} devMbbiDirectIcv296 = {
    5,
    NULL,
    NULL,
    init_mbbiDirect_record,
    NULL,
    read_mbbiDirect
};
epicsExportAddress(dset,devMbbiDirectIcv296);



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
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_mbboDirect_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_mbboDirect_record: %s invalid card number %d\n",
			  pmbbo->name, pvmeio->card);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbboDirect_record: %s invalid signal number %d\n",
			  pmbbo->name, pvmeio->signal);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check number of bits (NOBT)
	 */
	if (pmbbo->nobt > 16)
	{
	    errlogPrintf ("devIcv296: init_mbboDirect_record: %s NOBT > 16\n", pmbbo->name);
	    pmbbo->dpvt = (void *)1;
	    return ERROR;
	}
	
	if ((pvmeio->signal + pmbbo->nobt) > ICV296_MAX_CHANS)
	{
	    errlogPrintf ("devIcv296: init_mbboDirect_record: %s invalid NOBT\n", pmbbo->name);
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

	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_mbboDirect_record: %s card %d signal %d nobt %d shft=%d mask=0x%08x rval=0x%08x\n", 
		    pmbbo->name, pvmeio->card, pvmeio->signal, pmbbo->nobt, pmbbo->shft, pmbbo->mask, pmbbo->rval);

	/*
	 * configure output pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal, pmbbo->nobt, 1));

    default:
    
	errlogPrintf ("devIcv296: init_mbboDirect_record: %s illegal OUT field\n", pmbbo->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: write_mbboDirect: %s value=0x%08x\n", pmbbo->name, pmbbo->rval);

    return status;
}



/*
 * Create the dset for devMbboDirectIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo;
} devMbboDirectIcv296 = {
    5,
    NULL,
    NULL,
    init_mbboDirect_record,
    NULL,
    write_mbboDirect
};
epicsExportAddress(dset,devMbboDirectIcv296);



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
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_longin_record: %s invalid card number %d\n",
			  plongin->name, pvmeio->card);
	    plongin->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_longin_record: %s invalid card number %d\n",
			  plongin->name, pvmeio->card);
	    plongin->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= 3)
	{
	    errlogPrintf ("devIcv296: init_longin_record: %s invalid signal number %d\n",
			  plongin->name, pvmeio->signal);
	    plongin->dpvt = (void *)1;
	    return ERROR;
	}

	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_longin_record: %s card %d signal %d\n", 
		    plongin->name, pvmeio->card, pvmeio->signal);

	/*
	 * configure input pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal * 32, 32, 0));

    default:
    
	errlogPrintf ("devIcv296: init_longin_record: %s illegal INP field\n", plongin->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: read_longin: %s value=0x%08x\n", plongin->name, value);

    if (status == 0)
    {
	plongin->val = value;
    } 

    return status;
}



/*
 * Create the dset for devLonginIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_longin;
} devLonginIcv296 = {
    5,
    NULL,
    NULL,
    init_longin_record,
    NULL,
    read_longin
};
epicsExportAddress(dset,devLonginIcv296);



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
	if (pvmeio->card >= ICV296_MAX_CARDS)
	{
	    errlogPrintf ("devIcv296: init_longout_record: %s invalid card number %d\n",
			  plongout->name, pvmeio->card);
	    plongout->dpvt = (void *)1;
	    return ERROR;
	}
	
	if (ppdio_icv296[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv296: init_longout_record: %s invalid card number %d\n",
			  plongout->name, pvmeio->card);
	    plongout->dpvt = (void *)1;
	    return ERROR;
	}
	
	/*
	 * check signal number
	 */
	if (pvmeio->signal >= 3)
	{
	    errlogPrintf ("devIcv296: init_longout_record: %s invalid signal number %d\n",
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

	if (devIcv296Verbose)
	    printf ("\ndevIcv296: init_longout_record: %s card %d signal %d\n", 
		    plongout->name, pvmeio->card, pvmeio->signal);

	/*
	 * configure output pattern
	 */
	return (config_dir(pvmeio->card, pvmeio->signal * 32, 32, 1));

    default:
    
	errlogPrintf ("devIcv296: init_longout_record: %s illegal OUT field\n", plongout->name);
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

    if (devIcv296Verbose == 2)
	printf ("devIcv296: write_longout: %s value=0x%08x\n", plongout->name, plongout->val);

    return status;
}



/*
 * Create the dset for devLongoutIcv296
 */

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_longout;
} devLongoutIcv296 = {
    5,
    NULL,
    NULL,
    init_longout_record,
    NULL,
    write_longout
};
epicsExportAddress(dset,devLongoutIcv296);
