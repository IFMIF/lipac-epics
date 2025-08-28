#!../bin/linux-x86_64/uber_ioc

## Load the RELEASE file with the location of all the support modules.
< envPaths

## Override some environment variables with the correct values.
epicsEnvSet("IOC", "uber_ioc")

## Register all support components.
cd $(TOP)/dbd
dbLoadDatabase("uber_ioc.dbd")
uber_ioc_registerRecordDeviceDriver(pdbbase)

## Load record instances. Sometimes DB files contain relative references to other files,
## so it is safer to jump into the DB folder, load the files, and then go back to the original folder.
cd $(TOP)/db
dbLoadRecords("ioc.db","IOCNAME=$(IOC),TODFORMAT=%Y/%m/%d %H:%M:%S")
#dbLoadRecords("uber_ioc.db","P=$(P),R=$(R)")

## Start the IOC
cd $(TOP)/boot
iocInit()

## Start any sequence programs
# dbpf $(P):START 1
