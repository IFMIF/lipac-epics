#******************************************************************************
# CEA - Direction des Sciences de la Matiere - IRFU/SIS
# CE-SACLAY, 91191 Gif-sur-Yvette, France
#
# $Id: Makefile 23 2013-03-13 15:38:34Z lussi $
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  09/10/07   created
# ylussign  09/11/07   added devIcv296.c
# ylussign  21/02/08   commented man pages section
# ylussign  07/07/08   removed drvIcv714.c
# ylussign  09/07/08   adas library built for vxWorks & RTEMS
# ylussign  02/12/08   added application test196
# ylussign  02/12/08   removed adas_LIBS
# ylussign  24/11/09   added target doxygen
# ylussign  18/02/10   added flag -O0 (do not optimize)
#
#******************************************************************************

TOP=../..

include $(TOP)/configure/CONFIG

#========================================
# Build IOC support library for ADAS boards

USR_CFLAGS += -fno-strict-aliasing -O0

LIBRARY_IOC_vxWorks = adas
LIBRARY_IOC_RTEMS = adas
adas_SRCS += devIcv150.c
adas_SRCS += devIcv196.c
adas_SRCS += devIcv296.c
adas_SRCS += devIcv714.c

#========================================
# Build IOC test application for icv196

PROD_IOC_vxWorks = test196
test196_SRCS += test196.c

#========================================
# Install public include files into <top>/include

INC += icv150.h

#========================================
# Install devAdas.dbd into <top>/dbd

DBD += devAdas.dbd

include $(TOP)/configure/RULES

#=======================================================
# Make man pages and html documentation with doxygen

install: doxygen
doxygen:
	@if [ -f doxygen.cfg ] ; then doxygen doxygen.cfg ; fi

