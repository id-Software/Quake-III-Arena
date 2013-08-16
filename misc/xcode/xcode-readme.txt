ioquake3 Xcode Projects
Created & Maintained by Jeremiah Sypult & Contributors
Updated: 2013/07/13


GENERAL:
From the ioquake3 root, navigate to misc/xcode and open the ioquake3.xcworkspace. The workspace embeds all the projects and currently builds universal (fat) binaries supporting i386 and x86_64.

An older version of Xcode may be able to add support for PowerPC, but it is not currently supported in the Xcode version I produced these projects with (4.6.3).

Hopefully people working with ioquake3 on OS X will benefit from using Xcodes IDE for building, hacking and debugging.


LIBRARIES:
- Game libraries (cgame, game, ui) compile to universal binary dylib for native (versus qvm bytecode/interpreted).
- Renderer libraries also compile to universal binary dylib. The OpenGL 2 renderer project will normally show missing .c files in code/renderergl2/glsl. This is normal, as these .c files are automatically generated during the build process and deleted when building completes. This step is required in order for the renderer to link due to symbol dependencies in tr_glsl.c for fallback shaders.
- ioquake3 searches for game & renderer libraries with an abbreviated architecture suffix in the filename. Xcode scripts handle symlink creation for the appropriate architectures at build time in order to support universal (fat) binaries.
- Static libraries are used for botlib, jpeg and speex.
- Currently links OS X system libcurl.dylib and libz.dylib.
- SDL binary dylib is included.


TODO:
- dedicated support
- missionpack support
- curl.xcodeproj
- ogg.xcodeproj (ogg vorbis support)
- opusfile.xcodeproj
- zlib.xcodeproj
- q3asm.xcodeproj
- q3cpp.xcodeproj
- q3lcc.xcodeproj
- q3rcc.xcodeproj
















NO CARRIER

