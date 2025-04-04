# Makefile to compile all the EPICS extensions
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2024-03-26

# ---------------------------------------------------------
# Build instructions
# ---------------------------------------------------------

TOPTARGETS := all distclean

SUBDIRS := $(wildcard */.)

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@echo "\033[31;1m"
	@echo "--------------------------------------------------------------------------------"
	@echo "Building extensions/$@"
	@echo "--------------------------------------------------------------------------------"
	@echo "\033[0m"

	$(MAKE) -C $@ $(MAKECMDGOALS) INSTALL_LOCATION=$(EPICS_TARGET)/extensions/$@

.PHONY: $(TOPTARGETS) $(SUBDIRS)

.NOTPARALLEL:
