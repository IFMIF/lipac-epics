#!/bin/bash

# Abort the whole script if any step fails
set -eo pipefail

# Ensure that we are root
if [ "$EUID" -ne 0 ]; then
	echo "Please run as root"
	exit
fi

# Add additional required repos
dnf install -y epel-release
/usr/bin/crb enable
dnf makecache

# Basic compiler stuff
dnf -y install \
	gcc \
	gcc-c++ \
	git \
	make

# Libraries
dnf -y install \
	readline \
	readline-devel \
	pcre \
	pcre-devel \
	libtirpc \
	libtirpc-devel \
	libusbx \
	libusbx-devel \
	libevent-devel \
	perl-lib \
	perl-File-Find \
	perl-FindBin \
	perl-ExtUtils-Command

# Additional required tools
dnf -y install \
	re2c \
	rpcgen \
	doxygen
