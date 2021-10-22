#!/bin/bash

cd `dirname $0`
if [ ! -f Makefile ]; then
	echo "This script must be run from the ioquake3 build directory"
	exit 1
fi

# This script is to build a Universal 2 binary
# (Apple's term for an x86_64 and arm64 binary)

unset X86_64_SDK
unset X86_64_CFLAGS
unset X86_64_MACOSX_VERSION_MIN
unset ARM64_SDK
unset ARM64_CFLAGS
unset ARM64_MACOSX_VERSION_MIN

X86_64_MACOSX_VERSION_MIN="10.7"
ARM64_MACOSX_VERSION_MIN="11.0"

echo "Building X86_64 Client/Dedicated Server"
echo "Building ARM64 Client/Dedicated Server"
echo

if [ "$1" == "" ]; then
	echo "Run script with a 'notarize' flag to perform signing and notarization."
fi

# For parallel make on multicore boxes...
NCPU=`sysctl -n hw.ncpu`

# x86_64 client and server
#if [ -d build/release-release-x86_64 ]; then
#	rm -r build/release-darwin-x86_64
#fi
(ARCH=x86_64 CFLAGS=$X86_64_CFLAGS MACOSX_VERSION_MIN=$X86_64_MACOSX_VERSION_MIN make -j$NCPU) || exit 1;

echo;echo

# arm64 client and server
#if [ -d build/release-release-arm64 ]; then
#	rm -r build/release-darwin-arm64
#fi
(ARCH=arm64 CFLAGS=$ARM64_CFLAGS MACOSX_VERSION_MIN=$ARM64_MACOSX_VERSION_MIN make -j$NCPU) || exit 1;

echo

# use the following shell script to build a universal 2 application bundle
export MACOSX_DEPLOYMENT_TARGET="10.7"
export MACOSX_DEPLOYMENT_TARGET_X86_64="$X86_64_MACOSX_VERSION_MIN"
export MACOSX_DEPLOYMENT_TARGET_ARM64="$ARM64_MACOSX_VERSION_MIN"

if [ -d build/release-darwin-universal2 ]; then
	rm -r build/release-darwin-universal2
fi
"./make-macosx-app.sh" release

if [ "$1" == "notarize" ]; then
	# user-specific values
	# specify the actual values in a separate file called make-macosx-values.local

	# ****************************************************************************************
	# identity as specified in Keychain
	SIGNING_IDENTITY="Developer ID Application: Your Name (XXXXXXXXX)"

	ASC_USERNAME="your@apple.id"

	# signing password is app-specific (https://appleid.apple.com/account/manage) and stored in Keychain (as "notarize-app" in this case)
	ASC_PASSWORD="@keychain:notarize-app"

	# ProviderShortname can be found with
	# xcrun altool --list-providers -u your@apple.id -p "@keychain:notarize-app"
	ASC_PROVIDER="XXXXXXXXX"
	# ****************************************************************************************

	source make-macosx-values.local

	# release build location
	RELEASE_LOCATION="build/release-darwin-universal2"

	# release build name
	RELEASE_BUILD="ioquake3.app"

	# Pre-notarized zip file (not what is shipped)
	PRE_NOTARIZED_ZIP="ioquake3_prenotarized.zip"

	# Post-notarized zip file (shipped)
	POST_NOTARIZED_ZIP="ioquake3_notarized.zip"

	BUNDLE_ID="org.ioquake3.ioquake3"

	# allows for unsigned executable memory in hardened runtime
	# see: https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_security_cs_allow-unsigned-executable-memory
	ENTITLEMENTS_FILE="misc/xcode/ioquake3/ioquake3.entitlements"

	# sign the resulting app bundle
	echo "signing..."
	codesign --force --options runtime --deep --entitlements "${ENTITLEMENTS_FILE}" --sign "${SIGNING_IDENTITY}" ${RELEASE_LOCATION}/${RELEASE_BUILD}

	cd ${RELEASE_LOCATION}

	# notarize app
	# script taken from https://github.com/rednoah/notarize-app

	# create the zip to send to the notarization service
	echo "zipping..."
	ditto -c -k --sequesterRsrc --keepParent ${RELEASE_BUILD} ${PRE_NOTARIZED_ZIP}

	# create temporary files
	NOTARIZE_APP_LOG=$(mktemp -t notarize-app)
	NOTARIZE_INFO_LOG=$(mktemp -t notarize-info)

	# delete temporary files on exit
	function finish {
		rm "$NOTARIZE_APP_LOG" "$NOTARIZE_INFO_LOG"
	}
	trap finish EXIT

	echo "submitting..."
	# submit app for notarization
	if xcrun altool --notarize-app --primary-bundle-id "$BUNDLE_ID" --asc-provider "$ASC_PROVIDER" --username "$ASC_USERNAME" --password "$ASC_PASSWORD" -f "$PRE_NOTARIZED_ZIP" > "$NOTARIZE_APP_LOG" 2>&1; then
		cat "$NOTARIZE_APP_LOG"
		RequestUUID=$(awk -F ' = ' '/RequestUUID/ {print $2}' "$NOTARIZE_APP_LOG")

		# check status periodically
		while sleep 60 && date; do
			# check notarization status
			if xcrun altool --notarization-info "$RequestUUID" --asc-provider "$ASC_PROVIDER" --username "$ASC_USERNAME" --password "$ASC_PASSWORD" > "$NOTARIZE_INFO_LOG" 2>&1; then
				cat "$NOTARIZE_INFO_LOG"

				# once notarization is complete, run stapler and exit
				if ! grep -q "Status: in progress" "$NOTARIZE_INFO_LOG"; then
					xcrun stapler staple "$RELEASE_BUILD"
					break
				fi
			else
				cat "$NOTARIZE_INFO_LOG" 1>&2
				exit 1
			fi
		done
	else
		cat "$NOTARIZE_APP_LOG" 1>&2
		exit 1
	fi

	echo "notarized"
	echo "zipping notarized..."

	ditto -c -k --sequesterRsrc --keepParent ${RELEASE_BUILD} ${POST_NOTARIZED_ZIP}

	echo "done. ${POST_NOTARIZED_ZIP} contains notarized ${RELEASE_BUILD} build."
fi