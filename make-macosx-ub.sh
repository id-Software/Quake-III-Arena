#!/bin/sh
APPBUNDLE=ioquake3.app
BINARY=ioquake3.ub
DEDBIN=ioq3ded.ub
PKGINFO=APPLIOQ3
ICNS=misc/quake3.icns
DESTDIR=build/release-darwin-ub
BASEDIR=baseq3
SDKDIR=""
MPACKDIR=missionpack
Q3_VERSION=`grep "\#define Q3_VERSION" code/qcommon/q_shared.h | \
	sed -e 's/.*".* \([^ ]*\)"/\1/'`;

BIN_OBJ="
	build/release-darwin-ppc/ioquake3.ppc
	build/release-darwin-i386/ioquake3.i386
"
BIN_DEDOBJ="
	build/release-darwin-ppc/ioq3ded.ppc
	build/release-darwin-i386/ioq3ded.i386
"
BASE_OBJ="
	build/release-darwin-ppc/$BASEDIR/cgameppc.dylib
	build/release-darwin-i386/$BASEDIR/cgamei386.dylib
	build/release-darwin-ppc/$BASEDIR/uippc.dylib
	build/release-darwin-i386/$BASEDIR/uii386.dylib
	build/release-darwin-ppc/$BASEDIR/qagameppc.dylib
	build/release-darwin-i386/$BASEDIR/qagamei386.dylib
"
MPACK_OBJ="
	build/release-darwin-ppc/$MPACKDIR/cgameppc.dylib
	build/release-darwin-i386/$MPACKDIR/cgamei386.dylib
	build/release-darwin-ppc/$MPACKDIR/uippc.dylib
	build/release-darwin-i386/$MPACKDIR/uii386.dylib
	build/release-darwin-ppc/$MPACKDIR/qagameppc.dylib
	build/release-darwin-i386/$MPACKDIR/qagamei386.dylib
"
if [ ! -f Makefile ]; then
	echo "This script must be run from the ioquake3 build directory";
fi

# this is kind of a hack to find out the latest SDK to use. I assume that newer SDKs appear later in this for loop,
# thus the last valid one is the one we want.

for availsdks in /Developer/SDKs/*
do
	if [ -d $availsdks ]
	then
		SDKDIR="$availsdks"
	fi
done

if [ -z $SDKDIR ]
then
	echo "MacOSX SDK is missing. Please install a recent version of the MacOSX SDK."
	exit 1;
else
	echo "Using $SDKDIR for compilation"
fi

(BUILD_MACOSX_UB=ppc make && BUILD_MACOSX_UB=i386 make) || exit 1;

echo "Creating .app bundle $DESTDIR/$APPBUNDLE"
if [ ! -d $DESTDIR/$APPBUNDLE/Contents/MacOS/$BASEDIR ]; then
	mkdir -p $DESTDIR/$APPBUNDLE/Contents/MacOS/$BASEDIR || exit 1;
fi
if [ ! -d $DESTDIR/$APPBUNDLE/Contents/MacOS/$MPACKDIR ]; then
	mkdir -p $DESTDIR/$APPBUNDLE/Contents/MacOS/$MPACKDIR || exit 1;
fi
if [ ! -d $DESTDIR/$APPBUNDLE/Contents/Resources ]; then
	mkdir -p $DESTDIR/$APPBUNDLE/Contents/Resources
fi
cp $ICNS $DESTDIR/$APPBUNDLE/Contents/Resources/ioquake3.icns || exit 1;
echo $PKGINFO > $DESTDIR/$APPBUNDLE/Contents/PkgInfo
echo "
	<?xml version=\"1.0\" encoding="UTF-8"?>
	<!DOCTYPE plist
		PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\"
		\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
	<plist version=\"1.0\">
	<dict>
		<key>CFBundleDevelopmentRegion</key>
		<string>English</string>
		<key>CFBundleExecutable</key>
		<string>$BINARY</string>
		<key>CFBundleGetInfoString</key>
		<string>ioquake3 $Q3_VERSION</string>
		<key>CFBundleIconFile</key>
		<string>ioquake3.icns</string>
		<key>CFBundleIdentifier</key>
		<string>org.icculus.quake3</string>
		<key>CFBundleInfoDictionaryVersion</key>
		<string>6.0</string>
		<key>CFBundleName</key>
		<string>ioquake3</string>
		<key>CFBundlePackageType</key>
		<string>APPL</string>
		<key>CFBundleShortVersionString</key>
		<string>$Q3_VERSION</string>
		<key>CFBundleSignature</key>
		<string>$PKGINFO</string>
		<key>CFBundleVersion</key>
		<string>$Q3_VERSION</string>
		<key>NSExtensions</key>
		<dict/>
		<key>NSPrincipalClass</key>
		<string>NSApplication</string>
	</dict>
	</plist>
	" > $DESTDIR/$APPBUNDLE/Contents/Info.plist

lipo -create -o $DESTDIR/$APPBUNDLE/Contents/MacOS/$BINARY $BIN_OBJ
lipo -create -o $DESTDIR/$APPBUNDLE/Contents/MacOS/$DEDBIN $BIN_DEDOBJ
cp $BASE_OBJ $DESTDIR/$APPBUNDLE/Contents/MacOS/$BASEDIR/
cp $MPACK_OBJ $DESTDIR/$APPBUNDLE/Contents/MacOS/$MPACKDIR/
cp code/libs/macosx/*.dylib $DESTDIR/$APPBUNDLE/Contents/MacOS/

