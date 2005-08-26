#!/bin/zsh

buildRoot=./build
executable=$buildRoot/Quake3.app/Contents/MacOS/Quake3
ls -l $executable

flags="$flags +set timedemo 1"
flags="$flags +set s_initsound 0"
flags="$flags +set vm_cgame 1"
flags="$flags +set vm_game 1"
flags="$flags +set r_texturebits 16"
flags="$flags +set r_depthbits 16"
flags="$flags +set r_colorbits 16"
flags="$flags +set stencilbits 8"

flags="$flags +set r_appleTransformHint 1"

echo flags=$flags

function demo {
    echo Demo $*
    $executable $flags +demo $* |& egrep "(seconds|VM)"
}

demo foo

