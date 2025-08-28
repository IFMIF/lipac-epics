# -------------------------------------------------------------------------------------------------
# Build an IOC application
# -------------------------------------------------------------------------------------------------

TOP = ..
include $(TOP)/configure/CONFIG

# Name of the IOC
PROD_IOC = uber_ioc

# Database definition files
# We need to add all the support modules in the correct order.
# The modules are organized in tiers; modules belongin to a tier only depend on previous tiers.
DBD += uber_ioc.dbd

# Tier 0: base
uber_ioc_DBD += base.dbd

# Tier 1: autosave, iocStats, ipac, pcas (!), pvxs, seq (!)
uber_ioc_DBD += asSupport.dbd
uber_ioc_DBD += devIocStats.dbd
uber_ioc_DBD += drvIpac.dbd
uber_ioc_DBD += pvxsIoc.dbd

# Tier 2: opcua, s7plc, sscan
uber_ioc_DBD += opcua.dbd
uber_ioc_DBD += s7plc.dbd
uber_ioc_DBD += sscanSupport.dbd

# Tier 3: calc
uber_ioc_DBD += calc.dbd
uber_ioc_DBD += sCalcoutRecord.dbd

# Tier 4: asyn
uber_ioc_DBD += asyn.dbd
uber_ioc_DBD += drvAsynIPPort.dbd
uber_ioc_DBD += drvAsynSerialPort.dbd

# Tier 5: busy, lua, modbus, std, StreamDevice
uber_ioc_DBD += busySupport.dbd
uber_ioc_DBD += luaSupport.dbd
uber_ioc_DBD += modbusSupport.dbd
uber_ioc_DBD += stdSupport.dbd
uber_ioc_DBD += stream.dbd

# Tier 6: motor

# Source files
# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
uber_ioc_SRCS += uber_ioc_registerRecordDeviceDriver.cpp
uber_ioc_SRCS_DEFAULT += main.cpp

# asyn
uber_ioc_LIBS += asyn
# autosave
uber_ioc_LIBS += autosave
# busy
uber_ioc_LIBS += busy
# calc
uber_ioc_LIBS += calc
# iocStats
uber_ioc_LIBS += devIocStats
# ipac
uber_ioc_LIBS += Ipac
# lua
uber_ioc_LIBS += lua
# modbus
uber_ioc_LIBS += modbus
# motor (MORE STUFF HERE!)
uber_ioc_LIBS += motor
# opcua
uber_ioc_LIBS += opcua
# pcas
uber_ioc_LIBS += cas gdd
# pvxs
uber_ioc_LIBS += pvxs pvxsIoc
# s7plc
uber_ioc_LIBS += s7plc
# seq (MORE STUFF HERE!)
uber_ioc_LIBS += seq
# sscan
uber_ioc_LIBS += sscan scanProgress
# std
uber_ioc_LIBS += std
# StreamDevice
uber_ioc_LIBS += stream

# Finally include the libraries from base
uber_ioc_LIBS += $(EPICS_BASE_IOC_LIBS)
$(info Linking with ${uber_ioc_LIBS})

# EPICS build rules
include $(TOP)/configure/RULES

#  ADD CUSTOM RULES AFTER THIS LINE
USR_CXXFLAGS += -Wno-deprecated -Werror
