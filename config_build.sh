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
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN/toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$SCRIPT_DIR/install -DLIBCONFIG=$SCRIPT_DIR/../libconfig/install -DFFTW3F=$SCRIPT_DIR/../fftw-3.3.10/install -DALSA=$SCRIPT_DIR/../alsa-lib-1.2.7/install -DPULSEAUDIO=$SCRIPT_DIR/../pulseaudio/install/aarch64-linux -DSNDFILE=$SCRIPT_DIR/../libsndfile/install -DHI3519_ROOT=$SCRIPT_DIR/../a55_linux/source/out -DUAC_SRC_SINK=OFF

cd $SCRIPT_DIR/build

# Build the project
make

# Install the project
make install

file $SCRIPT_DIR/install/lib/libodas.so

# Strip sybmols
echo "Stripping symbols"
export STRIP=$TOOLCHAIN/aarch64-v01c01-linux-gnu-gcc/bin/aarch64-linux-gnu-strip
$STRIP --strip-unneeded $SCRIPT_DIR/install/lib/libodas.so 

file $SCRIPT_DIR/install/lib/libodas.so