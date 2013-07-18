#!/bin/bash

# Let's make the user give us a target to work with
# architecture is optional
# if used, it we will store the .app bundle in the target arch build directory
if [ $# == 0 ] || [ $# -gt 2 ]; then
	echo "Usage:   $0 target <arch>"
	echo "Example: $0 release x86"
	echo "Valid targets are:"
	echo " release"
	echo " debug"
	echo
	echo "Optional architectures are:"
	echo " x86"
	echo " x86_64"
	echo " ppc"
	echo
	exit 1
fi

if [ "$1" == "release" ]; then
	TARGET_NAME="release"
elif [ "$1" == "debug" ]; then
	TARGET_NAME="debug"
else
	echo "Invalid target: $1"
	echo "Valid targets are:"
	echo " release"
	echo " debug"
	exit 1
fi

CURRENT_ARCH=""

if [ "$2" != "" ]; then
	if [ "$2" == "x86" ]; then
		CURRENT_ARCH="x86"
	elif [ "$2" == "x86_64" ]; then
		CURRENT_ARCH="x86_64"
	elif [ "$2" == "ppc" ]; then
		CURRENT_ARCH="ppc"
	else
		echo "Invalid architecture: $1"
		echo "Valid architectures are:"
		echo " x86"
		echo " x86_64"
		echo " ppc"
		echo
		exit 1
	fi
fi

function symlinkArch()
{
    EXT="dylib"
    SEP="${3}"
    SRCFILE="${1}"
    DSTFILE="${2}${SEP}"
    DSTPATH="${4}"

    if [ ! -e "${DSTPATH}/${SRCFILE}.${EXT}" ]; then
        echo "**** ERROR: missing ${SRCFILE}.${EXT} from ${MACOS}"
        exit 1
    fi

    if [ ! -d "${DSTPATH}" ]; then
        echo "**** ERROR: path not found ${DSTPATH}"
        exit 1
    fi

    pushd "${DSTPATH}" > /dev/null

    IS32=`file "${SRCFILE}.${EXT}" | grep "i386" | awk '{print $NF}'`
    IS64=`file "${SRCFILE}.${EXT}" | grep "x86_64" | awk '{print $NF}'`
	ISPPC=`file "${SRCFILE}.${EXT}" | grep "ppc" | awk '{print $NF}'`

    if [ "${IS32}" == "i386" ]; then
        if [ ! -L "${DSTFILE}x86.${EXT}" ]; then
            ln -s "${SRCFILE}.${EXT}" "${DSTFILE}x86.${EXT}"
        fi
    elif [ -L "${DSTFILE}x86.${EXT}" ]; then
        rm "${DSTFILE}x86.${EXT}"
    fi

    if [ "${IS64}" == "x86_64" ]; then
        if [ ! -L "${DSTFILE}x86_64.${EXT}" ]; then
            ln -s "${SRCFILE}.${EXT}" "${DSTFILE}x86_64.${EXT}"
        fi
    elif [ -L "${DSTFILE}x86_64.${EXT}" ]; then
        rm "${DSTFILE}x86_64.${EXT}"
    fi

    if [ "${ISPPC}" == "ppc" ]; then
        if [ ! -L "${DSTFILE}ppc.${EXT}" ]; then
            ln -s "${SRCFILE}.${EXT}" "${DSTFILE}ppc.${EXT}"
        fi
    elif [ -L "${DSTFILE}ppc.${EXT}" ]; then
        rm "${DSTFILE}ppc.${EXT}"
    fi

    popd > /dev/null
}

SEARCH_ARCHS="																	\
	x86																			\
	x86_64																		\
	ppc																			\
"

# if the optional arch parameter is used, we'll set CURRENT_ARCH
if [ "${CURRENT_ARCH}" != "" ]; then
	SEARCH_ARCHS="${CURRENT_ARCH}"
fi

AVAILABLE_ARCHS=""

IOQ3_CLIENT_ARCHS=""
IOQ3_SERVER_ARCHS=""
IOQ3_RENDERER_GL1_ARCHS=""
IOQ3_RENDERER_GL2_ARCHS=""
IOQ3_CGAME_ARCHS=""
IOQ3_GAME_ARCHS=""
IOQ3_UI_ARCHS=""
IOQ3_MP_CGAME_ARCHS=""
IOQ3_MP_GAME_ARCHS=""
IOQ3_MP_UI_ARCHS=""

BASEDIR="baseq3"
MISSIONPACKDIR="missionpack"

CGAME="cgame"
GAME="qagame"
UI="ui"

RENDERER_OPENGL="renderer_opengl"

DEDICATED_NAME="ioq3ded"

CGAME_NAME="${CGAME}.dylib"
GAME_NAME="${GAME}.dylib"
UI_NAME="${UI}.dylib"

RENDERER_OPENGL1_NAME="${RENDERER_OPENGL}1.dylib"
RENDERER_OPENGL2_NAME="${RENDERER_OPENGL}2.dylib"

ICNSDIR="misc"
ICNS="quake3_flat.icns"
PKGINFO="APPLIOQ3"

OBJROOT="build"
#BUILT_PRODUCTS_DIR="${OBJROOT}/${TARGET_NAME}-darwin-${CURRENT_ARCH}"
PRODUCT_NAME="ioquake3"
WRAPPER_EXTENSION="app"
WRAPPER_NAME="${PRODUCT_NAME}.${WRAPPER_EXTENSION}"
CONTENTS_FOLDER_PATH="${WRAPPER_NAME}/Contents"
UNLOCALIZED_RESOURCES_FOLDER_PATH="${CONTENTS_FOLDER_PATH}/Resources"
EXECUTABLE_FOLDER_PATH="${CONTENTS_FOLDER_PATH}/MacOS"
EXECUTABLE_NAME="${PRODUCT_NAME}"

for ARCH in $SEARCH_ARCHS; do
	CURRENT_ARCH=${ARCH}
	BUILT_PRODUCTS_DIR="${OBJROOT}/${TARGET_NAME}-darwin-${CURRENT_ARCH}"
	IOQ3_CLIENT="${EXECUTABLE_NAME}.${CURRENT_ARCH}"
	IOQ3_SERVER="${DEDICATED_NAME}.${CURRENT_ARCH}"
	IOQ3_RENDERER_GL1="${RENDERER_OPENGL}1_${CURRENT_ARCH}.dylib"
	IOQ3_RENDERER_GL2="${RENDERER_OPENGL}2_${CURRENT_ARCH}.dylib"
	IOQ3_CGAME="${CGAME}${CURRENT_ARCH}.dylib"
	IOQ3_GAME="${GAME}${CURRENT_ARCH}.dylib"
	IOQ3_UI="${UI}${CURRENT_ARCH}.dylib"

	if [ ! -d ${BUILT_PRODUCTS_DIR} ]; then
		CURRENT_ARCH=""
		BUILT_PRODUCTS_DIR=""
		continue
	fi

	# executables
	if [ -e ${BUILT_PRODUCTS_DIR}/${IOQ3_CLIENT} ]; then
		IOQ3_CLIENT_ARCHS="${BUILT_PRODUCTS_DIR}/${IOQ3_CLIENT} ${IOQ3_CLIENT_ARCHS}"
		VALID_ARCHS="${ARCH} ${VALID_ARCHS}"
	else
		continue
	fi
	if [ -e ${BUILT_PRODUCTS_DIR}/${IOQ3_SERVER} ]; then
		IOQ3_SERVER_ARCHS="${BUILT_PRODUCTS_DIR}/${IOQ3_SERVER} ${IOQ3_SERVER_ARCHS}"
	fi

	# renderers
	if [ -e ${BUILT_PRODUCTS_DIR}/${IOQ3_RENDERER_GL1} ]; then
		IOQ3_RENDERER_GL1_ARCHS="${BUILT_PRODUCTS_DIR}/${IOQ3_RENDERER_GL1} ${IOQ3_RENDERER_GL1_ARCHS}"
	fi
	if [ -e ${BUILT_PRODUCTS_DIR}/${IOQ3_RENDERER_GL2} ]; then
		IOQ3_RENDERER_GL2_ARCHS="${BUILT_PRODUCTS_DIR}/${IOQ3_RENDERER_GL2} ${IOQ3_RENDERER_GL2_ARCHS}"
	fi

	# game
	if [ -e ${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_CGAME} ]; then
		IOQ3_CGAME_ARCHS="${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_CGAME} ${IOQ3_CGAME_ARCHS}"
	fi
	if [ -e ${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_GAME} ]; then
		IOQ3_GAME_ARCHS="${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_GAME} ${IOQ3_GAME_ARCHS}"
	fi
	if [ -e ${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_UI} ]; then
		IOQ3_UI_ARCHS="${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_UI} ${IOQ3_UI_ARCHS}"
	fi
	# missionpack
	if [ -e ${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_MP_CGAME} ]; then
		IOQ3_MP_CGAME_ARCHS="${BUILT_PRODUCTS_DIR}/${MISSIONPACKDIR}/${IOQ3_CGAME} ${IOQ3_MP_CGAME_ARCHS}"
	fi
	if [ -e ${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_MP_GAME} ]; then
		IOQ3_MP_GAME_ARCHS="${BUILT_PRODUCTS_DIR}/${MISSIONPACKDIR}/${IOQ3_GAME} ${IOQ3_MP_GAME_ARCHS}"
	fi
	if [ -e ${BUILT_PRODUCTS_DIR}/${BASEDIR}/${IOQ3_MP_UI} ]; then
		IOQ3_MP_UI_ARCHS="${BUILT_PRODUCTS_DIR}/${MISSIONPACKDIR}/${IOQ3_UI} ${IOQ3_MP_UI_ARCHS}"
	fi
done

if [ "${2}" == "" ]; then
	BUILT_PRODUCTS_DIR="${OBJROOT}"
else
	BUILT_PRODUCTS_DIR="${OBJROOT}/${TARGET_NAME}-darwin-${CURRENT_ARCH}"
fi

cd `dirname $0`

if [ ! -f Makefile ]; then
	echo "$0 must be run from the ioquake3 build directory"
	exit 1
fi

Q3_VERSION=`grep '^VERSION=' Makefile | sed -e 's/.*=\(.*\)/\1/'`

if [ "${IOQ3_CLIENT_ARCHS}" == "" ]; then
	echo "$0: no ioquake3 binary architectures were found for target '${TARGET_NAME}'"
	exit 1
else
	echo "Creating bundle '${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}'"
	echo "with architectures:"
	for ARCH in ${VALID_ARCHS}; do
		echo " ${ARCH}"
	done
	echo ""
fi


if [ ! -d ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/$BASEDIR ]; then
	mkdir -p ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/$BASEDIR || exit 1;
fi
if [ ! -d ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/$MISSIONPACKDIR ]; then
	mkdir -p ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/$MISSIONPACKDIR || exit 1;
fi
if [ ! -d ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH} ]; then
	mkdir -p ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH} || exit 1;
fi

cp ${ICNSDIR}/${ICNS} ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/$ICNS || exit 1;

echo -n ${PKGINFO} > ${BUILT_PRODUCTS_DIR}/${CONTENTS_FOLDER_PATH}/PkgInfo || exit 1;

echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>${EXECUTABLE_NAME}</string>
    <key>CFBundleIconFile</key>
    <string>quake3_flat</string>
    <key>CFBundleIdentifier</key>
    <string>org.ioquake.${PRODUCT_NAME}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>${PRODUCT_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>${Q3_VERSION}</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>CFBundleVersion</key>
    <string>${Q3_VERSION}</string>
    <key>CGDisableCoalescedUpdates</key>
    <true/>
    <key>LSMinimumSystemVersion</key>
    <string>${MACOSX_DEPLOYMENT_TARGET}</string>
    <key>NSHumanReadableCopyright</key>
    <string>QUAKE III ARENA Copyright © 1999-2000 id Software, Inc. All rights reserved.</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
" > ${BUILT_PRODUCTS_DIR}/${CONTENTS_FOLDER_PATH}/Info.plist

cp code/libs/macosx/*.dylib ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}

#
# use lipo to create a universal binary in to the appropriate bundle location
# then symlink appropriate architecture names for universal (fat) binary support
#
BUNDLEDIR="${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}"

# executables
lipo -create -o ${BUNDLEDIR}/${EXECUTABLE_NAME}				${IOQ3_CLIENT_ARCHS}
lipo -create -o ${BUNDLEDIR}/${DEDICATED_NAME}				${IOQ3_SERVER_ARCHS}

# renderers
lipo -create -o ${BUNDLEDIR}/${RENDERER_OPENGL1_NAME}		${IOQ3_RENDERER_GL1_ARCHS}
lipo -create -o ${BUNDLEDIR}/${RENDERER_OPENGL2_NAME}		${IOQ3_RENDERER_GL2_ARCHS}
symlinkArch "${RENDERER_OPENGL}1" "${RENDERER_OPENGL}1" "_" "${BUNDLEDIR}"
symlinkArch "${RENDERER_OPENGL}2" "${RENDERER_OPENGL}2" "_" "${BUNDLEDIR}"

# game
lipo -create -o ${BUNDLEDIR}/${BASEDIR}/${CGAME_NAME}		${IOQ3_CGAME_ARCHS}
lipo -create -o ${BUNDLEDIR}/${BASEDIR}/${GAME_NAME}		${IOQ3_GAME_ARCHS}
lipo -create -o ${BUNDLEDIR}/${BASEDIR}/${UI_NAME}			${IOQ3_UI_ARCHS}
symlinkArch "cgame"		"cgame"		""	"${BUNDLEDIR}/${BASEDIR}"
symlinkArch "qagame"	"qagame"	""	"${BUNDLEDIR}/${BASEDIR}"
symlinkArch "ui"		"ui"		""	"${BUNDLEDIR}/${BASEDIR}"

# missionpack
lipo -create -o ${BUNDLEDIR}/${MISSIONPACKDIR}/${CGAME_NAME}	${IOQ3_MP_CGAME_ARCHS}
lipo -create -o ${BUNDLEDIR}/${MISSIONPACKDIR}/${GAME_NAME}		${IOQ3_MP_GAME_ARCHS}
lipo -create -o ${BUNDLEDIR}/${MISSIONPACKDIR}/${UI_NAME}		${IOQ3_MP_UI_ARCHS}
symlinkArch "cgame"		"cgame"		""	"${BUNDLEDIR}/${MISSIONPACKDIR}"
symlinkArch "qagame"	"qagame"	""	"${BUNDLEDIR}/${MISSIONPACKDIR}"
symlinkArch "ui"		"ui"		""	"${BUNDLEDIR}/${MISSIONPACKDIR}"
