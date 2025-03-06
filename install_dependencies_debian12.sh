#!/bin/bash

# Abort the whole script if any step fails
set -eo pipefail

# Ensure that we are root
if [ "$EUID" -ne 0 ]; then
        echo "Please run as root"
        exit
fi

# First, update the package cache
apt update

# Libraries and tools required to compile EPICS
apt -y install \
	gcc \
	g++ \
	git \
	make \
        libreadline-dev \
        libtirpc-dev \
        libpcre2-dev \
	libpcre3-dev \
	libusb-1.0-0 \
	libusb-1.0-0-dev \
	libevent-dev \
	perl-modules \
        re2c \
	rpcsvc-proto \
        doxygen
