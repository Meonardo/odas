set -e

# 32~39 are the gain control of the 8 channels
# 31 is the maximum gain

# following 2 channels are reference channels, they are recording system sounds
# just like loopback channels
amixer -c 0 cset numid=32 31
amixer -c 0 cset numid=33 31

# following 6 channels are the 6 channels mic array
amixer -c 0 cset numid=34 31
amixer -c 0 cset numid=35 31
amixer -c 0 cset numid=36 31
amixer -c 0 cset numid=37 31
amixer -c 0 cset numid=38 31
amixer -c 0 cset numid=39 31

# environment variables
export LD_LIBRARY_PATH=/userdata/amdox/lib:$LD_LIBRARY_PATH
export PATH=/userdata/amdox/bin:$PATH

cd /userdata/amdox/bin/

# run gdbserver
gdbserver :6666 ./odaslive -c ./3308_6_mic_array.cfg -v