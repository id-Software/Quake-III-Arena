#!/bin/sh

readlink() {
    local path=$1 ll
    
    if [ -L "$path" ]; then 
        ll="$(LC_ALL=C ls -l "$path" 2> /dev/null)" &&
        echo "${ll##* -> }"
    else    
        return 1
    fi
}

script=$0
count=0
while [ -L "$script" ]  
do
    script=$(readlink "$script")
    count=`expr $count + 1`
    if [ $count -gt 100 ]   
    then    
        echo "Too many symbolic links"
        exit 1
    fi
done
cd "`dirname $script`"


lib=lib
test -e lib64 && lib=lib64

if test "x$LD_LIBRARY_PATH" = x; then
	LD_LIBRARY_PATH="`pwd`/$lib"
else
	LD_LIBRARY_PATH="`pwd`/$lib:$LD_LIBRARY_PATH"
fi
export LD_LIBRARY_PATH

archs=`uname -m`
case "$archs" in
	i?86) archs=i386 ;;
	x86_64) archs="x86_64 i386" ;;
	ppc64) archs="ppc64 ppc" ;;
esac

for arch in $archs; do
	test -x ./ioquake3.$arch || continue
	exec ./ioquake3.$arch "$@"
done
echo "could not execute ioquake3" >&2
