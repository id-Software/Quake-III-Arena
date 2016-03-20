#!/bin/bash
#
# This script appends the text from Q3A_EULA.txt to a .dmg as a SLA resource
#
# usage is './SLA-dmg.sh /path/to/Q3A_EULA.txt /path/to/ioquake3.dmg'
#

if [ "x$1" = "x" ] || [ "x$2" = "x" ]; then
	echo "usage: ./SLA-dmg.sh /path/to/Q3A_EULA.txt /path/to/ioquake3.dmg"
	exit 1;
fi

if [ ! -r $1 ]; then
	echo "$1 is not a readable Q3A_EULA.txt file"
	exit 1;
fi
if [ ! -w $2 ]; then
	echo "$2 is not writable .dmg file"
	exit 1;
fi
touch tmp.r
if [ ! -w tmp.r ]; then
	echo "Could not create temporary file tmp.r for writing"
	exit 1;
fi

echo "  
data 'LPic' (5000) {
    \$\"0002 0011 0003 0001 0000 0000 0002 0000\"
    \$\"0008 0003 0000 0001 0004 0000 0004 0005\"
    \$\"0000 000E 0006 0001 0005 0007 0000 0007\"
    \$\"0008 0000 0047 0009 0000 0034 000A 0001\"
    \$\"0035 000B 0001 0020 000C 0000 0011 000D\"
    \$\"0000 005B 0004 0000 0033 000F 0001 000C\"
    \$\"0010 0000 000B 000E 0000\"
};

data 'TEXT' (5002, \"English\") {
" > tmp.r

sed -e 's/"/\\"/g' -e 's/\(.*\)$/"\1\\n"/g' $1 >> tmp.r

echo "
};

resource 'STR#' (5002, \"English\") {
    {   
        \"English\",
        \"Agree\",
        \"Disagree\",
        \"Print\",
        \"Save...\",
        \"IMPORTANT - Read this License Agreement carefully before clicking on \"
        \"the \\\"Agree\\\" button.  By clicking on the \\\"Agree\\\" button, you agree \"
        \"to be bound by the terms of the License Agreement.\",
        \"Software License Agreement\",
        \"This text cannot be saved. This disk may be full or locked, or the \"
	\"file may be locked.\",
        \"Unable to print. Make sure you have selected a printer.\"
    }
};
" >> tmp.r

hdiutil convert -format UDCO -o tmp.dmg $2 || exit 1
hdiutil unflatten tmp.dmg || exit 1
/Developer/Tools/Rez /Developer/Headers/FlatCarbon/*.r tmp.r -a -o tmp.dmg \
	|| exit 1
hdiutil flatten tmp.dmg || exit 1
hdiutil internet-enable -yes tmp.dmg || exit 1
mv tmp.dmg $2 || (echo "Could not copy tmp.dmg to $2" && exit 1)
rm tmp.dmg
rm tmp.r
echo "SLA $1 successfully added to $2"
