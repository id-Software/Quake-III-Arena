#!/bin/bash

# Source directory
MOUNT_DIR="../.."

# Solaris stuff
PLATFORM=`uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]'`
if [ "X${PLATFORM}" != "Xsunos" ]; then
	echo "Unsupported platform! Must run this script on Solaris host!" ; exit 127
fi


if [ "X`uname -m`" = "Xi86pc" ]; then
	ARCH=x86
else
	ARCH=sparc
fi

# Packages
PKG_SOLARIS_NAME=ioquake3
PKG_DATA_NAME=ioquake3d
PKG_DEMO_NAME=ioquake3m
BUILD_DATE="`/usr/bin/date '+%Y%m%d%H%M%S'`"
SVNVERSION=/usr/local/bin/svnversion
BUILD_VERSION="1.36_SVN"
if [ -x "$SVNVERSION" ]; then
	SVN_BANNER=`$SVNVERSION ${MOUNT_DIR}|sed -e 's/S$//' -e 's/M$//' `
	BUILD_VERSION="${BUILD_VERSION}${SVN_BANNER}"
fi
PKG_VERSION="`date '+%Y%m%d%H%M'`"
PKG_MAINT_ID="quake@cojot.name"
SOLARIS_PKGFILE="${PKG_SOLARIS_NAME}-${BUILD_VERSION}-${PKG_VERSION}-${ARCH}.pkg"
DATA_PKGFILE="${PKG_DATA_NAME}-${BUILD_VERSION}-${PKG_VERSION}.pkg"
DEMO_PKGFILE="${PKG_DEMO_NAME}-${BUILD_VERSION}-${PKG_VERSION}.pkg"

# build directories
BUILD_DIR="${MOUNT_DIR}/build/release-${PLATFORM}-${ARCH}"
PKG_SRC_DIR="${MOUNT_DIR}/misc/setup/pkg/${PKG_SOLARIS_NAME}"
PKG_BUILD_DIR="/tmp/ioquake3-build/${PKG_SOLARIS_NAME}-${BUILD_VERSION}"
PKG_EXTRA_BUILD_DIR="/usr/local/src/quake3-data/ioquake3/quake3"
PKG_DATA_SRC_DIR="${MOUNT_DIR}/misc/setup/pkg/${PKG_DATA_NAME}"
PKG_DATA_BUILD_DIR="/usr/local/src/quake3-data/ioquake3d/quake3"
PKG_DEMO_SRC_DIR="${MOUNT_DIR}/misc/setup/pkg/${PKG_DEMO_NAME}"
PKG_DEMO_BUILD_DIR="/usr/local/src/quake3-data/ioquake3m/quake3"

# Tools
RM="/usr/bin/rm"
TOUCH="/usr/bin/touch"
SED="/usr/bin/sed"
CAT="/usr/bin/cat"
NAWK="/usr/bin/nawk"
MKDIR="gmkdir -v -p"
INSTALL_BIN="ginstall -D -m 755"
INSTALL_DATA="ginstall -D -m 644"
PKGPROTO="/usr/bin/pkgproto"
PKGMK="/usr/bin/pkgmk"
PKGTRANS="/usr/bin/pkgtrans"

#############################################################################
# SOLARIS PACKAGE
#############################################################################

if [ -d ${BUILD_DIR} ]; then
	if [ ! -d ${BUILD_DIR}/pkg ]; then
		${MKDIR} ${BUILD_DIR}/pkg
	fi
	echo "Building ${BUILD_DIR}/pkg/${SOLARIS_PKGFILE}"
        ${RM} -f ${BUILD_DIR}/pkg/${SOLARIS_PKGFILE}
        ${TOUCH} ${BUILD_DIR}/pkg/${SOLARIS_PKGFILE}
        ${SED} -e "/VERSION=/s/.*/VERSION=${BUILD_VERSION}-${PKG_VERSION}/" \
                < ${PKG_SRC_DIR}/pkginfo.template \
                > ${PKG_SRC_DIR}/pkginfo
        ${CAT} ${PKG_SRC_DIR}/prototype.template > ${PKG_SRC_DIR}/prototype

	${INSTALL_DATA} ${MOUNT_DIR}/COPYING.txt ${PKG_SRC_DIR}/copyright
	for EXEC_READ in README id-readme.txt
	do
		if [ -f ${MOUNT_DIR}/${EXEC_READ} ]; then
			${INSTALL_DATA} ${MOUNT_DIR}/${EXEC_READ} ${PKG_BUILD_DIR}/${EXEC_READ}
		fi
	done

	for EXEC_BIN in ioq3ded ioquake3-smp ioquake3
	do
		if [ -f ${BUILD_DIR}/${EXEC_BIN}.${ARCH} ]; then
        		${INSTALL_BIN} ${BUILD_DIR}/${EXEC_BIN}.${ARCH} ${PKG_BUILD_DIR}/${EXEC_BIN}.${ARCH}
		fi
	done

	for EXEC_SH in ioq3ded.sh ioquake3.sh
	do
		if [ -f ${MOUNT_DIR}/misc/setup/pkg/${EXEC_SH} ]; then
        		${INSTALL_BIN} ${MOUNT_DIR}/misc/setup/pkg/${EXEC_SH} ${PKG_BUILD_DIR}/${EXEC_SH}
		fi
	done

	for EXEC_SO in cgamesparc.so qagamesparc.so uisparc.so cgamex86.so qagamex86.so uix86.so
	do
		if [ -f ${BUILD_DIR}/baseq3/${EXEC_SO} ]; then
        		${INSTALL_BIN} ${BUILD_DIR}/baseq3/${EXEC_SO} ${PKG_BUILD_DIR}/baseq3/${EXEC_SO}
		fi
		if [ -f ${BUILD_DIR}/missionpack/${EXEC_SO} ]; then
        		${INSTALL_BIN} ${BUILD_DIR}/missionpack/${EXEC_SO} ${PKG_BUILD_DIR}/missionpack/${EXEC_SO}
		fi
	done

	for EXEC_VM in cgame.qvm qagame.qvm ui.qvm
	do
		if [ -f ${BUILD_DIR}/baseq3/vm/${EXEC_VM} ]; then
        		${INSTALL_BIN} ${BUILD_DIR}/baseq3/vm/${EXEC_VM} ${PKG_BUILD_DIR}/baseq3/vm/${EXEC_VM}
		fi
		if [ -f ${BUILD_DIR}/missionpack/vm/${EXEC_VM} ]; then
        		${INSTALL_BIN} ${BUILD_DIR}/missionpack/vm/${EXEC_VM} ${PKG_BUILD_DIR}/missionpack/vm/${EXEC_VM}
		fi
	done

        ${PKGPROTO} ${PKG_BUILD_DIR}=quake3 ${PKG_EXTRA_BUILD_DIR}=quake3 | \
                ${NAWK} '{ print $1,$2,$3,$4 }' >> ${PKG_SRC_DIR}/prototype
        ${PKGMK} -o -p "${PKG_MAINT_ID}${BUILD_DATE}" \
                -b ${PKG_SRC_DIR} -f ${PKG_SRC_DIR}/prototype \
                -d /tmp -a ${ARCH} owner=root group=bin mode=0755
        ${PKGTRANS} -s /tmp ${BUILD_DIR}/pkg/${SOLARIS_PKGFILE} ${PKG_SOLARIS_NAME}

	echo "Building ${BUILD_DIR}/pkg/${DATA_PKGFILE}"
        ${RM} -f ${BUILD_DIR}/pkg/${DATA_PKGFILE}
        ${TOUCH} ${BUILD_DIR}/pkg/${DATA_PKGFILE}
        ${SED} -e "/VERSION=/s/.*/VERSION=${BUILD_VERSION}.${PKG_VERSION}/" \
                < ${PKG_DATA_SRC_DIR}/pkginfo.template \
                > ${PKG_DATA_SRC_DIR}/pkginfo
        ${CAT} ${PKG_DATA_SRC_DIR}/prototype.template > ${PKG_DATA_SRC_DIR}/prototype

	if [ -d ${MOUNT_DIR}/../webspace/include ]; then
		EULA_DIR=${MOUNT_DIR}/../webspace/include
	else
		if [ -d ${MOUNT_DIR}/../../webspace/include ]; then
			EULA_DIR=${MOUNT_DIR}/../../webspace/include
		fi
	fi
	if [ -f ${EULA_DIR}/id_patch_pk3s_Q3A_EULA.txt ]; then
		${INSTALL_DATA} ${EULA_DIR}/id_patch_pk3s_Q3A_EULA.txt ${PKG_DATA_SRC_DIR}/copyright
	fi

        ${PKGPROTO} ${PKG_DATA_BUILD_DIR}=quake3 | \
                ${NAWK} '{ print $1,$2,$3,$4 }' >> ${PKG_DATA_SRC_DIR}/prototype
        ${PKGMK} -o -p "${PKG_MAINT_ID}${BUILD_DATE}" \
                -b ${PKG_DATA_SRC_DIR} -f ${PKG_DATA_SRC_DIR}/prototype \
                -d /tmp -a ${ARCH} owner=root group=bin mode=0755
        ${PKGTRANS} -s /tmp ${BUILD_DIR}/pkg/${DATA_PKGFILE} ${PKG_DATA_NAME}

	echo "Building ${BUILD_DIR}/pkg/${DEMO_PKGFILE}"
        ${RM} -f ${BUILD_DIR}/pkg/${DEMO_PKGFILE}
        ${TOUCH} ${BUILD_DIR}/pkg/${DEMO_PKGFILE}
        ${SED} -e "/VERSION=/s/.*/VERSION=${BUILD_VERSION}.${PKG_VERSION}/" \
                < ${PKG_DEMO_SRC_DIR}/pkginfo.template \
                > ${PKG_DEMO_SRC_DIR}/pkginfo
        ${CAT} ${PKG_DEMO_SRC_DIR}/prototype.template > ${PKG_DEMO_SRC_DIR}/prototype

	if [ -d ${MOUNT_DIR}/../webspace/include ]; then
		EULA_DIR=${MOUNT_DIR}/../webspace/include
	else
		if [ -d ${MOUNT_DIR}/../../webspace/include ]; then
			EULA_DIR=${MOUNT_DIR}/../../webspace/include
		fi
	fi
	if [ -f ${EULA_DIR}/id_patch_pk3s_Q3A_EULA.txt ]; then
		${INSTALL_DEMO} ${EULA_DIR}/id_patch_pk3s_Q3A_EULA.txt ${PKG_DEMO_SRC_DIR}/copyright
	fi

        ${PKGPROTO} ${PKG_DEMO_BUILD_DIR}=quake3 | \
                ${NAWK} '{ print $1,$2,$3,$4 }' >> ${PKG_DEMO_SRC_DIR}/prototype
        ${PKGMK} -o -p "${PKG_MAINT_ID}${BUILD_DATE}" \
                -b ${PKG_DEMO_SRC_DIR} -f ${PKG_DEMO_SRC_DIR}/prototype \
                -d /tmp -a ${ARCH} owner=root group=bin mode=0755
        ${PKGTRANS} -s /tmp ${BUILD_DIR}/pkg/${DEMO_PKGFILE} ${PKG_DEMO_NAME}
else
	echo "Directory ${BUILD_DIR} not found!"
	exit 1
fi


