#******************************************************************************
#
# "@(#) $Id: st.cmd 23 2013-03-13 15:38:34Z lussi $" 
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  12/10/07   created
# ylussign  21/02/08   load icv150 database
# ylussign  09/07/08   added icv714 database
#
#******************************************************************************
#
# The following is needed if your board support package doesn't at boot
# time automatically cd to the directory containing its startup script
#
cd "/home/epicsmgr/EPICS/support/adas/iocBoot/iocAdas/boot"
#
#-----------------------------------------------------------------
# Set environment variables
#
< cdCommands
#
#-----------------------------------------------------------------
# Load application
#
cd topbin
ld < iocAdas.munch
#
#-----------------------------------------------------------------
# Register all support components
#
cd top
dbLoadDatabase("dbd/iocAdas.dbd","","")
iocAdas_registerRecordDeviceDriver(pdbbase)
#
#-----------------------------------------------------------------
# Load record instances
#
cd startup
dbLoadTemplate("icv150.substitutions")
#dbLoadTemplate("icv196.substitutions")
#dbLoadTemplate("icv296.substitutions")
#dbLoadTemplate("icv714.substitutions")
#
# to test icv150 error detection
#dbLoadTemplate("icv150Errors.substitutions")
#
# to test icv196 error detection
#dbLoadTemplate("icv196Errors.substitutions")
#
# to test icv196 longin/longout
#cd top
#dbLoadRecords("db/icv196long.db","")
#
#-----------------------------------------------------------------
# IOC initialization
#
devIcv196Verbose = 1
devIcv296Verbose = 1
devIcv150Verbose = 1
devIcv714Verbose = 1
#
# 14 bit ADC's
#icv150CfgAdc(0,14)
#
iocInit()
#
#-----------------------------------------------------------------
# END of startup script
