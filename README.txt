Quake III Arena GPL source release
==================================

This file contains the following sections:

LICENSE
GENERAL NOTES
COMPILING ON WIN32
COMPILING ON GNU/LINUX
COMPILING ON MAC

LICENSE
=======

See COPYING.txt for the GNU GENERAL PUBLIC LICENSE

Some source code in this release is not covered by the GPL:

IO on .zip files using portions of zlib
-----------------------------------------------------------------------------
lines	file(s)
4299	code/qcommon/unzip.c
4546	libs/pak/unzip.cpp
Copyright (C) 1998 Gilles Vollant
zlib is Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

MD4 Message-Digest Algorithm
-----------------------------------------------------------------------------
lines	file(s)
299		code/qcommon/md4.c
277		common/md4.c
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights reserved.

License to copy and use this software is granted provided that it is identified 
as the <93>RSA Data Security, Inc. MD4 Message-Digest Algorithm<94> in all mater
ial mentioning or referencing this software or this function.
License is also granted to make and use derivative works provided that such work
s are identified as <93>derived from the RSA Data Security, Inc. MD4 Message-Dig
est Algorithm<94> in all material mentioning or referencing the derived work.
RSA Data Security, Inc. makes no representations concerning either the merchanta
bility of this software or the suitability of this software for any particular p
urpose. It is provided <93>as is<94> without express or implied warranty of any 
kind.

checksums are used to validate pak files

standard C library replacement routines
-----------------------------------------------------------------------------
lines	file(s)
1324	code/game/bg_lib.c
Copyright (c) 1992, 1993
The Regents of the University of California. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:
     This product includes software developed by the University of
     California, Berkeley and its contributors.
4. Neither the name of the University nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

ADPCM coder/decoder
-----------------------------------------------------------------------------
lines	file(s)
330		code/client/snd_adpcm.c
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

JPEG library
-----------------------------------------------------------------------------
code/jpeg-6
libs/jpeg6
Copyright (C) 1991-1995, Thomas G. Lane

Permission is hereby granted to use, copy, modify, and distribute this
software (or portions thereof) for any purpose, without fee, subject to these
conditions:
(1) If any part of the source code for this software is distributed, then this
README file must be included, with this copyright and no-warranty notice
unaltered; and any additions, deletions, or changes to the original files
must be clearly indicated in accompanying documentation.
(2) If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the work of
the Independent JPEG Group".
(3) Permission for use of this software is granted only if the user accepts
full responsibility for any undesirable consequences; the authors accept
NO LIABILITY for damages of any kind.

These conditions apply to any software derived from or based on the IJG code,
not just to the unmodified library.  If you use our work, you ought to
acknowledge us.

NOTE: unfortunately the README that came with our copy of the library has
been lost, so the one from release 6b is included instead. There are a few
'glue type' modifications to the library to make it easier to use from
the engine, but otherwise the dependency can be easily cleaned up to a
better release of the library.


GENERAL NOTES
=============

A short summary of the file layout:

code/			   		Quake III Arena source code ( renderer, game code, OS layer etc. )
code/bspc				bot routes compiler source code
lcc/					the retargetable C compiler ( produces assembly to be turned into qvm bytecode by q3asm )
q3asm/					assembly to qvm bytecode compiler
q3map/					map compiler ( .map -> .bsp ) - this is the version that comes with Q3Radiant 200f
q3radiant/				Q3Radiant map editor build 200f ( common/ and libs/ are support dirs for radiant )

While we made sure we were still able to compile the game on Windows, GNU/Linux and Mac, this build didn't get any kind of extensive testing so it may not work completely right. Whenever an id game is released under GPL, several projects start making the source code more friendly to nowaday's compilers and environements. If you are picking up this release weeks/months/years after we uploaded it, you probably want to look around on the net for cleaned up versions of this codebase as well.

COMPILING ON WIN32
==================

VC7 / Visual C++ 2003 project files are provided:
code/quake3.sln
q3radiant/Radiant.sln

To compile the qvms, you need to run some batch files:
you will need to have lcc.exe q3cpp.exe q3rcc.exe and q3asm.exe in your path
( some precompiled binaries are provided in lcc/bin and code/win32/mod-sdk-setup/bin )
the qvm batch files are in code/game code/cgame code/q3_ui code/ui ..

COMPILING ON GNU/LINUX
==================

the build system using cons, which may be known as scons's perl ancestor now
you don't have to track it down though, the build script is provided in the tree
you will need nasm and gcc 2.95
make sure you have the X Direct Graphics Access and X Video Mode extensions headers for your X11
a typical compile command goes like this:
[..]/code$ ./unix/cons -- gcc=gcc-2.95 g++=g++-2.95

COMPILING ON MAC
================

project file for OSX compile is in code/macosx/Quake3.pbproj
