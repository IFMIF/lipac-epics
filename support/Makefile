# Makefile to compile all the EPICS support modules
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2024-03-26

# ---------------------------------------------------------
# EPICS base
# Empty target, we just include it here so that other targets can reference it
# ---------------------------------------------------------

#base:

# ---------------------------------------------------------
# Subdirs to build
# We need to specify the dependencies so that Make can resolve the build order
# The dependencies are manually extracted from */configure/RELEASE
# ---------------------------------------------------------

autosave:
asyn: calc ipac seq sscan
busy: asyn autosave
calc: sscan seq
iocStats:
ipac:
lua: asyn
modbus: asyn
motor: asyn seq busy ipac modbus lua #MX
pcas:
pvxs:
s7plc: seq
seq:
sscan: seq
std: asyn seq
StreamDevice: asyn calc
symb:

# ---------------------------------------------------------
# Build instructions
# ---------------------------------------------------------

TOPTARGETS := all distclean

SUBDIRS := autosave asyn busy calc iocStats ipac lua modbus motor pcas s7plc seq sscan std StreamDevice symb pvxs

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@echo "\033[31;1m"
	@echo "--------------------------------------------------------------------------------"
	@echo "Building support/$@"
	@echo "--------------------------------------------------------------------------------"
	@echo "\033[0m"
	$(MAKE) -C $@ $(MAKECMDGOALS) INSTALL_LOCATION=$(EPICS_TARGET)/support/$@

.PHONY: $(TOPTARGETS) $(SUBDIRS)

.NOTPARALLEL:
