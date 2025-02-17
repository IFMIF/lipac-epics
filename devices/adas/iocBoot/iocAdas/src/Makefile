#******************************************************************************
#
# "@(#) $Id: Makefile 23 2013-03-13 15:38:34Z lussi $" 
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  12/10/07   created
# ylussign  09/07/08   iocAdas built for vxWorks & RTEMS
#
#******************************************************************************

TOP=../../..

include $(TOP)/configure/CONFIG

#========================================
# Build IOC application iocAdas

# ioc application name
PROD_IOC_vxWorks = iocAdas
PROD_IOC_RTEMS = iocAdas

# iocAdas_registerRecordDeviceDriver.cpp is created from iocAdas.dbd
iocAdas_SRCS += iocAdas_registerRecordDeviceDriver.cpp
iocAdas_SRCS_RTEMS += iocAdasMain.cpp

# Libraries needed by application
iocAdas_LIBS += adas
iocAdas_LIBS += $(EPICS_BASE_IOC_LIBS)
iocAdas_SYS_LIBS_RTEMS = rtemsCom telnetd bspExt

# iocAdas.dbd is created from iocAdasInclude.dbd
DBD += iocAdas.dbd

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

