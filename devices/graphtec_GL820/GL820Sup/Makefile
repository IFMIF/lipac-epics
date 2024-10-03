TOP = ..
include $(TOP)/configure/CONFIG

#=======================================

# Build a GL820 as a library for an IOC:
LIBRARY_IOC = GL820

# Library source files
GL820_SRCS += drvGL820.cpp
GL820_SRCS += drvGL820_Cmnd.cpp

# Link with the asyn and base libraries
GL820_LIBS += asyn
GL820_LIBS += $(EPICS_BASE_IOC_LIBS)

# Install .dbd and .db files
DBD += GL820.dbd
DB += GL820.db

# # GL820.dbd will be created and installed
# DBD += GL820.dbd
# GL820_DBD += base.dbd
# GL820_DBD += drvAsynIPPort.dbd



#=======================================

include $(TOP)/configure/RULES
