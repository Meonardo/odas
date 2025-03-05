#!/bin/bash

set -e

adb pull /userdata/amdox/bin/separated.raw /home/hd/project/tmp/
adb pull /userdata/amdox/bin/postfiltered.raw /home/hd/project/tmp/

echo "File copied to Windows"
echo "Done"