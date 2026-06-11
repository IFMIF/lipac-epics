#!/bin/bash

echo '--------------------------------------------------------------------------------'
echo 'Initializing submodules'
echo '--------------------------------------------------------------------------------'
echo
git submodule deinit -f --all
git submodule update --init --recursive

echo
echo '--------------------------------------------------------------------------------'
echo 'Updating submodules'
echo '--------------------------------------------------------------------------------'
echo
git submodule update --remote --merge
