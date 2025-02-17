#******************************************************************************
#
# $Id: Makefile 23 2013-03-13 15:38:34Z lussi $
#
# who       when       what
# --------  --------   ----------------------------------------------
# ylussign  09/10/07   created
#
#******************************************************************************

TOP=../..

include $(TOP)/configure/CONFIG

#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this

DB += icv150.template
DB += icv714.template
DB += BI.template
DB += BO.template
DB += MBBI.template
DB += MBBO.template
DB += MBBIDIRECT.template
DB += MBBODIRECT.template
DB += icv196long.db
DB += icv296long.db

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

