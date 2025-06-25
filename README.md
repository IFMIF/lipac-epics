# EPICS 7.0 distribution for LIPAc

## Introduction

This repository is intended to be the official EPICS 7.0 distribution used in the LIPAc project. It currently includes the EPICS base repository, and a collection of support modules and extensions. The dependencies are included as submodules, so when cloning it please make sure to do a recursive clone:

    git clone --recursive https://github.com/IFMIF/lipac-epics-7.0

After cloning, you should have the following folder structure:

    ./
    ├-- base       -> EPICS 7.0 itself
    ├-- support    -> EPICS support modules
    ├-- devices    -> Additional device support modules
    └-- extensions -> EPICS extensions

To update the submodules after the initial clone:

    git submodule update --init --recursive

## Compilation instructions

Forewarning: These instructions have been tested under AlmaLinux 9 and Debian 12 and confirmed to work. They will probably work under other Linux distributions, but we can't confirm it. They won't probably work under CentOS 7, as it is very old and missing some required dependencies.

Before the code can be compiled, it is necessary to install all the required dependencies. The specific name of the packages depend on the Linux distro being used:

- For AlmaLinux 9 please run [`install_dependencies_al9.sh`](install_dependencies_al9.sh)
- For Debian 12 please run [`install_dependencies_debian12.sh`](install_dependencies_debian12.sh)

To compile the OPC-UA support you need [open62541](https://www.open62541.org/). A prebuilt package is provided in the `deps` folder, just unpack it under `/opt`.

The EPICS build process is quite complicated:

- EPICS base itself is easy to build, but the support modules and the extensions require providing a hardcoded file with the location of 'base' and the other support modules. Futhermore, there is no dependency resolution for the support modules.
- The EPICS build process doesn't allow for clean out-of-source builds. Instead, all the compilation artifacts are generated alongside the code, and the relevant artifacts are copied to the install location. 

To solve these problems, we have written our own Makefile, which provides two goals and one variable:

- build: compile EPICS base and all the support modules and extensions.
- clean: delete all the intermediate files and compilation artifacts.
- EPICS_TARGET: target directory where the resulting EPICS distribution will be installed. If not provided, it defaults to ./target

To build the EPICS distribution, please run:

    make EPICS_TARGET=${target_dir} -j${num_cores}

To clean the project completely, please run:

    make clean EPICS_TARGET=${target_dir}

Please note that this command will also delete the target directory, so please back it up first if you want to keep Fit!

## Updating and fixing submodules

To update your submodules to the latest upstream version, please run:

    git submodule update --init --recursive

If you can't compile the project, you might have accidentally broken your submodules. In that case, the script `clean_submodules.sh` will delete and recreate them, to start from a clean slate.

## Using the compiled distribution to develop your IOCs

WRITE THIS

## Obsolete modules

The following support modules that were used in the old EPICS 3.14 distribution used in LIPAc are missing, as they are not compatible with EPICS 7.0 and they have been replaced by newer modules:

- genSub -> asub
- stream -> StreamDevice

## Future work

We will add additional support modules and extensions as needed. 
