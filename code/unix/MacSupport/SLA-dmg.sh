#!/bin/sh
#
# This script appends a SLA.r (Software License Agreement) resource to a .dmg
#
# usage is './SLA-dmg.sh SLA.r /path/to/ioquake3.dmg'
#

if [ "x$1" = "x" ] || [ "x$2" = "x"]; then
	echo "usage: ./SLA-dmg.sh SLAFILE DMGFILE"
	exit 1;
fi

if [ ! -r $1 ]; then
	echo "$1 is not a readable .r file"
	exit 1;
fi
if [ ! -w $2 ]; then
	echo "$2 is not writable .dmg file"
	exit 1;
fi

hdiutil convert -format UDCO -o tmp.dmg $2 || exit 1
hdiutil unflatten tmp.dmg || exit 1
/Developer/Tools/Rez /Developer/Headers/FlatCarbon/*.r $1 -a -o tmp.dmg \
	|| exit 1
hdiutil flatten tmp.dmg || exit 1
hdiutil internet-enable -yes tmp.dmg || exit 1
mv tmp.dmg $2 || (echo "Could not copy tmp.dmg to $2" && exit 1)
rm tmp.dmg
echo "SLA $1 successfully added to $2"
