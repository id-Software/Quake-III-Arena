#!/bin/sh
# for easy use with Anjuta
cd `dirname $0`/debugi386-glibc
echo "in $PWD"

# now execute whatever you want
#./linuxquake3-smp +set fs_basepath /usr/local/games/quake3 +set developer 1 +set r_smp 1 +set r_showsmp 1 +devmap mythology

#gvd ./linuxquake3-smp --pargs +set logfile 2 +set fs_basepath /usr/local/games/quake3 +set developer 1 +set r_smp 0 +set r_showsmp 1 +devmap mythology

./linuxquake3-smp +set logfile 2 +set fs_basepath /usr/local/games/quake3 +set developer 1 +set r_smp 1 +set r_fullscreen 0 +set r_showsmp 1 +devmap mythology &
gvd ./linuxquake3-smp
