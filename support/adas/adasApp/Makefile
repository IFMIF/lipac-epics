#******************************************************************************
#
# $Id: Makefile 23 2013-03-13 15:38:34Z lussi $
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  09/10/07   created
# ylussign  24/11/09   replaced adl by gui, added target clean
#
#******************************************************************************

TOP = ..

include $(TOP)/configure/CONFIG

DIRS += src
DIRS += db
DIRS += gui

include $(TOP)/configure/RULES_DIRS

clean:
	@echo "deleting html & man"
	@$(RMDIR) html man 
