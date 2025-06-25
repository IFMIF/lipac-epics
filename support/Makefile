# ---------------------------------------------------------
# Makefile to compile all the EPICS support modules
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2025-06-25
# ---------------------------------------------------------

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
opcua: pcas
pcas:
pvxs:
s7plc: seq
seq:
sscan: seq
std: asyn seq
StreamDevice: asyn calc

# ---------------------------------------------------------
# Build instructions
# ---------------------------------------------------------

TOPTARGETS := all distclean

#SUBDIRS := $(wildcard */.)
SUBDIRS := autosave asyn busy calc iocStats ipac lua modbus motor opcua pcas pvxs s7plc seq sscan std StreamDevice

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
