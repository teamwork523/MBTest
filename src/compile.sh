#!/bin/bash

fname=$1

arm-linux-gnueabi-gcc-4.6 -static -march=armv7 $fname.c -o $fname

adb push $fname /data/local/
# also push the script
adb push run.sh /data/local/
