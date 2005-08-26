#!/bin/bash
# Build various setups..

# inputs: 
#   directory with the common media
Q3SETUPMEDIA=/home/timo/Id/Q3SetupMedia/quake3
#   directory with binaries tree
Q3BINARIES=../install
#   version: $1
VERSION=$1
#   temporary directory used to prepare the files
#   NOTE: this dir is erased before a new setup is built
TMPDIR=setup.tmp

# location of the setup dir (for graphical installer and makeself)
SETUPDIR=setup

# cp setup phase
# we need to copy the symlinked files, and not the symlinks themselves
# on antares this is forced with a cp -L
# on spoutnik, -L is not recognized, and dereference is the default behaviour
# we need a robust way of checking
TESTFILE=/tmp/foo$$
touch $TESTFILE
# see if option is supported
cp -L $TESTFILE $TESTFILE.cp 2>/dev/null
if [ $? -eq 1 ]
then
  # option not supported, should be on by default
  echo "cp doesn't have -L option"
  unset CPOPT
else
  # option supported, use it
  echo "cp supports -L option"
  CPOPT="-L"
fi
rm $TESTFILE


# graphical installer (based on Loki Software's Setup tool)
build_installer ()
{
TMPDIR=setup.tmp

rm -rf $TMPDIR
mkdir $TMPDIR

# copy base setup files
cp $CPOPT -R $SETUPDIR/setup.sh $SETUPDIR/setup.data $TMPDIR

# copy media files
cp $CPOPT -R $Q3SETUPMEDIA/* $TMPDIR

# remove CVS entries
find $TMPDIR -name CVS | xargs rm -rf

# copy binaries
mkdir -p $TMPDIR/bin/x86
# smp
cp $CPOPT $Q3BINARIES/linuxquake3-smp $TMPDIR/bin/x86/quake3-smp.x86
strip $TMPDIR/bin/x86/quake3-smp.x86
brandelf -t Linux $TMPDIR/bin/x86/quake3-smp.x86
# old school
cp $CPOPT $Q3BINARIES/linuxquake3 $TMPDIR/bin/x86/quake3.x86
strip $TMPDIR/bin/x86/quake3.x86
brandelf -t Linux $TMPDIR/bin/x86/quake3.x86
# ded
cp $CPOPT $Q3BINARIES/linuxq3ded $TMPDIR/bin/x86/q3ded
strip $TMPDIR/bin/x86/q3ded
brandelf -t Linux $TMPDIR/bin/x86/q3ded

# PB files
mkdir -p $TMPDIR/pb/htm
cp $CPOPT ../pb/linux/*.so $TMPDIR/pb
cp $CPOPT ../pb/htm/*.htm $TMPDIR/pb/htm

# Linux FAQ
mkdir -p $TMPDIR/Docs/LinuxFAQ
cp $CPOPT LinuxSupport/* $TMPDIR/Docs/LinuxFAQ

# generated .qvm pk3 files
mkdir -p $TMPDIR/baseq3
mkdir -p $TMPDIR/missionpack
# not needed now
#cp $CPOPT $Q3BINARIES/baseq3/pak8.pk3 $TMPDIR/baseq3/
#cp $CPOPT $Q3BINARIES/missionpack/pak3.pk3 $TMPDIR/missionpack/

# menu shortcut to the game
# FIXME current setup doesn't have a way to set symlinks on arbitrary things
# so we use a dummy quake3 script (which will be overwritten by postinstall.sh)
echo -e "#!/bin/sh\necho \"If you read this, then the setup script failed miserably.\nPlease report to ttimo@idsoftware.com\n\"" > $TMPDIR/bin/x86/quake3
echo -e "#!/bin/sh\necho \"If you read this, then the setup script failed miserably.\nPlease report to ttimo@idsoftware.com\n\"" > $TMPDIR/bin/x86/quake3-smp
# create the auto-extractible archive
# first step: on FreeBSD we would default to Linux binaries .. use a symlink
(
cd $TMPDIR/setup.data/bin
ln -s Linux FreeBSD
ln -s Linux NetBSD
ln -s Linux OpenBSD
)
# NOTE: we used to pass the $VERSION, but it doesn't seem very usefull
./$SETUPDIR/makeself/makeself.sh $TMPDIR linuxq3apoint-$VERSION.x86.run "Quake III Arena Point Release $VERSION " ./setup.sh

chmod a+rx linuxq3apoint-$VERSION.x86.run

#rm -rf $TMPDIR
}

check_brandelf()
{
  # make sure brandelf is installed to avoid any problem when building the setups
  BRAND=`which brandelf`;
  if [ -n "$BRAND" ] && [ -x "$BRAND" ]
  then
    echo "brandelf is present: $BRAND"
  else
    echo "brandelf not found"
    exit
  fi
}

# safe checks
check_brandelf

build_installer
