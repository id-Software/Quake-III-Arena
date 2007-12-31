#!/bin/sh
APPBUNDLE=ioquake3.app
BINARY=ioquake3.ub
DEDBIN=ioq3ded.ub
PKGINFO=APPLIOQ3
ICNS=misc/quake3.icns
DESTDIR=build/release-darwin-ub
BASEDIR=baseq3
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

# we want to use the oldest available SDK for max compatiblity
unset PPC_SDK_DIR
unset X86_SDK_DIR
unset PPC_SDK_INC
unset X86_SDK_INC
unset PPC_SDK_LIB
unset X86_SDK_LIB
unset PPC_SDK_OPENAL_DLOPEN
for availsdks in $(find /Developer/SDKs -type d -maxdepth 1 -mindepth 1 -name "MacOSX*" -exec basename {} \; | sort -r)
do
	case "$availsdks" in
	'MacOSX10.5u.sdk')
		PPC_SDK_DIR=/Developer/SDKs/MacOSX10.5u.sdk
		X86_SDK_DIR=/Developer/SDKs/MacOSX10.5u.sdk
		PPC_SDK_INC=usr/lib/gcc/powerpc-apple-darwin9/4.0.1/include
		X86_SDK_INC=usr/lib/gcc/i686-apple-darwin9/4.0.1/include
		PPC_SDK_LIB=usr/lib/gcc/powerpc-apple-darwin9/4.0.1
		X86_SDK_LIB=usr/lib/gcc/i686-apple-darwin9/4.0.1
		PPC_SDK_OPENAL_DLOPEN=0
	;;
	'MacOSX10.4u.sdk')
		PPC_SDK_DIR=/Developer/SDKs/MacOSX10.4u.sdk
		X86_SDK_DIR=/Developer/SDKs/MacOSX10.4u.sdk
		PPC_SDK_INC=usr/lib/gcc/powerpc-apple-darwin8/4.0.1/include
		X86_SDK_INC=usr/lib/gcc/i686-apple-darwin8/4.0.1/include
		PPC_SDK_LIB=usr/lib/gcc/powerpc-apple-darwin8/4.0.1
		X86_SDK_LIB=usr/lib/gcc/i686-apple-darwin8/4.0.1
		PPC_SDK_OPENAL_DLOPEN=0
	;;
	'MacOSX10.3.9.sdk')
		PPC_SDK_DIR=/Developer/SDKs/MacOSX10.3.9.sdk
		PPC_SDK_INC=usr/lib/gcc/powerpc-apple-darwin7/4.0.1/include
		PPC_SDK_LIB=usr/lib/gcc/powerpc-apple-darwin7/4.0.1
		PPC_SDK_OPENAL_DLOPEN=1
	;;
	'MacOSX10.2.8.sdk')
		# no longer supported due to lack of dlfcn.h 
		#PPC_SDK_DIR=/Developer/SDKs/MacOSX10.2.8.sdk
		#PPC_SDK_INC=usr/include/gcc/darwin/3.3
		#PPC_SDK_LIB=usr/lib/gcc/darwin/3.3
		#PPC_SDK_OPENAL_DLOPEN=1
	;;
	*)
		echo "WARNING: detected unknown MacOSX SDK ($availsdks)"
	esac
done

if [ -z $PPC_SDK_DIR ] || [ -z $X86_SDK_DIR ];  then
	echo "Error detecting compatible Mac OS X SDK."
	exit 1;
fi

if [ $PPC_SDK_DIR != "/Developer/SDKs/MacOSX10.3.9.sdk" ]; then
	echo "WARNING: missing MacOS10.3.9.sdk.  Resulting binary may not be compatible with Mac OS X 10.3"
	sleep 1
fi
if [ $X86_SDK_DIR != "/Developer/SDKs/MacOSX10.4u.sdk" ]; then
	echo "WARNING: missing MacOS10.4u.sdk.  Resulting binary may not be compatible with Mac OS X 10.4"
	sleep 1
fi

echo "Using $PPC_SDK_DIR for PowerPC"
echo "Using $X86_SDK_DIR for Intel"

(USE_OPENAL_DLOPEN=$PPC_SDK_OPENAL_DLOPEN \
 MACOSX_SDK_DIR=$PPC_SDK_DIR \
 MACOSX_SDK_INC=$PPC_SDK_INC \
 MACOSX_SDK_LIB=$PPC_SDK_LIB BUILD_MACOSX_UB=ppc make \
&&
 MACOSX_SDK_DIR=$X86_SDK_DIR \
 MACOSX_SDK_INC=$X86_SDK_INC \
 MACOSX_SDK_LIB=$X86_SDK_LIB BUILD_MACOSX_UB=i386 make ) || exit 1 

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

