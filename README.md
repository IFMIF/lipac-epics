# EPICS 7.0 distribution for LIPAc

## Introduction

This repository is intended to be the official EPICS 7.0 distribution used in the LIPAc project. It currently includes the EPICS base repository, and a collection of support modules and extensions. Whenever possible, all the repositories are mirrored from the official sources, and stored under https://code.ifmif.org/mirror/

    ./
    ├-- base       -> EPICS 7.0 itself
    ├-- support    -> EPICS support modules
    └-- extensions -> EPICS extensions

This repository includes most folders as submodules, so when cloning it please make sure to do a recursive clone:

    git clone --recursive https://code.ifmif.org/lipac/lipac-epics-7.0

## Compilation instructions

Forewarning: These instructions have been tested under AlmaLinux 9 and confirmed to work. They will probably work under other Linux distributions, but we can't confirm it. They won't probably work under CentOS 7, as it is very old and missing some required dependencies.

Before you can compile the code, you need to install all the required dependencies. In a moder RHEL-based system, run the following command:

    dnf install \
	git \
	make \
	gcc \
	gcc-c++ \
	readline \
	readline-devel \
	perl-lib \
	perl-File-Find \
	perl-FindBin \
	re2c \
	rpcgen \
	libtirpc \
	libtirpc-devel \
	perl-ExtUtils-Command \
	pcre \
	pcre-devel \
	doxygen

Compiling EPICS requires, at a minimum, configuring the `$EPICS_BASE` and `$EPICS_HOST_ARCH` environment variables. According to the LIPAc CSP, EPICS is stored under `/home/epicsmgr`, so we assume the following configuration:

    export EPICS_ROOT=/home/epicsmgr/epics-7.0
    export EPICS_BASE=/home/epicsmgr/epics-7.0/base
    export EPICS_HOST_ARCH=linux-x86_64

The file [`set_epics_environment.sh`](set_epics_environment.sh) contains an example.

To run a full cloning and compilation:

    git clone --recursive https://code.ifmif.org/lipac/lipac-epics-7.0 ${EPICS_ROOT}
    source ${EPICS_ROOT}/set_epics_environment.sh
    cd ${EPICS_ROOT}/base
    make -j4
    cd ${EPICS_ROOT}/support
    make -j4 all
    cd ${EPICS_ROOT}/extensions
    make -j4

## Future work

We will add additional support modules and extensions as needed. 
