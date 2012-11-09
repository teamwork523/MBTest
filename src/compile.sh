#!/bin/bash

arm-linux-gnueabi-gcc-4.6 -static -march=armv7 MBTest.c -o MBTest

adb push MBTest /data/local/
