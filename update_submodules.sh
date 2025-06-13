#!/bin/bash

git submodule deinit -f --all
git submodule update --init --recursive
#git submodule update --remote --merge
