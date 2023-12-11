#!/bin/bash

# Basic compiler stuff
sudo dnf -y install \
	gcc \
	gcc-c++ \
	git \
	make

# Libraries
sudo dnf -y install \
	readline \
	readline-devel \
	pcre \
	pcre-devel \
	libtirpc \
	libtirpc-devel \
	perl-lib \
	perl-File-Find \
	perl-FindBin \
	perl-ExtUtils-Command

# Additional required tools
sudo dnf -y install \
	re2c \
	rpcgen \
	doxygen
