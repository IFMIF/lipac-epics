#!/bin/bash

# Basic compiler stuff
sudo apt -y install \
	gcc \
	g++ \
	git \
	make

# Libraries
sudo apt -y install \
        libreadline-dev \
        libtirpc-dev \
        libpcre2-dev \
	libpcre3-dev \
	perl-modules

# Additional required tools
sudo apt -y install \
        re2c \
	rpcsvc-proto \
        doxygen
