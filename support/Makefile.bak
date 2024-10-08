# Makefile to compile all the EPICS support modules
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2024-03-26

# ---------------------------------------------------------
# EPICS base
# Empty target, we just include it here so that other targets can reference it
# ---------------------------------------------------------

base:
pcre:

# ---------------------------------------------------------
# Subdirs to build
# We need to specify the dependencies so that Make can resolve the build order
# The dependencies are manually extracted from */configure/RELEASE
# ---------------------------------------------------------

adas: base
autosave: base
asyn: base calc ipac seq sscan
busy: base asyn autosave
calc: base sscan seq
ipac: base
lua: base asyn
modbus: base asyn
motor: base asyn seq busy ipac modbus lua #MX
pcas: base
s7plc: base seq
seq: base
sscan: base seq
StreamDevice: base asyn calc pcre
symb: base

# ---------------------------------------------------------
# Build instructions
# ---------------------------------------------------------

TOPTARGETS := all distclean

SUBDIRS := adas autosave asyn busy calc ipac lua modbus motor pcas pcre s7plc seq sscan StreamDevice symb

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@echo "\033[31;1m"
	@echo "--------------------------------------------------------------------------------"
	@echo "Building support/$@"
	@echo "--------------------------------------------------------------------------------"
	@echo "\033[0m"
	$(MAKE) -C $@ $(MAKECMDGOALS) INSTALL_LOCATION=$(EPICS_TARGET)/support/$@

.PHONY: $(TOPTARGETS) $(SUBDIRS)
