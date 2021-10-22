ioquake3 Xcode Projects
Created by Jeremiah Sypult & Contributors
Updated: 2019/10/28 by Tom Kidd

GENERAL:
From the ioquake3 root, navigate to misc/xcode and open the ioquake3.xcworkspace. The workspace embeds all the projects and currently builds universal (fat) binaries supporting i386 and x86_64.

Hopefully people working with ioquake3 on macOS will benefit from using Xcode for building, hacking and debugging.

NOTE: THE FIRST BUILD WITH XCODE WILL LIKELY FAIL. Subsequent builds will succeed. This is related to the second bullet point below. Some of the files necessary are created via a build phase but more recent versions of Xcode are less forgiving about the files being missing initially so the first build will fail. If anyone has a better way to handle this please let me know. 

LIBRARIES:

- Game libraries (cgame, game, ui) currently compile to universal binary dylib for native (versus qvm bytecode/interpreted).

- Renderer libraries also compile to universal binary dylib. The OpenGL 2 renderer project will normally show missing .c files in code/renderergl2/glsl. This is normal, as these .c files are automatically generated during the build process and excluded from source control. This step is required in order for the renderer to link due to symbol dependencies in tr_glsl.c for fallback shaders.

- ioquake3 searches for game & renderer libraries with an abbreviated architecture suffix in the filename. Xcode scripts handle symlink creation for the appropriate architectures at build time in order to support universal (fat) binaries.

- Static libraries are used for botlib, jpeg and opus.

- Currently links macOS system libcurl.dylib and libz.dylib.

- SDL binary dylib is included.


TODO:
- dedicated support
- curl.xcodeproj
- ogg.xcodeproj (ogg vorbis support)
- zlib.xcodeproj
- q3asm.xcodeproj
- q3cpp.xcodeproj
- q3lcc.xcodeproj
- q3rcc.xcodeproj













NO CARRIER

