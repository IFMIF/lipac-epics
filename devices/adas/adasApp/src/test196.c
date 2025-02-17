/******************************************************************************
 * CEA - Direction des Sciences de la Matiere - IRFU/SIS
 * CE-SACLAY, 91191 Gif-sur-Yvette, France
 *
 * $Id: test196.c 23 2013-03-13 15:38:34Z lussi $
 *
 * ADAS ICV 196 test application
 *
 * who       when       what
 * --------  --------   ----------------------------------------------
 * ylussign  02/12/08   created
 */

#include <vxWorks.h>
#include <vxLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <vme.h>
#include <types.h>
#include <stdioLib.h>

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

/* VME ICV196 defines */

#define ICV196_BASE  (char *)0x200000	/* VME base address */
#define ICV196_MAX_CARDS	    2	/* max. number of boards in a VME crate */

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

#define DELAY {int i; for (i=0;i<10000;i++);}

void
test196 (void)
{
    short dummy;
    unsigned short *dump;
    int card;
    int i, j;
    unsigned char a, b, c;
    volatile unsigned char *ctrl1, *ctrl2, *ctrl3;
    struct dio_icv196 *pdio_icv196;

    /* 
     * convert VME address A24/D16 to local address 
     */
    if ( sysBusToLocalAdrs (VME_AM_STD_SUP_DATA,
			    ICV196_BASE,
			    (char **) &pdio_icv196) != OK)
    {
	printf ("unable to map ICV196 base address\n");
	return;
    }

    /* 
     * determine which cards are present and initialize 
     */
    for (card = 0; card < ICV196_MAX_CARDS; card++, pdio_icv196++) {

	/* Do not read at address 0 (clear module) */
	if ( vxMemProbe ((char *) &pdio_icv196->ports,
			 VX_READ,
			 2,
			(char *) &dummy) == OK)
	{
	    printf ("\n==================================================\n");
	    printf ("card %d, address = 0x%x\n", card, pdio_icv196);
	    printf ("==================================================\n\n");
	    ppdio_icv196[card] = pdio_icv196;
	    ctrl1 = &pdio_icv196->z8536_control;
	    ctrl2 = &pdio_icv196->z8536_control;
	    ctrl3 = &pdio_icv196->z8536_control;
	    
	    printf ("dump:\n");
	    dump = (unsigned short *)pdio_icv196;
	    for (i = 0; i < 16; i++)
	    {
		printf("%08x:  ", dump);
		for (j = 0; j < 8; j++)
		{
		    dummy = *dump++;
		    printf("%04hx ", dummy);
		}
		printf("\n");
	    }
	    
	    printf("\nZ8536 Control Register Address = 0x%08x\n", ctrl1);	    
	    
	    /* 
	     * reset the Z8536 
	     */
	    printf("\nReset Z8536:\n");	    
	    a = *ctrl1;
	    *ctrl2 = MIC;
	    DELAY;
	    *ctrl1 = 0x01;
	    DELAY;
	    a = *ctrl3;
	    printf("read MIC: 0x%02x (should be 0x01)\n", a);
	    *ctrl1 = 0x00;
	    DELAY;
	    a = *ctrl2;
	    printf("read MIC: 0x%02x (should be 0x02)\n", a);

	    /* 
	     * Port A
	     */
	    printf("\nInitialize port A registers:\n");	    
	    printf ("write:  PMSA=0x05 DPPA=0x00 DDA=0xff\n");
	    
	    *ctrl1 = PMSA;  /* Port A Mode Specification Register */
	    DELAY;
	    *ctrl2 = 0x05;  /* Bit Port, OR mode, Latch on Pattern Match */
	    DELAY;
	    a = *ctrl3;
	    
	    *ctrl1 = DPPA;  /* Port A Data Path Polarity Register */
	    DELAY;
	    *ctrl2 = 0x00;  /* non-inverting */
	    DELAY;
	    b = *ctrl3;
	    
	    *ctrl1 = DDA;   /* Port A Data Direction Register */
	    DELAY;
	    *ctrl2 = 0xff;  /* 8 Input Bits */
	    DELAY;
	    c = *ctrl3;
	    
	    printf ("read:   PMSA=0x%02x DPPA=0x%02x DDA=0x%02x\n", a, b, c);	        
	    
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
	    printf ("verify: PMSA=0x%02x DPPA=0x%02x DDA=0x%02x\n", a, b, c);
	} 
	else 
	{
	    ppdio_icv196[card] = 0;
	}
    }
    
    return;
}
