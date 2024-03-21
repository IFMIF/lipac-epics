#!/bin/bash

export EPICS_ROOT=/home/epicsmgr/epics-7.0

export EPICS_BASE=${EPICS_ROOT}/base
export EPICS_HOST_ARCH=`${EPICS_BASE}/startup/EpicsHostArch`
export EPICS_BASE_BIN=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}

export EPICS_EXTENSIONS=${EPICS_ROOT}/extensions
export EPICS_EXTENSIONS_BIN=${EPICS_EXTENSIONS}/bin/${EPICS_HOST_ARCH}

export PATH=${PATH}:${EPICS_BASE_BIN}:${EPICS_EXTENSIONS_BIN}
