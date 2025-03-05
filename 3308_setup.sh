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

# record the 8 channels audio
# arecord -D hw:0,0  -f S16_LE -c 8 -r 32000 -d 10 -t wav /userdata/output.wav

## setup WiFi for 3308
ifconfig wlan0 up

## check if wpa_supplicant is running
ps aux | grep wpa_supplicant
## kill wpa_supplicant
killall wpa_supplicant

## setup wpa_supplicant.conf file
wpa_supplicant -B -i wlan0 -c /userdata/wpa_supplicant.conf
## get IP address
udhcpc -i wlan0
## check IP address
ifconfig wlan0