#******************************************************************************
#
# "@(#) $Id: Makefile 23 2013-03-13 15:38:34Z lussi $" 
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  09/10/07   created
#
#******************************************************************************

#Makefile at top of application tree
TOP = .

include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), adasApp)
DIRS := $(DIRS) $(filter-out $(DIRS), iocBoot)

include $(TOP)/configure/RULES_TOP
