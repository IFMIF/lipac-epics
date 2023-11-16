# Makefile to compile all the EPICS extensions
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Date: 2023-11-17

TOPTARGETS := all clean

SUBDIRS := $(wildcard */.)

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
