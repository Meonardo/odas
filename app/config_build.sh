#!/bin/bash

set -e

SCRIPT_DIR=$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")
echo "Current dir: $SCRIPT_DIR"

# Create the build directory & install directory
rm -fr $SCRIPT_DIR/build
mkdir -p $SCRIPT_DIR/build

# set the toolchain.cmake
TOOLCHAIN=/home/meonardo/Hi3519/gcc-20231123-aarch64-v01c01-linux-gnu

# Run CMake
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN/toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DHI3519_ROOT=$SCRIPT_DIR/../../smp/a55_linux/source/out 

cd $SCRIPT_DIR/build

# Build the project
make 