# EPICS 7.0 distribution for LIPAc

## Introduction

This repository is intended to be the official EPICS 7.0 distribution used in the LIPAc project. It currently includes the EPICS base repository, and a collection of support modules and extensions. Whenever possible, all the repositories are mirrored from the official sources, and stored under https://code.ifmif.org/mirror/

    ./
    ├-- base       -> EPICS 7.0 itself
    ├-- support    -> EPICS support modules
    └-- extensions -> EPICS extensions

This repository includes most folders as submodules, so when cloning it please make sure to do a recursive clone:

    git clone --recursive https://code.ifmif.org/lipac/epics-7.0

## Compilation instructions

Forewarning: These instructions have been tested under AlmaLinux 9 and Debian 12 and confirmed to work. They will probably work under other Linux distributions, but we can't confirm it. They won't probably work under CentOS 7, as it is very old and missing some required dependencies.

Before the code can be compiled, it is necessary to install all the required dependencies. The specific name of the packages depend on the Linux distro being used:

- For AlmaLinux 9 please run [`install_dependencies_al9.sh`](install_dependencies_al9.sh)
- For Debian 12 please run [`install_dependencies_debian12.sh`](install_dependencies_debian12.sh)

Compiling EPICS requires, at a minimum, configuring the `$EPICS_BASE` and `$EPICS_HOST_ARCH` environment variables. According to the LIPAc CSP, EPICS is stored under `/home/epicsmgr`, so we assume the following configuration:

    export EPICS_ROOT=/home/epicsmgr/epics-7.0
    export EPICS_BASE=/home/epicsmgr/epics-7.0/base
    export EPICS_HOST_ARCH=linux-x86_64

The file [`set_epics_environment.sh`](set_epics_environment.sh) contains an example.

To run a full cloning and compilation:

    git clone --recursive https://code.ifmif.org/lipac/epics-7.0 ${EPICS_ROOT}
    source ${EPICS_ROOT}/set_epics_environment.sh
    cd ${EPICS_ROOT}
    make all -j4

To clean the project completely:

    cd ${EPICS_ROOT}
    make distclean

## Obsolete modules

The following support modules that were used in the old EPICS 3.14 distribution used in LIPAc are missing, as they are not compatible with EPICS 7.0 and they have been replaced by newer modules:

- genSub -> asub
- stream -> StreamDevice

## Future work

We will add additional support modules and extensions as needed. 
