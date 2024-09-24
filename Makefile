# -------------------------------------------------------------------------------------------------
# Makefile to compile the whole LIPAc EPICS-7.0 distribution
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2024-03-26
# -------------------------------------------------------------------------------------------------

red := $(shell tput setaf 1)
reset := $(shell tput sgr0)

EPICS_TARGET = $(CURDIR)/target

# Auxiliary function to print a text enclosed in a red box
# We want to make the compilation log a bit prettier and easier to read
define red-text
@echo "\033[31;1m"
@echo "--------------------------------------------------------------------------------"
@echo "$(1)"
@echo "--------------------------------------------------------------------------------"
@echo "\033[0m"
endef

# EXPLAIN THIS
prepare:
	$(call red-text,"Preparing the build environment")

	# Create the target directories
	@mkdir -p $(EPICS_TARGET)
	@mkdir -p $(EPICS_TARGET)/support/
	@mkdir -p $(EPICS_TARGET)/devices/
	@mkdir -p $(EPICS_TARGET)/extensions/

	# Prepare RELEASE.local
	@rm -f RELEASE.local
	@echo "EPICS_BASE=$(EPICS_TARGET)/base" >> RELEASE.local
	@echo "SUPPORT=$(EPICS_TARGET)/support" >> RELEASE.local
	@echo "DEVICES=$(EPICS_TARGET)/devices" >> RELEASE.local
	@echo ""                              >> RELEASE.local
	@cat RELEASE.local.template           >> RELEASE.local

	@cp RELEASE.local support/
	@cp RELEASE.local devices/
	@cp RELEASE.local extensions/

	@cp RELEASE.local $(EPICS_TARGET)
	@cp RELEASE.local $(EPICS_TARGET)/support/
	@cp RELEASE.local $(EPICS_TARGET)/devices/
	@cp RELEASE.local $(EPICS_TARGET)/extensions/

	@rm RELEASE.local

	# Prepare CONFIG_SITE.local
	@cp CONFIG_SITE.local support/
	@cp CONFIG_SITE.local devices/
	@cp CONFIG_SITE.local extensions/

	@cp CONFIG_SITE.local $(EPICS_TARGET)
	@cp CONFIG_SITE.local $(EPICS_TARGET)/support
	@cp CONFIG_SITE.local $(EPICS_TARGET)/devices
	@cp CONFIG_SITE.local $(EPICS_TARGET)/extensions

	# SEQ depends on itself
	# We need to copy its 'configure' folder to the target folder beforehand, or it doesn't compile
	@mkdir -p $(EPICS_TARGET)/support/seq/configure
	@find support/seq/configure/ -maxdepth 1 -type f -exec cp -f -t $(EPICS_TARGET)/support/seq/configure {} +

# Build base first, then support, then extensions
build: prepare
	# base
	$(call red-text,"Building base")
	$(MAKE) -C base all INSTALL_LOCATION=$(EPICS_TARGET)/base EPICS_TARGET=$(EPICS_TARGET)

	# support
	$(call red-text,"Building support")
	$(MAKE) -C support all EPICS_TARGET=$(EPICS_TARGET)

	# devices
	$(call red-text,"Building additional device support")
	$(MAKE) -C devices all EPICS_TARGET=$(EPICS_TARGET)

	# extensions
	$(call red-text,"Building extensions")
	$(MAKE) -C extensions all EPICS_TARGET=$(EPICS_TARGET)

# Clean in reverse order
clean:
	$(call red-text,"Cleaning extensions")
	$(MAKE) -C extensions distclean EPICS_TARGET=$(EPICS_TARGET)

	$(call red-text,"Cleaning support")
	$(MAKE) -C support distclean EPICS_TARGET=$(EPICS_TARGET)

	$(call red-text,"Cleaning devices")
	$(MAKE) -C devices distclean EPICS_TARGET=$(EPICS_TARGET)

	$(call red-text,"Cleaning target")
	$(MAKE) -C base distclean INSTALL_LOCATION=$(EPICS_TARGET)/base EPICS_TARGET=$(EPICS_TARGET)

	$(call red-text,"Removing target")
	@rm -rf $(EPICS_TARGET)
	@rm -f RELEASE.local support/RELEASE.local extensions/RELEASE.local

.PHONY: prepare build clean
