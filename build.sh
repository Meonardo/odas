#!/bin/bash

set -e

# build 

cd ./build

make -j4
echo "build done"

# copy files to shared
echo "copy bin to shared"
cp bin/odaslive ../../../shared/
cp bin/uac ../../../shared/

echo "copy so to shared"
cp lib/libodas.so ../../../shared/
cp lib/libodascore.so ../../../shared/

echo "copy config to shared"
cp ../config/odaslive/respeaker_8_mic_array.cfg ../../../shared/

echo "copy done"
