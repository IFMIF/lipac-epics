# Makefile to compile all the EPICS support modules
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2024-03-26

# ---------------------------------------------------------
# EPICS base
# Empty target, we just include it here so that other targets can reference it
# ---------------------------------------------------------

base:

# ---------------------------------------------------------
# Subdirs to build
# We need to specify the dependencies so that Make can resolve the build order
# The dependencies are manually extracted from */configure/RELEASE
# ---------------------------------------------------------

#symb: base
agilent_e364xa: base
#SNCSEQ = $(SUPPORT)/seq-2.2.8
#STREAM = $(SUPPORT)/StreamDevice-2.8.16
#ASYN =  $(SUPPORT)/asyn-4.44.2
#CALC=$(SUPPORT)/calc-3.7.4

# ---------------------------------------------------------
# Build instructions
# ---------------------------------------------------------

TOPTARGETS := all distclean

SUBDIRS := agilent_e364xa

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@echo "\033[31;1m"
	@echo "--------------------------------------------------------------------------------"
	@echo "Building devices/$@"
	@echo "--------------------------------------------------------------------------------"
	@echo "\033[0m"
	$(MAKE) -C $@ $(MAKECMDGOALS) INSTALL_LOCATION=$(EPICS_TARGET)/devices/$@

.PHONY: $(TOPTARGETS) $(SUBDIRS)
