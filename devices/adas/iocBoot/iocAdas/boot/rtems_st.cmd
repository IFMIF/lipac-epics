#******************************************************************************
#
# "@(#) $Id: rtems_st.cmd 23 2013-03-13 15:38:34Z lussi $" 
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  09/07/08   created
#
#******************************************************************************
#
# The following is needed if your board support package doesn't at boot
# time automatically cd to the directory containing its startup script
#
cd "/home/epicsmgr/EPICS/support/adas/iocBoot/iocAdas/boot"
#
#-----------------------------------------------------------------
# Set prompt and IOC name
#
epicsEnvSet("IOCSH_PS1", "sigvm20> ")
epicsEnvSet("IOC_NAME", "sigvm20")
#
#-----------------------------------------------------------------
# Set environment variables
#
< envPaths
#
#-----------------------------------------------------------------
# Register all support components
#
cd ${TOP}
dbLoadDatabase("dbd/iocAdas.dbd","","")
iocAdas_registerRecordDeviceDriver(pdbbase)
#
#-----------------------------------------------------------------
# Load record instances
#
cd ./iocBoot/iocAdas/boot
dbLoadTemplate("icv150.substitutions")
dbLoadTemplate("icv196.substitutions")
dbLoadTemplate("icv296.substitutions")
dbLoadTemplate("icv714.substitutions")
#
# to test icv150 error detection
#dbLoadTemplate("icv150Errors.substitutions")
#
# to test icv196 error detection
#dbLoadTemplate("icv196Errors.substitutions")
#
# to test icv196 longin/longout
#cd ${TOP}
#dbLoadRecords("db/icv196long.db","")
#
#-----------------------------------------------------------------
# IOC initialization
#
var devIcv196Verbose 1
var devIcv296Verbose 1
var devIcv150Verbose 1
var devIcv714Verbose 1
#
# 14 bit ADC's
icv150CfgAdc(0,14)
#
iocInit()
#
#-----------------------------------------------------------------
# END of startup script
