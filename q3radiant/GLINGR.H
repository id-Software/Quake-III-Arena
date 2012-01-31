/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// This .h file contains constants, typedefs, etc. for Intergraph
// extensions to OpenGL.  These extensions are:
//    
//            Multiple Palette Extension
//            Texture Object Extension

#define GL_INGR_multiple_palette        1
#define GL_EXT_texture_object           1


// New constants and typedefs for the Multiple Palette Extension
#define GL_PALETTE_INGR                 0x80c0
#define GL_MAX_PALETTES_INGR            0x80c1
#define GL_MAX_PALETTE_ENTRIES_INGR     0x80c2
#define GL_CURRENT_PALETTE_INGR         0x80c3
#define GL_PALETTE_WRITEMASK_INGR       0x80c4
#define GL_CURRENT_RASTER_PALETTE_INGR	0x80c5
#define GL_PALETTE_CLEAR_VALUE_INGR	0x80c6

// Function prototypes for the Multiple Palette Extension routines
typedef void (APIENTRY *PALETTEFUNCPTR)(GLuint);
typedef void (APIENTRY *PALETTEMASKFUNCPTR)(GLboolean);
typedef void (APIENTRY *WGLLOADPALETTEFUNCPTR)(GLuint, GLsizei, GLuint *);
typedef void (APIENTRY *CLEARPALETTEFUNCPTR)(GLuint);


// New Constants and typedefs for the Texture Object Extension
#define GL_TEXTURE_PRIORITY_EXT         0x8066
#define GL_TEXTURE_RESIDENT_EXT         0x8067
#define GL_TEXTURE_1D_BINDING_EXT       0x8068
#define GL_TEXTURE_2D_BINDING_EXT       0x8069

// Function prototypes for the Texture Object Extension routines
typedef GLboolean (APIENTRY *ARETEXRESFUNCPTR)(GLsizei, const GLuint *,
                    const GLboolean *);
typedef void (APIENTRY *BINDTEXFUNCPTR)(GLenum, GLuint);
typedef void (APIENTRY *DELTEXFUNCPTR)(GLsizei, const GLuint *);
typedef void (APIENTRY *GENTEXFUNCPTR)(GLsizei, GLuint *);
typedef GLboolean (APIENTRY *ISTEXFUNCPTR)(GLuint);
typedef void (APIENTRY *PRIORTEXFUNCPTR)(GLsizei, const GLuint *,
                    const GLclampf *);


/* OpenGL ExtEscape escape function constants */
#ifndef OPENGL_GETINFO
#define OPENGL_GETINFO  4353        /* for OpenGL ExtEscape */
#endif

// OPENGL_GETINFO ExtEscape sub-escape numbers.  They are defined by
// Microsoft.

#ifndef OPENGL_GETINFO_DRVNAME

#define OPENGL_GETINFO_DRVNAME  0


// Input structure for OPENGL_GETINFO ExtEscape.

typedef struct _OPENGLGETINFO
{
    ULONG   ulSubEsc;
} OPENGLGETINFO, *POPENGLGETINFO;


// Output structure for OPENGL_GETINFO_DRVNAME ExtEscape.

typedef struct _GLDRVNAMERET
{
    ULONG   ulVersion;              // must be 1 for this version
    ULONG   ulDriverVersion;        // driver specific version number
    WCHAR   awch[MAX_PATH+1];
} GLDRVNAMERET, *PGLDRVNAMERET;

#endif


