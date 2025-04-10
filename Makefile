# -------------------------------------------------------------------------------------------------
# Makefile to compile the whole LIPAc EPICS-7.0 distribution
#
# Author: José Franco Campos <franco.jose@qst.go.jp>
# Last update: 2025-04-03
# -------------------------------------------------------------------------------------------------

# Default installing location
EPICS_TARGET = $(CURDIR)/target

# Auxiliary function to print a text enclosed in a red box
# We want to make the compilation log a bit prettier and easier to read
red := $(shell tput setaf 1)
reset := $(shell tput sgr0)

define red-text
@echo "\033[31;1m"
@echo "--------------------------------------------------------------------------------"
@echo "$(1)"
@echo "--------------------------------------------------------------------------------"
@echo "\033[0m"
endef

# -------------------------------------------------------------------------------------------------

# Run the build stages in the correct order
all: prepare base support extensions

# EXPLAIN THIS
prepare:
	$(call red-text,"Preparing the build environment")

	# Create the target directories
	@mkdir -p $(EPICS_TARGET)
	@mkdir -p $(EPICS_TARGET)/support/
	@mkdir -p $(EPICS_TARGET)/extensions/

	# Prepare RELEASE.local
	@rm -f RELEASE.local
	@echo "EPICS_BASE=$(EPICS_TARGET)/base" >> RELEASE.local
	@echo "SUPPORT=$(EPICS_TARGET)/support" >> RELEASE.local
	@echo ""                              >> RELEASE.local
	@cat RELEASE.local.template           >> RELEASE.local

	@cp RELEASE.local support/
	@cp RELEASE.local extensions/

	@cp RELEASE.local $(EPICS_TARGET)
	@cp RELEASE.local $(EPICS_TARGET)/support/
	@cp RELEASE.local $(EPICS_TARGET)/extensions/

	@rm RELEASE.local

	# Prepare CONFIG_SITE.local
	@cp CONFIG_SITE.local support/
	@cp CONFIG_SITE.local extensions/

	@cp CONFIG_SITE.local $(EPICS_TARGET)
	@cp CONFIG_SITE.local $(EPICS_TARGET)/support
	@cp CONFIG_SITE.local $(EPICS_TARGET)/extensions

	# SEQ depends on itself
	# We need to copy its 'configure' folder to the target folder beforehand, or it doesn't compile
	@mkdir -p $(EPICS_TARGET)/support/seq/configure
	@find support/seq/configure/ -maxdepth 1 -type f -exec cp -f -t $(EPICS_TARGET)/support/seq/configure {} +

# base
base:
	$(call red-text,"Building base")
	$(MAKE) all -C base INSTALL_LOCATION=$(EPICS_TARGET)/base EPICS_TARGET=$(EPICS_TARGET)

# support
support:
	$(call red-text,"Building support")
	$(MAKE) all -C support EPICS_TARGET=$(EPICS_TARGET)

# extensions
extensions:
	$(call red-text,"Building extensions")
	$(MAKE) all -C extensions EPICS_TARGET=$(EPICS_TARGET)

# -------------------------------------------------------------------------------------------------

# Clean in reverse order
clean:
	$(call red-text,"Cleaning extensions")
	$(MAKE) distclean -C extensions EPICS_TARGET=$(EPICS_TARGET)

	$(call red-text,"Cleaning support")
	$(MAKE) distclean -C support EPICS_TARGET=$(EPICS_TARGET)

	$(call red-text,"Cleaning target")
	$(MAKE) distclean -C base INSTALL_LOCATION=$(EPICS_TARGET)/base EPICS_TARGET=$(EPICS_TARGET)

	$(call red-text,"Removing target")
	@rm -rf $(EPICS_TARGET)
	@find -name 'RELEASE.local' -delete

# -------------------------------------------------------------------------------------------------

.PHONY: all clean prepare base support extensions

.NOTPARALLEL:
