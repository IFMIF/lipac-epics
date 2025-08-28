# -------------------------------------------------------------------------------------------------
# Build envPaths from RELEASE.local
#
# This file will containing the location of EPICS base, all the EPICS support modules, plus:
#	IOC -> name of the IOC itself.
#	TOP -> installation folder of the IOC.
#
# Unfortunately, the 'standard' generation procedure is quite broken. Like the rest of the EPICS
# build system, it relies on a lot of arcane macro dark magic and a rigid folder structure that
# doesn't reflect modern development practices. For this reason we have written our own Makefile,
# which calls convertRelease.pl directly, patches the result, and copies all the relevant files
# to the target folder.
# -------------------------------------------------------------------------------------------------

TOP = ..
include $(TOP)/configure/CONFIG

install:
	perl -CSD $(EPICS_BASE)/bin/$(EPICS_HOST_ARCH)/convertRelease.pl -T $(TOP) envPaths
	sed -i 's!epicsEnvSet("TOP","..")!epicsEnvSet("TOP","$(INSTALL_LOCATION)")!g' ./envPaths

	@mkdir -p $(INSTALL_LOCATION)/boot/
	@cp README.md envPaths *.cmd $(INSTALL_LOCATION)/boot/
	@chmod +x $(INSTALL_LOCATION)/boot/*.cmd

clean:
	rm -f envPaths

realclean:
	rm -f envPaths
