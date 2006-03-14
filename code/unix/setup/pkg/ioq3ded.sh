#!/bin/bash
# Rev: $Id: ioq3ded.sh,v 1.9 2006/01/18 13:47:42 raistlin Exp raistlin $
# Needed to make symlinks/shortcuts work.
# the binaries must run with correct working directory
#

IOQ3_DIR=/usr/local/share/games/quake3

COMPILE_PLATFORM=`uname|sed -e s/_.*//|tr A-Z a-z`
COMPILE_ARCH=`uname -p | sed -e s/i.86/i386/`

EXEC_REL=release

#EXEC_BIN=ioquake3.${COMPILE_ARCH}
#EXEC_BIN=ioquake3-smp.${COMPILE_ARCH}
EXEC_BIN=ioq3ded.${COMPILE_ARCH}

EXEC_FLAGS="+set fs_cdpath ${IOQ3_DIR} +set vm_game 1 +set vm_cgame 1 +set vm_ui 1 +set sv_pure 1 +set ttycon 0"

EXEC_DIR_LIST=${IOQ3_DIR}

for d in ${EXEC_DIR_LIST}
do
	if [ -d $d ]; then
		EXEC_DIR=${d}
		break
	fi
done

if [ "X${EXEC_DIR}" != "X" ]; then
	if [ ! -x  ${EXEC_DIR}/${EXEC_BIN} ]; then
		echo "Executable ${EXEC_DIR}/${EXEC_BIN} not found!" ; exit 1
	fi
	cd ${IOQ3_DIR} && \
	${EXEC_DIR}/${EXEC_BIN} ${EXEC_FLAGS} $*
	exit $? 
else
	echo "No ioq3 binaries found!"
	exit 1
fi
  

