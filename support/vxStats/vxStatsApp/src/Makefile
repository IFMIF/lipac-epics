# C.E.A. IRFU/SIS/LDII
#
# @(#) $Id: Makefile,v 1.2 2010/10/27 12:09:14 cmsmgr Exp $
#
# Makefile
#

TOP=../..
include $(TOP)/configure/CONFIG

#=======================================================
# Build the support library vxStats
# and install it in <top>/lib

# Build the library for vxWorks only
LIBRARY_IOC_vxWorks += vxStats

# Source files
vxStats_SRCS += vxStats.c
vxStats_SRCS += vxStatsResetSub.c

#=======================================================
# Install vxStatsSupport.dbd and vxStatsInclude.dbd in <top>/dbd

DBD += vxStatsSupport.dbd
DBD += vxStatsInclude.dbd

#=======================================================
# Build and install vxStats.dbd in <top>/dbd
# vxStats.dbd derives from vxStatsInclude.dbd

DBD += vxStats.dbd

include $(TOP)/configure/RULES

#=======================================================
# Make man pages and html documentation with doxygen
# and install application launchers on desktop 

install: doxygen

doxygen:
	@if [ -f doxygen.cfg ] ; then \
	echo "making man & html documentation"; \
	doxygen doxygen.cfg ; \
	fi
