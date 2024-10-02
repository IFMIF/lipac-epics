#!/opt/epics-7.0/devices/agilent_e364xa/bin/linux-x86_64/e364xaTest
# the first line might need to be updated to fetch from the correct direction
# Save the current directory for later usage
epicsEnvSet("CURRENT_DIR", ${PWD})
#< envPaths

epicsEnvSet(ETHER,"IP.to.your.device:port")
#epicsEnvSet(TTY,"/dev/tty01")     # ColdFire uCdimm 5282
#epicsEnvSet(TTY,"/dev/ttyS0")    # Linux
epicsEnvSet(P, "e364xa:")
epicsEnvSet(R, "")
epicsEnvSet(SIMM, "NO")


## Register all support components
cd ..
dbLoadDatabase("dbd/e364xaTest.dbd", 0, 0)
e364xaTest_registerRecordDeviceDriver(pdbbase)

## Set up IOC/hardware links -- Remote serial port
##  (link, host, priority, noAutoConnect, noEosProcessing)
drvAsynIPPortConfigure("L0", "$(ETHER)", 0, 0, 0)
asynOctetSetInputEos("L0", 0, "\n")
asynOctetSetOutputEos("L0", 0, "\n")

## Set up IOC/hardware links -- Local serial port
##  (link, ttyName, priority, noAutoConnect, noEosProcessing)
#drvAsynSerialPortConfigure("L0", "$(TTY)", 0, 0, 0)
#asynOctetSetInputEos("L0", 0, "\n")
#asynOctetSetOutputEos("L0", 0, "\n")
#asynSetOption("L0", 0, "baud", "9600")
#asynSetOption("L0", 0, "bits", "8")
#asynSetOption("L0", 0, "parity", "none")
#asynSetOption("L0", 0, "stop", "2")
#asynSetOption("L0", 0, "clocal", "Y")
#asynSetOption("L0", 0, "crtscts", "Y")

## Turn on all driver I/O messages
# asynSetTraceMask("L0", -1, 0x9)
# asynSetTraceIOMask("L0", -1, 0x2)

## Load record instances
dbLoadRecords("db/deve364xa.db", "P=$(P),R=$(R),L=0,A=-1,SIMM=$(SIMM)")
dbLoadRecords("db/asynRecord.db", "P=$(P),R=asyn,PORT=L0,ADDR=-1,OMAX=80,IMAX=80")

# Start the IOC control loop
cd ${CURRENT_DIR}
iocInit()

epicsThreadSleep(2)
# dbpf e364xachk:IDN.PROC 1 ## not needed anymore with the PINI field