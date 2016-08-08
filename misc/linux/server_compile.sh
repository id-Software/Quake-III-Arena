#!/bin/bash
localPATH=`pwd`
export BUILD_CLIENT="0"
export BUILD_SERVER="1"
export USE_CURL="1"
export USE_CODEC_OPUS="1"
export USE_VOIP="1"
export COPYDIR="~/ioquake3"
IOQ3REMOTE="https://github.com/ioquake/ioq3.git"
IOQ3LOCAL="/tmp/ioquake3compile"
JOPTS="-j2" 
echo "This process requires you to have the following installed through your distribution:
 make
 git
 and all of the ioquake3 dependencies necessary for an ioquake3 server.
 If you do not have the necessary dependencies this script will bail out.
 Please post a message to http://discourse.ioquake.org/ asking for help and include whatever error messages you received during the compile phase.
 Please edit this script. Inside you will find a COPYDIR variable you can alter to change where ioquake3 will be installed to."
while true; do
        read -p "Are you ready to compile ioquake3 in the $IOQ3LOCAL directory, and have it installed into $COPYDIR? " yn
case $yn in
        [Yy]* )
if  [ -x "$(command -v git)" ] && [ -x "$(command -v make)" ] ; then
        git clone $IOQ3REMOTE $IOQ3LOCAL && cd $IOQ3LOCAL && make $JOPTS && make copyfiles && cd $localPATH && rm -rf $IOQ3LOCAL
fi
        exit;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
esac
done
