#!/bin/bash

set -e

# build 

cd ../build

make
echo "build done"

# copy file to shared
cp bin/odas3519 ../../../shared/
echo "copy done"
