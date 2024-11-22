#!/bin/bash
SCRIPT_DIR=$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")
echo "Current dir: $SCRIPT_DIR"

# Create the build directory & install directory
rm -fr $SCRIPT_DIR/build
mkdir -p $SCRIPT_DIR/build

# set the toolchain.cmake
TOOLCHAIN=/home/meonardo/Hi3519/gcc-20231123-aarch64-v01c01-linux-gnu

export PKG_CONFIG_PATH=$SCRIPT_DIR/../alsa-lib/install/lib/pkgconfig:$SCRIPT_DIR/../fftw3/install/lib/pkgconfig:$SCRIPT_DIR/../pulseaudio/install/aarch64-linux/lib/pkgconfig:$PKG_CONFIG_PATH
echo $PKG_CONFIG_PATH

# Run CMake
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN/toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$SCRIPT_DIR/install -DLIBCONFIG=$SCRIPT_DIR/../libconfig/install -DFFTW3=$SCRIPT_DIR/../fftw3/install -DFFTW3F=$SCRIPT_DIR/../fftw-3.3.10/install -DALSA=$SCRIPT_DIR/../alsa-lib-1.2.7/install -DPULSEAUDIO=$SCRIPT_DIR/../pulseaudio/install/aarch64-linux -DSNDFILE=$SCRIPT_DIR/../libsndfile/install

cd $SCRIPT_DIR/build

# Build the project
make

# Install the project
make install

# # Strip sybmols
# echo "Stripping symbols"
# export STRIP=$TOOLCHAIN/aarch64-v01c01-linux-gnu-gcc/bin/aarch64-linux-gnu-strip
# $STRIP --strip-unneeded $SCRIPT_DIR/install/lib/libonvif.so 

# file $SCRIPT_DIR/install/lib/libonvif.so 

# ./install_dynamic_basic_library.sh /home/meonardo/Hi3519/gcc-20231123-aarch64-v01c01-linux-gnu/aarch64-v01c01-linux-gnu-gcc arm-glibc_a53_hard_neon-vfpv4 