#******************************************************************************
#
# "@(#) $Id: Makefile 23 2013-03-13 15:38:34Z lussi $" 
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  05/04/07   created
# ylussign  21/02/08   changed ARCH 
# ylussign  20/06/08   changed ARCH 
# ylussign  09/08/08   added envPaths for RTEMS
#
#******************************************************************************

TOP = ../../..

include $(TOP)/configure/CONFIG

#========================================
# build cdCommands for vxWorks

ARCH = vxWorks-mvme5500
TARGETS = cdCommands

#========================================
# build envPaths for RTEMS

#ARCH = RTEMS-mvme5500
#TARGETS = envPaths

include $(TOP)/configure/RULES.ioc
