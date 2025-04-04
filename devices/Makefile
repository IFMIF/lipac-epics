# -------------------------------------------------------------------------------------------------
# Device support modules
# This modules are fully independent of each other, they only depend on 'base' and 'support'
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2024-03-26
# -------------------------------------------------------------------------------------------------

TOPTARGETS := all distclean

# Uncomment the correct device support.
SUBDIRS := adas
SUBDIRS := agilent_e364xa
#SUBDIRS := caen_ad1725s
SUBDIRS += caen_n1470
SUBDIRS += graphtec_GL820
SUBDIRS += keysight_B2900
SUBDIRS += srs_DG645

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@echo "\033[31;1m"
	@echo "--------------------------------------------------------------------------------"
	@echo "Building devices/$@"
	@echo "--------------------------------------------------------------------------------"
	@echo "\033[0m"
	$(MAKE) -C $@ $(MAKECMDGOALS) INSTALL_LOCATION=$(EPICS_TARGET)/devices/$@

.PHONY: $(TOPTARGETS) $(SUBDIRS)

.NOTPARALLEL:
