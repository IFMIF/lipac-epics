# Makefile to compile the whole LIPAc EPICS-7.0 distribution
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Date: 2023-12-18

# ---------------------------------------------------------
# Build base first, then support, then extensions
# ---------------------------------------------------------

base:
support: base
extensions: base support

# ---------------------------------------------------------
# Build instructions
# ---------------------------------------------------------

TOPTARGETS := all distclean

SUBDIRS := base support extensions

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
