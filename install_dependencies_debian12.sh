#!/bin/bash

# Abort the whole script if any step fails
set -eo pipefail

# Ensure that we are root
if [ "$EUID" -ne 0 ]; then
        echo "Please run as root"
        exit
fi

# Basic compiler stuff
apt -y install \
	gcc \
	g++ \
	git \
	make

# Libraries
apt -y install \
        libreadline-dev \
        libtirpc-dev \
        libpcre2-dev \
	libpcre3-dev \
	libusb-1.0-0 \
	libusb-1.0-0-dev \
	perl-modules

# Additional required tools
apt -y install \
        re2c \
	rpcsvc-proto \
        doxygen
