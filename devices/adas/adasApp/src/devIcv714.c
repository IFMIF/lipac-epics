/******************************************************************************
 * CEA - Direction des Sciences de la Matiere - IRFU/SIS
 * CE-SACLAY, 91191 Gif-sur-Yvette, France
 *
 * $Id: devIcv714.c 23 2013-03-13 15:38:34Z lussi $
 *
 * ADAS ICV 714 Device Support
 *
 *      Author: JFG
 *
 * who       when       what
 * --------  --------   ----------------------------------------------
 * jgournay  08/10/93   created
 * jhosselet 07/12/06   updated for 3.14
 * ylussign  09/10/07   removed all warnings
 * ylussign  07/07/08   - merged device and driver support
 *                      - field DPVT used to mark bad records
 *                      - added configuration functions
 *                      - use Device Support Library devLib
 *                      - added RTEMS support
 * ylussign  24/11/09   - added include errlog.h
 *                      - added doxygen documentation
 */

/** 
 * @file
 * @brief ADAS ICV714 & ICV712 Device Support for EPICS R3.14.
 * 
 * ICV714 Device Support accepts up to 4 boards in a VME crate, starting 
 * from address @b 0x600000 with an increment of 0x100. The obsolete board
 * ICV712 is also supported.
 * 
 * It supports AO record type. The device type @b DTYP is @b ICV714.
 * 
 * The following IOC shell functions allow to change the ICV714 device 
 * configuration. They may be called from an application, from the shell
 * or from a startup script.
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
#include <cvtTable.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <recSup.h>
#include <devSup.h>
#include <link.h>
#include <aoRecord.h>
#include <dbScan.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <devLib.h>
#include <errlog.h>

/* VME ICV714 defines */

#define ICV714_BASE (char *)0x600000	/* VME base address */
#define ICV714_SIZE            0x100	/* VME memory length */
#define CS_ARRAY                0x20	/* load signal values from NOVRAM */
#define CS_STORE                0x40	/* store signal values in NOVRAM */
#define CS_CLEAR                0x80	/* reset all output signals */
#define CTRL_REG                0xC0	/* control register */
#define CTRL_BUSY               0x80	/* control register BUSY bit */
#define ID_REG                  0xE0	/* Identification register */
#define MAX_ICV714_CARDS           4	/* max. number of boards in a VME crate */
#define ICV714_MAXCHAN            16	/* number of output signals */

static char *icv714[MAX_ICV714_CARDS];  /* VME address */

/**
 * This IOC shell variable allows to print debug messages.
 * Valid range is:
 * - 0 no message is printed
 * - 1 messages at initialization are printed
 * - 2 initialization and I/O messages are printed
 */
int devIcv714Verbose = 0;
epicsExportAddress(int, devIcv714Verbose);

#define devMapAddr(a,b,c,d,e) ((pdevLibVirtualOS->pDevMapAddr)(a,b,c,d,e))


/*__________________________________________________________________
 *
 *	Service and configuration functions
 */

static char
*mapAddress (
	int card
	)
{
    char *icv714Addr;
    short dum;
    size_t vmeAddress = (size_t)(ICV714_BASE + card * ICV714_SIZE);

    if (devMapAddr (atVMEA24,
		    0,
		    vmeAddress,
		    0,
		   (volatile void **)&icv714Addr) != OK)
	return NULL;
    
    printf ("mapAddress: VME-adrs=0x%x CPU-adrs=0x%x\n", vmeAddress, (int)icv714Addr);
    
    if (devReadProbe (sizeof (short),
		     (volatile const void *)icv714Addr,
		     (void *)&dum) != OK)
	return NULL;
		      
    return icv714Addr;
}



/**
 * This IOC shell function changes the binary output value 
 * of a channel in RAM. To make this change permanent, it is necessary
 * to store the values in the on board NOVRAM by calling icv714StoreValues();
 */
 
void
icv714OutValue (
	/** [in] ICV714 card number. Valid range: 0 to 3 */
	int card,
	/** [in] signal number. Valid range: 0 to 15 */
	int signal,
	/** [in] signal value. Valid range: 0 to 4095 */
	int value
	)
{
    unsigned short *icv714Addr;

    if ((icv714Addr = (unsigned short *) mapAddress (card)) == NULL)
    {
	printf ("icv714OutValue: missing card %d\n", card);
	return;
    }
    
    icv714Addr += (signal & 0xf);
    *icv714Addr = (unsigned short)value & 0xfff;

    printf ("icv714OutValue: card %d signal %d value=%d done\n",
	    card, signal, value);

    return;
}

/* Information needed by iocsh */
static const iocshArg     icv714OutValueArg0 = {"card", iocshArgInt};
static const iocshArg     icv714OutValueArg1 = {"signal", iocshArgInt};
static const iocshArg     icv714OutValueArg2 = {"value", iocshArgInt};
static const iocshArg    *icv714OutValueArgs[] = {&icv714OutValueArg0, &icv714OutValueArg1, &icv714OutValueArg2};
static const iocshFuncDef icv714OutValueFuncDef = {"icv714OutValue", 3, icv714OutValueArgs};

/* Wrapper called by iocsh, selects the argument types that icv714OutValue needs */
static void icv714OutValueCallFunc(const iocshArgBuf *args) {
    icv714OutValue(args[0].ival, args[1].ival, args[2].ival);
}

/* Registration routine, runs at startup */
static void icv714OutValueRegister(void) {
    iocshRegister(&icv714OutValueFuncDef, icv714OutValueCallFunc);
}
epicsExportRegistrar(icv714OutValueRegister);



/**
 * This IOC shell function stores the current signal output values in
 * permanent memory NOVRAM. At power-on these values will be loaded
 * into RAM thus allowing the board to output pre-defined values
 * before the EPICS software startup.
 */
 
void
icv714StoreValues (
	/** [in] ICV714 card number. Valid range: 0 to 3 */
	int card
	)
{
    char *icv714Addr;
    unsigned short *pReg;

    if ((icv714Addr = mapAddress (card)) == NULL)
    {
	printf ("icv714StoreValues: missing card %d\n", card);
	return;
    }

    /* store RAM values in NOVRAM */
    pReg = (unsigned short *)(icv714Addr + CS_STORE);
    *pReg = 0;
    
    /* wait */
    taskDelay(1);
/*    pReg = (unsigned short *)(icv714Addr + CTRL_REG);
    while (*pReg & CTRL_BUSY);*/
    
    /* reload values in RAM */
    pReg = (unsigned short *)(icv714Addr + CS_ARRAY);
    *pReg = 0;
    
    printf ("icv714StoreValues: card %d done\n", card);

    return;
}

/* Information needed by iocsh */
static const iocshArg     icv714StoreValuesArg0 = {"card", iocshArgInt};
static const iocshArg    *icv714StoreValuesArgs[] = {&icv714StoreValuesArg0};
static const iocshFuncDef icv714StoreValuesFuncDef = {"icv714StoreValues", 1, icv714StoreValuesArgs};

/* Wrapper called by iocsh, selects the argument types that icv714StoreValues needs */
static void icv714StoreValuesCallFunc(const iocshArgBuf *args) {
    icv714StoreValues(args[0].ival);
}

/* Registration routine, runs at startup */
static void icv714StoreValuesRegister(void) {
    iocshRegister(&icv714StoreValuesFuncDef, icv714StoreValuesCallFunc);
}
epicsExportRegistrar(icv714StoreValuesRegister);



/*__________________________________________________________________
 *
 *	AO Device Support 
 */

/*
 * Generate device report
 */

static long
report (
	int interest
	)
{
    int card;

    for (card = 0; card < MAX_ICV714_CARDS; card++) 
    {
	if (icv714[card])
	{
	    printf ("Report ICV714 card %d: VME address = 0x%x\n", card, (int)icv714[card]);
	}
    }
     
    return OK;
}



/*
 * Set linear conversion slope
 */

static long
special_linconv (
	aoRecord *pao,
	int after
	)
{
    if (!after)
	return (0);
    
    pao->eslo = (pao->eguf - pao->egul) / 4095.0;
    return (0);
}



/*
 * Initialize device processing
 */

static long
init (
	int after
	)
{
    short dum;
    char *icv714Addr;
    int card;

    /*
     * process init only once before
     */
    if (after)
	return OK;

    /* 
     * convert VME address A24/D16 to CPU local address 
     */
    if (devMapAddr (atVMEA24,
		    0,
		   (size_t) ICV714_BASE,
		    0,
		   (volatile void **)&icv714Addr) != OK)
    {
	errlogPrintf ("devIcv714: init: unable to map ICV714 base address\n");
	return ERROR;
    }

    /* 
     * test for ICV714 boards present in the VME crate
     */
    for (card = 0; card < MAX_ICV714_CARDS; card++)
    {
	icv714[card] = 0;
	
	if (devReadProbe (sizeof (short),
			 (volatile const void *)icv714Addr,
			 (void *)&dum) == OK)
	{
	    icv714[card] = icv714Addr;
	    
	    if (devIcv714Verbose)
		printf ("devIcv714: init: card %d present (0x%x)\n",
			card, (int)icv714Addr);
	}

	/* 
	 * next card
	 */
	icv714Addr += ICV714_SIZE;
    }
    
    return OK;
}



/*
 * Initialize AO record
 */

static long
init_ao_record (
	aoRecord *pao
	)
{
    struct vmeio *pvmeio;
    unsigned short *icv714Addr;

    pao->dpvt = (void *)0;
    
    switch (pao->out.type)
    {
    case (VME_IO):
	
	pvmeio = (struct vmeio *)&(pao->out.value);
	
	/*
	 * check card number
	 */
	if (pvmeio->card >= MAX_ICV714_CARDS) 
	{
	    errlogPrintf ("devIcv714: init_ao_record: %s invalid card number %d\n",
			  pao->name, pvmeio->card);
	    pao->dpvt = (void *)1;
	    return ERROR;
	}

	if (icv714[pvmeio->card] == 0)
	{
	    errlogPrintf ("devIcv714: init_ao_record: %s invalid card number %d\n",
			  pao->name, pvmeio->card);
	    pao->dpvt = (void *)1;
	    return ERROR;
	}

	/*
	 * check signal number
	 */
	if (pvmeio->signal >= ICV714_MAXCHAN)
	{
	    errlogPrintf ("devIcv714: init_ao_record: %s invalid signal number %d\n",
			  pao->name, pvmeio->signal);
	    pao->dpvt = (void *)1;
	    return ERROR;
	}
	
	/* 
	 * set linear conversion slope 
	 */
	pao->eslo = (pao->eguf - pao->egul) / 4095.0;
	
	/* 
	 * read current value
	 */
	icv714Addr = (unsigned short *)icv714[pvmeio->card] + pvmeio->signal;
	pao->rval = *icv714Addr & 0xfff;

	if (devIcv714Verbose)
	    printf ("\ndevIcv714: init_ao_record: %s card %d signal %d value=%d\n", 
		    pao->name, pvmeio->card, pvmeio->signal, pao->rval);

	return OK;
    
    default:
	
	errlogPrintf ("devIcv714: init_ao_record: illegal OUT field\n");
	pao->dpvt = (void *)1;
	return ERROR;
    }
}



/*
 * Write signal value
 */

static long
write_ao (
	aoRecord *pao
	)
{
    struct vmeio *pvmeio;
    unsigned short *icv714Addr;

    if (pao->dpvt)
	return ERROR;
	
    pvmeio = (struct vmeio *)&(pao->out.value);
    
    icv714Addr = (unsigned short *)icv714[pvmeio->card] + pvmeio->signal;
    *icv714Addr = pao->rval;
    
    if (devIcv714Verbose == 2)
	printf ("devIcv714: write_ao: card %d signal %d value=%d\r\n",
		pvmeio->card, pvmeio->signal, pao->rval);

    return 0;
}



/*
 * Create the dset for devAoIcv714
 */
struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_ao;
    DEVSUPFUN special_linconv;
} devAoIcv714 = {
    6, 
    report, 
    init, 
    init_ao_record, 
    NULL, 
    write_ao, 
    special_linconv
};
epicsExportAddress(dset, devAoIcv714);
