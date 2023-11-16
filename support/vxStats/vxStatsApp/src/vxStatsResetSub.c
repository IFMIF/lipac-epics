/*
 * C.E.A. IRFU/SIS/LDII
 *
 * @(#) $Id: vxStatsResetSub.c,v 1.2 2010/10/27 12:09:14 cmsmgr Exp $
 *
 * init and process functions for the sub record $(IOC):cpuReset
 *
 * who       when      what
 * --------  --------  ----------------------------------------------
 * ylussign  25/10/10  created
 * ylussign  27/10/10  updated documentation
 */

/** 
 * @file
 * @brief MVME5500 Failsafe timer support.
 *
 * This module provides support for the Failsafe timer available with the VxWorks
 * BSP of the MVME5500 CPU board. Failsafe timer expiration is reported via a board
 * reset event.
 *
 * The module provides Init and Process functions for the sub record @b "$(IOC):cpuReset"
 * defined in the database file @b vxStatsReset.db. These functions are defined in the
 * database definition file @b vxStatsSupport.dbd and included in the library @b libvxStats.a.
 * 
 * To include this support in a VxWorks IOC:
 * - add the library in <ioc>/src/Makefile:
 * @n <ioc>_LIBS += vxStats
 * - add the dbd in <ioc>/src/<ioc>Include.dbd:
 * @n include "vxStatsSupport.dbd"
 * - add the db in <ioc>/db/<ioc>.substitutions:
 * @n file "/home/epicsmgr/EPICS/support/vxStats/db/vxStatsReset.db"
 * @n { { IOC = <ioc> } }
 * 
 * At IOC initialization time, the record Init function is called to set the Real-Time
 * Clock with the current time and start it. Then it sets the failsafe timer with an
 * expiration delay of 2 seconds. The record Process function is called each time the
 * record is processed by the database scanning and starts again the failsafe timer
 * for 2 seconds. The SCAN field of the record "$(IOC):cpuReset" is fixed to 1 second.
 * If the VxWorks or the EPICS system goes down, the board is reset after 2 seconds.
 */
 
#include <stdio.h>
#include <time.h>
#include <dbDefs.h>
#include <registryFunction.h>
#include <subRecord.h>
#include <epicsExport.h>
#include <epicsTime.h>

/*
 * This structure holds the Real-Time Clock configuration values
 */
typedef struct rtcdt 
    {
    int  century;		/* century */
    int  year;			/* year */
    int  month;			/* month */
    int  day_of_month; 		/* day of month */
    int  day_of_week;		/* day of week */
    int  hour;			/* hour */
    int  minute;		/* minute */
    int  second;		/* second */
    } RTC_DATE_TIME; 

/* Set the Real-Time Clock */
STATUS sysRtcSet (RTC_DATE_TIME *rtc_time);
/* Set the failsafe timer */
STATUS sysFailsafeSet (UCHAR seconds, BOOL reset);

/* 
 * Subroutine record init function
 */
static long vxStatsResetSubInit (subRecord *precord)
{
    epicsTimeStamp ts;
    struct tm etm;
    unsigned long nano;
    RTC_DATE_TIME rtc;
    int year;
    
    /*
     * Real-Time Clock initialization
     */
    epicsTimeGetCurrent (&ts);
    epicsTimeToTM (&etm, &nano, &ts);
    rtc.second = etm.tm_sec;
    rtc.minute = etm.tm_min;
    rtc.hour = etm.tm_hour;
    rtc.day_of_week = etm.tm_wday;
    rtc.day_of_month = etm.tm_mday;
    rtc.month = etm.tm_mon;
    year = 1900 + etm.tm_year;
    rtc.year = year % 100;
    rtc.century = year / 100;
    if (sysRtcSet(&rtc) == ERROR)
	printf("RTC initialization error\n");
    
    /*
     *  Failsafe timer initialization
     */
    if (sysFailsafeSet(2, TRUE) == ERROR)
	printf("Failsafe timer initialization error\n");
    
    printf("Failsafe reset timer started\n");
    
    return(0);
}

/* 
 * Subroutine record process function
 */
static long vxStatsResetSubProcess (subRecord *precord)
{
    sysFailsafeSet(2, TRUE);
    return(0);
}

/* 
 * Register these symbols for use by IOC code
 */
epicsRegisterFunction(vxStatsResetSubInit);
epicsRegisterFunction(vxStatsResetSubProcess);
