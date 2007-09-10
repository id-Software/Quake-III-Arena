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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_image.c
#include "tr_local.h"

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#define JPEG_INTERNALS
#include "../jpeg-6/jpeglib.h"

#include "../qcommon/puff.h"


static void LoadBMP( const char *name, byte **pic, int *width, int *height );
static void LoadTGA( const char *name, byte **pic, int *width, int *height );
static void LoadJPG( const char *name, byte **pic, int *width, int *height );
static void LoadPNG( const char *name, byte **pic, int *width, int *height );

static byte			 s_intensitytable[256];
static unsigned char s_gammatable[256];

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE		1024
static	image_t*		hashTable[FILE_HASH_SIZE];

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

typedef struct {
	char *name;
	int	minimize, maximize;
} textureMode_t;

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string ) {
	int		i;
	image_t	*glt;

	for ( i=0 ; i< 6 ; i++ ) {
		if ( !Q_stricmp( modes[i].name, string ) ) {
			break;
		}
	}

	// hack to prevent trilinear from being set on voodoo,
	// because their driver freaks...
	if ( i == 5 && glConfig.hardwareType == GLHW_3DFX_2D3D ) {
		ri.Printf( PRINT_ALL, "Refusing to set trilinear on a voodoo.\n" );
		i = 3;
	}


	if ( i == 6 ) {
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for ( i = 0 ; i < tr.numImages ; i++ ) {
		glt = tr.images[ i ];
		if ( glt->mipmap ) {
			GL_Bind (glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int	total;
	int i;

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[i]->frameUsed == tr.frameCount ) {
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	int		i;
	image_t	*image;
	int		texels;
	const char *yesno[] = {
		"no ", "yes"
	};

	ri.Printf (PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- -if-- wrap --name-------\n");
	texels = 0;

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[ i ];

		texels += image->uploadWidth*image->uploadHeight;
		ri.Printf (PRINT_ALL,  "%4i: %4i %4i  %s   %d   ",
			i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->TMU );
		switch ( image->internalFormat ) {
		case 1:
			ri.Printf( PRINT_ALL, "I    " );
			break;
		case 2:
			ri.Printf( PRINT_ALL, "IA   " );
			break;
		case 3:
			ri.Printf( PRINT_ALL, "RGB  " );
			break;
		case 4:
			ri.Printf( PRINT_ALL, "RGBA " );
			break;
		case GL_RGBA8:
			ri.Printf( PRINT_ALL, "RGBA8" );
			break;
		case GL_RGB8:
			ri.Printf( PRINT_ALL, "RGB8" );
			break;
		case GL_RGB4_S3TC:
			ri.Printf( PRINT_ALL, "S3TC " );
			break;
		case GL_RGBA4:
			ri.Printf( PRINT_ALL, "RGBA4" );
			break;
		case GL_RGB5:
			ri.Printf( PRINT_ALL, "RGB5 " );
			break;
		default:
			ri.Printf( PRINT_ALL, "???? " );
		}

		switch ( image->wrapClampMode ) {
		case GL_REPEAT:
			ri.Printf( PRINT_ALL, "rept " );
			break;
		case GL_CLAMP:
			ri.Printf( PRINT_ALL, "clmp " );
			break;
		default:
			ri.Printf( PRINT_ALL, "%4i ", image->wrapClampMode );
			break;
		}
		
		ri.Printf( PRINT_ALL, " %s\n", image->imgName );
	}
	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, " %i total texels (not including mipmaps)\n", texels);
	ri.Printf (PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function 
before or after.
================
*/
static void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out,  
							int outwidth, int outheight ) {
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[2048], p2[2048];
	byte		*pix1, *pix2, *pix3, *pix4;

	if (outwidth>2048)
		ri.Error(ERR_DROP, "ResampleTexture: max width");
								
	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for ( i=0 ; i<outwidth ; i++ ) {
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for ( i=0 ; i<outwidth ; i++ ) {
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth) {
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++) {
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int		i, c;
			byte	*p;

			p = (byte *)in;

			c = inwidth*inheight;
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;

		if ( glConfig.deviceSupportsGamma )
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}


/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2( unsigned *in, int inWidth, int inHeight ) {
	int			i, j, k;
	byte		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = (byte *) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total = 
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap (byte *in, int width, int height) {
	int		i, j;
	byte	*out;
	int		row;

	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( (unsigned *)in, width, height );
		return;
	}

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		for (i=0 ; i<width ; i++, out+=4, in+=8 ) {
			out[0] = ( in[0] + in[4] )>>1;
			out[1] = ( in[1] + in[5] )>>1;
			out[2] = ( in[2] + in[6] )>>1;
			out[3] = ( in[3] + in[7] )>>1;
		}
		return;
	}

	for (i=0 ; i<height ; i++, in+=row) {
		for (j=0 ; j<width ; j++, out+=4, in+=8) {
			out[0] = (in[0] + in[4] + in[row+0] + in[row+4])>>2;
			out[1] = (in[1] + in[5] + in[row+1] + in[row+5])>>2;
			out[2] = (in[2] + in[6] + in[row+2] + in[row+6])>>2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7])>>2;
		}
	}
}


/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture( byte *data, int pixelCount, byte blend[4] ) {
	int		i;
	int		inverseAlpha;
	int		premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( i = 0 ; i < pixelCount ; i++, data+=4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}

byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};


/*
===============
Upload32

===============
*/
extern qboolean charSet;
static void Upload32( unsigned *data, 
						  int width, int height, 
						  qboolean mipmap, 
						  qboolean picmip, 
							qboolean lightMap,
						  int *format, 
						  int *pUploadWidth, int *pUploadHeight )
{
	int			samples;
	unsigned	*scaledBuffer = NULL;
	unsigned	*resampledBuffer = NULL;
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	GLenum		internalFormat = GL_RGB;
	float		rMax = 0, gMax = 0, bMax = 0;

	//
	// convert to exact power of 2 sizes
	//
	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
	if ( r_roundImagesDown->integer && scaled_width > width )
		scaled_width >>= 1;
	if ( r_roundImagesDown->integer && scaled_height > height )
		scaled_height >>= 1;

	if ( scaled_width != width || scaled_height != height ) {
		resampledBuffer = ri.Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
		ResampleTexture (data, width, height, resampledBuffer, scaled_width, scaled_height);
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}

	//
	// clamp to minimum size
	//
	if (scaled_width < 1) {
		scaled_width = 1;
	}
	if (scaled_height < 1) {
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( scaled_width > glConfig.maxTextureSize
		|| scaled_height > glConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	scaledBuffer = ri.Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width*height;
	scan = ((byte *)data);
	samples = 3;
	if (!lightMap) {
		for ( i = 0; i < c; i++ )
		{
			if ( scan[i*4+0] > rMax )
			{
				rMax = scan[i*4+0];
			}
			if ( scan[i*4+1] > gMax )
			{
				gMax = scan[i*4+1];
			}
			if ( scan[i*4+2] > bMax )
			{
				bMax = scan[i*4+2];
			}
			if ( scan[i*4 + 3] != 255 ) 
			{
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if ( samples == 3 )
		{
			if ( glConfig.textureCompression == TC_S3TC )
			{
				internalFormat = GL_RGB4_S3TC;
			}
			else if ( r_texturebits->integer == 16 )
			{
				internalFormat = GL_RGB5;
			}
			else if ( r_texturebits->integer == 32 )
			{
				internalFormat = GL_RGB8;
			}
			else
			{
				internalFormat = 3;
			}
		}
		else if ( samples == 4 )
		{
			if ( r_texturebits->integer == 16 )
			{
				internalFormat = GL_RGBA4;
			}
			else if ( r_texturebits->integer == 32 )
			{
				internalFormat = GL_RGBA8;
			}
			else
			{
				internalFormat = 4;
			}
		}
	} else {
		internalFormat = 3;
	}
	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) && 
		( scaled_height == height ) ) {
		if (!mipmap)
		{
			qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
		Com_Memcpy (scaledBuffer, data, width*height*4);
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		Com_Memcpy( scaledBuffer, data, width * height * 4 );
	}

	R_LightScaleTexture (scaledBuffer, scaled_width, scaled_height, !mipmap );

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap( (byte *)scaledBuffer, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;

			if ( r_colorMipLevels->integer ) {
				R_BlendOverTexture( (byte *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel] );
			}

			qglTexImage2D (GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}
done:

	if (mipmap)
	{
		if ( textureFilterAnisotropic )
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
					(GLint)Com_Clamp( 1, maxAnisotropy, r_ext_max_anisotropy->integer ) );

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		if ( textureFilterAnisotropic )
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();

	if ( scaledBuffer != 0 )
		ri.Hunk_FreeTempMemory( scaledBuffer );
	if ( resampledBuffer != 0 )
		ri.Hunk_FreeTempMemory( resampledBuffer );
}


/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t *R_CreateImage( const char *name, const byte *pic, int width, int height, 
					   qboolean mipmap, qboolean allowPicmip, int glWrapClampMode ) {
	image_t		*image;
	qboolean	isLightmap = qfalse;
	long		hash;

	if (strlen(name) >= MAX_QPATH ) {
		ri.Error (ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name);
	}
	if ( !strncmp( name, "*lightmap", 9 ) ) {
		isLightmap = qtrue;
	}

	if ( tr.numImages == MAX_DRAWIMAGES ) {
		ri.Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");
	}

	image = tr.images[tr.numImages] = ri.Hunk_Alloc( sizeof( image_t ), h_low );
	image->texnum = 1024 + tr.numImages;
	tr.numImages++;

	image->mipmap = mipmap;
	image->allowPicmip = allowPicmip;

	strcpy (image->imgName, name);

	image->width = width;
	image->height = height;
	image->wrapClampMode = glWrapClampMode;

	// lightmaps are always allocated on TMU 1
	if ( glConfig.numTextureUnits > 1 && isLightmap ) {
		image->TMU = 1;
	} else {
		image->TMU = 0;
	}

	if ( glConfig.numTextureUnits > 1 ) {
		GL_SelectTexture( image->TMU );
	}

	GL_Bind(image);

	Upload32( (unsigned *)pic, image->width, image->height, 
								image->mipmap,
								allowPicmip,
								isLightmap,
								&image->internalFormat,
								&image->uploadWidth,
								&image->uploadHeight );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

	qglBindTexture( GL_TEXTURE_2D, 0 );

	if ( image->TMU == 1 ) {
		GL_SelectTexture( 0 );
	}

	hash = generateHashValue(name);
	image->next = hashTable[hash];
	hashTable[hash] = image;

	return image;
}


/*
=========================================================

BMP LOADING

=========================================================
*/
typedef struct
{
	char id[2];
	unsigned long fileSize;
	unsigned long reserved0;
	unsigned long bitmapDataOffset;
	unsigned long bitmapHeaderSize;
	unsigned long width;
	unsigned long height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned long compression;
	unsigned long bitmapDataSize;
	unsigned long hRes;
	unsigned long vRes;
	unsigned long colors;
	unsigned long importantColors;
	unsigned char palette[256][4];
} BMPHeader_t;

static void LoadBMP( const char *name, byte **pic, int *width, int *height )
{
	int		columns, rows;
	unsigned	numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	BMPHeader_t bmpHeader;
	byte		*bmpRGBA;

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_ReadFile( ( char * ) name, (void **)&buffer);
	if (!buffer) {
		return;
	}

	buf_p = buffer;

	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.reserved0 = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.width = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.height = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.planes = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.compression = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.hRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.vRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.colors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.importantColors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;

	Com_Memcpy( bmpHeader.palette, buf_p, sizeof( bmpHeader.palette ) );

	if ( bmpHeader.bitsPerPixel == 8 )
		buf_p += 1024;

	if ( bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M' ) 
	{
		ri.Error( ERR_DROP, "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.fileSize != length )
	{
		ri.Error( ERR_DROP, "LoadBMP: header size does not match file size (%d vs. %d) (%s)\n", bmpHeader.fileSize, length, name );
	}
	if ( bmpHeader.compression != 0 )
	{
		ri.Error( ERR_DROP, "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.bitsPerPixel < 8 )
	{
		ri.Error( ERR_DROP, "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name );
	}

	columns = bmpHeader.width;
	rows = bmpHeader.height;
	if ( rows < 0 )
		rows = -rows;
	numPixels = columns * rows;

	if(columns <= 0 || !rows || numPixels > 0x1FFFFFFF // 4*1FFFFFFF == 0x7FFFFFFC < 0x7FFFFFFF
	    || ((numPixels * 4) / columns) / 4 != rows)
	{
	  ri.Error (ERR_DROP, "LoadBMP: %s has an invalid image size\n", name);
	}

	if ( width ) 
		*width = columns;
	if ( height )
		*height = rows;

	bmpRGBA = ri.Malloc( numPixels * 4 );
	*pic = bmpRGBA;


	for ( row = rows-1; row >= 0; row-- )
	{
		pixbuf = bmpRGBA + row*columns*4;

		for ( column = 0; column < columns; column++ )
		{
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;

			switch ( bmpHeader.bitsPerPixel )
			{
			case 8:
				palIndex = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;
			case 16:
				shortPixel = * ( unsigned short * ) pixbuf;
				pixbuf += 2;
				*pixbuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
				*pixbuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
				*pixbuf++ = ( shortPixel & ( 31 ) ) << 3;
				*pixbuf++ = 0xff;
				break;

			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;
			default:
				ri.Error( ERR_DROP, "LoadBMP: illegal pixel_size '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name );
				break;
			}
		}
	}

	ri.FS_FreeFile( buffer );

}


/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
static void LoadPCX ( const char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;
	unsigned		xmax, ymax;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_ReadFile( ( char * ) filename, (void **)&raw);
	if (!raw) {
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;
	raw = &pcx->data;

  	xmax = LittleShort(pcx->xmax);
    ymax = LittleShort(pcx->ymax);

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| xmax >= 1024
		|| ymax >= 1024)
	{
		ri.Printf (PRINT_ALL, "Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xmax+1, ymax+1, pcx->xmax, pcx->ymax);
		return;
	}

	out = ri.Malloc ( (ymax+1) * (xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = ri.Malloc(768);
		Com_Memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = xmax+1;
	if (height)
		*height = ymax+1;
// FIXME: use bytes_per_line here?

	for (y=0 ; y<=ymax ; y++, pix += xmax+1)
	{
		for (x=0 ; x<=xmax ; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		ri.Printf (PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		ri.Free (*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile (pcx);
}


/*
==============
LoadPCX32
==============
*/
static void LoadPCX32 ( const char *filename, byte **pic, int *width, int *height) {
	byte	*palette;
	byte	*pic8;
	int		i, c, p;
	byte	*pic32;

	LoadPCX (filename, &pic8, &palette, width, height);
	if (!pic8) {
		*pic = NULL;
		return;
	}

	// LoadPCX32 ensures width, height < 1024
	c = (*width) * (*height);
	pic32 = *pic = ri.Malloc(4 * c );
	for (i = 0 ; i < c ; i++) {
		p = pic8[i];
		pic32[0] = palette[p*3];
		pic32[1] = palette[p*3 + 1];
		pic32[2] = palette[p*3 + 2];
		pic32[3] = 255;
		pic32 += 4;
	}

	ri.Free (pic8);
	ri.Free (palette);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
static void LoadTGA ( const char *name, byte **pic, int *width, int *height)
{
	unsigned	columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	TargaHeader	targa_header;
	byte		*targa_rgba;

	*pic = NULL;

	//
	// load the file
	//
	ri.FS_ReadFile ( ( char * ) name, (void **)&buffer);
	if (!buffer) {
		return;
	}

	buf_p = buffer;

	targa_header.id_length = buf_p[0];
	targa_header.colormap_type = buf_p[1];
	targa_header.image_type = buf_p[2];
	
	memcpy(&targa_header.colormap_index, &buf_p[3], 2);
	memcpy(&targa_header.colormap_length, &buf_p[5], 2);
	targa_header.colormap_size = buf_p[7];
	memcpy(&targa_header.x_origin, &buf_p[8], 2);
	memcpy(&targa_header.y_origin, &buf_p[10], 2);
	memcpy(&targa_header.width, &buf_p[12], 2);
	memcpy(&targa_header.height, &buf_p[14], 2);
	targa_header.pixel_size = buf_p[16];
	targa_header.attributes = buf_p[17];

	targa_header.colormap_index = LittleShort(targa_header.colormap_index);
	targa_header.colormap_length = LittleShort(targa_header.colormap_length);
	targa_header.x_origin = LittleShort(targa_header.x_origin);
	targa_header.y_origin = LittleShort(targa_header.y_origin);
	targa_header.width = LittleShort(targa_header.width);
	targa_header.height = LittleShort(targa_header.height);

	buf_p += 18;

	if (targa_header.image_type!=2 
		&& targa_header.image_type!=10
		&& targa_header.image_type != 3 ) 
	{
		ri.Error (ERR_DROP, "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if ( targa_header.colormap_type != 0 )
	{
		ri.Error( ERR_DROP, "LoadTGA: colormaps not supported\n" );
	}

	if ( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 )
	{
		ri.Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows * 4;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	if(!columns || !rows || numPixels > 0x7FFFFFFF || numPixels / columns / 4 != rows)
	{
		ri.Error (ERR_DROP, "LoadTGA: %s has an invalid image size\n", name);
	}

	targa_rgba = ri.Malloc (numPixels);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment
	
	if ( targa_header.image_type==2 || targa_header.image_type == 3 )
	{ 
		// Uncompressed RGB or gray scale image
		for(row=rows-1; row>=0; row--) 
		{
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) 
			{
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) 
				{
					
				case 8:
					blue = *buf_p++;
					green = blue;
					red = blue;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;

				case 24:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				default:
					ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
					break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;

		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;

		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
						default:
							ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
							break;
					}
	
					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
							default:
								ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
								break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}

#if 0 
  // TTimo: this is the chunk of code to ensure a behavior that meets TGA specs 
  // bit 5 set => top-down
  if (targa_header.attributes & 0x20) {
    unsigned char *flip = (unsigned char*)malloc (columns*4);
    unsigned char *src, *dst;

    for (row = 0; row < rows/2; row++) {
      src = targa_rgba + row * 4 * columns;
      dst = targa_rgba + (rows - row - 1) * 4 * columns;

      memcpy (flip, src, columns*4);
      memcpy (src, dst, columns*4);
      memcpy (dst, flip, columns*4);
    }
    free (flip);
  }
#endif
  // instead we just print a warning
  if (targa_header.attributes & 0x20) {
    ri.Printf( PRINT_WARNING, "WARNING: '%s' TGA file header declares top-down image, ignoring\n", name);
  }

  ri.FS_FreeFile (buffer);
}

static void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height ) {
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo = {NULL};
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPARRAY buffer;		/* Output row buffer */
  unsigned row_stride;		/* physical row width in output buffer */
  unsigned pixelcount, memcount;
  unsigned char *out;
  byte	*fbuffer;
  byte  *buf;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  ri.FS_ReadFile ( ( char * ) filename, (void **)&fbuffer);
  if (!fbuffer) {
	return;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, fbuffer);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */

  pixelcount = cinfo.output_width * cinfo.output_height;

  if(!cinfo.output_width || !cinfo.output_height
      || ((pixelcount * 4) / cinfo.output_width) / 4 != cinfo.output_height
      || pixelcount > 0x1FFFFFFF || cinfo.output_components > 4) // 4*1FFFFFFF == 0x7FFFFFFC < 0x7FFFFFFF
  {
    ri.Error (ERR_DROP, "LoadJPG: %s has an invalid image size: %dx%d*4=%d, components: %d\n", filename,
		    cinfo.output_width, cinfo.output_height, pixelcount * 4, cinfo.output_components);
  }

  memcount = pixelcount * 4;
  row_stride = cinfo.output_width * cinfo.output_components;

  out = ri.Malloc(memcount);

  *width = cinfo.output_width;
  *height = cinfo.output_height;

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
	buf = ((out+(row_stride*cinfo.output_scanline)));
	buffer = &buf;
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
  }
  
  buf = out;

  // If we are processing an 8-bit JPEG (greyscale), we'll have to convert
  // the greyscale values to RGBA.
  if(cinfo.output_components == 1)
  {
  	int sindex = pixelcount, dindex = memcount;
	unsigned char greyshade;

	// Only pixelcount number of bytes have been written.
	// Expand the color values over the rest of the buffer, starting
	// from the end.
	do
	{
		greyshade = buf[--sindex];

		buf[--dindex] = 255;
		buf[--dindex] = greyshade;
		buf[--dindex] = greyshade;
		buf[--dindex] = greyshade;
	} while(sindex);
  }
  else
  {
	// clear all the alphas to 255
	int	i;

	for ( i = 3 ; i < memcount ; i+=4 )
	{
		buf[i] = 255;
	}
  }

  *pic = out;

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  ri.FS_FreeFile (fbuffer);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
}


/* Expanded data destination object for stdio output */

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  byte* outfile;		/* target stream */
  int	size;
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

void init_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  dest->pub.next_output_byte = dest->outfile;
  dest->pub.free_in_buffer = dest->size;
}


/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

boolean empty_output_buffer (j_compress_ptr cinfo)
{
  return TRUE;
}


/*
 * Compression initialization.
 * Before calling this, all parameters and a data destination must be set up.
 *
 * We require a write_all_tables parameter as a failsafe check when writing
 * multiple datastreams from the same compression object.  Since prior runs
 * will have left all the tables marked sent_table=TRUE, a subsequent run
 * would emit an abbreviated stream (no tables) by default.  This may be what
 * is wanted, but for safety's sake it should not be the default behavior:
 * programmers should have to make a deliberate choice to emit abbreviated
 * images.  Therefore the documentation and examples should encourage people
 * to pass write_all_tables=TRUE; then it will take active thought to do the
 * wrong thing.
 */

GLOBAL void
jpeg_start_compress (j_compress_ptr cinfo, boolean write_all_tables)
{
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  if (write_all_tables)
    jpeg_suppress_tables(cinfo, FALSE);	/* mark all tables to be written */

  /* (Re)initialize error mgr and destination modules */
  (*cinfo->err->reset_error_mgr) ((j_common_ptr) cinfo);
  (*cinfo->dest->init_destination) (cinfo);
  /* Perform master selection of active modules */
  jinit_compress_master(cinfo);
  /* Set up for the first pass */
  (*cinfo->master->prepare_for_pass) (cinfo);
  /* Ready for application to drive first pass through jpeg_write_scanlines
   * or jpeg_write_raw_data.
   */
  cinfo->next_scanline = 0;
  cinfo->global_state = (cinfo->raw_data_in ? CSTATE_RAW_OK : CSTATE_SCANNING);
}


/*
 * Write some scanlines of data to the JPEG compressor.
 *
 * The return value will be the number of lines actually written.
 * This should be less than the supplied num_lines only in case that
 * the data destination module has requested suspension of the compressor,
 * or if more than image_height scanlines are passed in.
 *
 * Note: we warn about excess calls to jpeg_write_scanlines() since
 * this likely signals an application programmer error.  However,
 * excess scanlines passed in the last valid call are *silently* ignored,
 * so that the application need not adjust num_lines for end-of-image
 * when using a multiple-scanline buffer.
 */

GLOBAL JDIMENSION
jpeg_write_scanlines (j_compress_ptr cinfo, JSAMPARRAY scanlines,
		      JDIMENSION num_lines)
{
  JDIMENSION row_ctr, rows_left;

  if (cinfo->global_state != CSTATE_SCANNING)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  if (cinfo->next_scanline >= cinfo->image_height)
    WARNMS(cinfo, JWRN_TOO_MUCH_DATA);

  /* Call progress monitor hook if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->pass_counter = (long) cinfo->next_scanline;
    cinfo->progress->pass_limit = (long) cinfo->image_height;
    (*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
  }

  /* Give master control module another chance if this is first call to
   * jpeg_write_scanlines.  This lets output of the frame/scan headers be
   * delayed so that application can write COM, etc, markers between
   * jpeg_start_compress and jpeg_write_scanlines.
   */
  if (cinfo->master->call_pass_startup)
    (*cinfo->master->pass_startup) (cinfo);

  /* Ignore any extra scanlines at bottom of image. */
  rows_left = cinfo->image_height - cinfo->next_scanline;
  if (num_lines > rows_left)
    num_lines = rows_left;

  row_ctr = 0;
  (*cinfo->main->process_data) (cinfo, scanlines, &row_ctr, num_lines);
  cinfo->next_scanline += row_ctr;
  return row_ctr;
}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

static int hackSize;

void term_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
  size_t datacount = dest->size - dest->pub.free_in_buffer;
  hackSize = datacount;
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */

void jpegDest (j_compress_ptr cinfo, byte* outfile, int size)
{
  my_dest_ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(my_destination_mgr));
  }

  dest = (my_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outfile = outfile;
  dest->size = size;
}

void SaveJPG(char * filename, int quality, int image_width, int image_height, unsigned char *image_buffer) {
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */
  unsigned char *out;

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  out = ri.Hunk_AllocateTempMemory(image_width*image_height*4);
  jpegDest(&cinfo, out, image_width*image_height*4);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = image_width; 	/* image width and height, in pixels */
  cinfo.image_height = image_height;
  cinfo.input_components = 4;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
  /* If quality is set high, disable chroma subsampling */
  if (quality >= 85) {
    cinfo.comp_info[0].h_samp_factor = 1;
    cinfo.comp_info[0].v_samp_factor = 1;
  }

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = image_width * 4;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & image_buffer[((cinfo.image_height-1)*row_stride)-cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  /* After finish_compress, we can close the output file. */
  ri.FS_WriteFile( filename, out, hackSize );

  ri.Hunk_FreeTempMemory(out);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */
}

/*
=================
SaveJPGToBuffer
=================
*/
int SaveJPGToBuffer( byte *buffer, int quality,
    int image_width, int image_height,
    byte *image_buffer )
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  /* Step 1: allocate and initialize JPEG compression object */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */
  jpegDest(&cinfo, buffer, image_width*image_height*4);

  /* Step 3: set parameters for compression */
  cinfo.image_width = image_width; 	/* image width and height, in pixels */
  cinfo.image_height = image_height;
  cinfo.input_components = 4;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
  /* If quality is set high, disable chroma subsampling */
  if (quality >= 85) {
    cinfo.comp_info[0].h_samp_factor = 1;
    cinfo.comp_info[0].v_samp_factor = 1;
  }

  /* Step 4: Start compressor */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */
  row_stride = image_width * 4;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & image_buffer[((cinfo.image_height-1)*row_stride)-cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */
  jpeg_finish_compress(&cinfo);

  /* Step 7: release JPEG compression object */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */
  return hackSize;
}

//===================================================================

/*
=================
PNG LOADING
=================
*/

/*
 *  Quake 3 image format : RGBA
 */

#define Q3IMAGE_BYTESPERPIXEL (4)

/*
 *  PNG specifications
 */

/*
 *  The first 8 Bytes of every PNG-File are a fixed signature
 *  to identify the file as a PNG.
 */

#define PNG_Signature "\x89\x50\x4E\x47\xD\xA\x1A\xA"
#define PNG_Signature_Size (8)

/*
 *  After the signature diverse chunks follow.
 *  A chunk consists of a header and if Length
 *  is bigger than 0 a body and a CRC of the body follow.
 */

struct PNG_ChunkHeader
{
    uint32_t Length;
    uint32_t Type;
};

#define PNG_ChunkHeader_Size (8)

typedef uint32_t PNG_ChunkCRC;

#define PNG_ChunkCRC_Size (4)

/*
 *  We use the following ChunkTypes.
 *  All others are ignored.
 */

#define MAKE_CHUNKTYPE(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | ((d)))

#define PNG_ChunkType_IHDR MAKE_CHUNKTYPE('I', 'H', 'D', 'R')
#define PNG_ChunkType_PLTE MAKE_CHUNKTYPE('P', 'L', 'T', 'E')
#define PNG_ChunkType_IDAT MAKE_CHUNKTYPE('I', 'D', 'A', 'T')
#define PNG_ChunkType_IEND MAKE_CHUNKTYPE('I', 'E', 'N', 'D')
#define PNG_ChunkType_tRNS MAKE_CHUNKTYPE('t', 'R', 'N', 'S')

/*
 *  Per specification the first chunk after the signature SHALL be IHDR.
 */

struct PNG_Chunk_IHDR
{
    uint32_t Width;
    uint32_t Height;
    uint8_t  BitDepth;
    uint8_t  ColourType;
    uint8_t  CompressionMethod;
    uint8_t  FilterMethod;
    uint8_t  InterlaceMethod;
};

#define PNG_Chunk_IHDR_Size (13)

/*
 *  ColourTypes
 */

#define PNG_ColourType_Grey      (0)
#define PNG_ColourType_True      (2)
#define PNG_ColourType_Indexed   (3)
#define PNG_ColourType_GreyAlpha (4)
#define PNG_ColourType_TrueAlpha (6)

/*
 *  number of colour components
 *
 *  Grey      : 1 grey
 *  True      : 1 R, 1 G, 1 B
 *  Indexed   : 1 index
 *  GreyAlpha : 1 grey, 1 alpha
 *  TrueAlpha : 1 R, 1 G, 1 B, 1 alpha
 */

#define PNG_NumColourComponents_Grey      (1)
#define PNG_NumColourComponents_True      (3)
#define PNG_NumColourComponents_Indexed   (1)
#define PNG_NumColourComponents_GreyAlpha (2)
#define PNG_NumColourComponents_TrueAlpha (4)

/*
 *  For the different ColourTypes
 *  different BitDepths are specified.
 */

#define PNG_BitDepth_1  ( 1)
#define PNG_BitDepth_2  ( 2)
#define PNG_BitDepth_4  ( 4)
#define PNG_BitDepth_8  ( 8)
#define PNG_BitDepth_16 (16)

/*
 *  Only one valid CompressionMethod is standardized.
 */

#define PNG_CompressionMethod_0 (0)

/*
 *  Only one valid FilterMethod is currently standardized.
 */

#define PNG_FilterMethod_0 (0)

/*
 *  This FilterMethod defines 5 FilterTypes
 */

#define PNG_FilterType_None    (0)
#define PNG_FilterType_Sub     (1)
#define PNG_FilterType_Up      (2)
#define PNG_FilterType_Average (3)
#define PNG_FilterType_Paeth   (4)

/*
 *  Two InterlaceMethods are standardized :
 *  0 - NonInterlaced
 *  1 - Interlaced
 */

#define PNG_InterlaceMethod_NonInterlaced (0)
#define PNG_InterlaceMethod_Interlaced    (1)

/*
 *  The Adam7 interlace method uses 7 passes.
 */

#define PNG_Adam7_NumPasses (7)

/*
 *  The compressed data starts with a header ...
 */

struct PNG_ZlibHeader
{
    uint8_t CompressionMethod;
    uint8_t Flags;
};

#define PNG_ZlibHeader_Size (2)

/*
 *  ... and is followed by a check value
 */

#define PNG_ZlibCheckValue_Size (4)

/*
 *  Some support functions for buffered files follow.
 */

/*
 *  buffered file representation
 */

struct BufferedFile
{
    byte *Buffer;
    int   Length;
    byte *Ptr;
    int   BytesLeft;
};

/*
 *  Read a file into a buffer.
 */

static struct BufferedFile *ReadBufferedFile(const char *name)
{
    struct BufferedFile *BF;

    /*
     *  input verification
     */

    if(!name)
    {
        return(NULL);
    }

    /*
     *  Allocate control struct.
     */

    BF = ri.Malloc(sizeof(struct BufferedFile));
    if(!BF)
    {
        return(NULL);
    }

    /*
     *  Initialize the structs components.
     */

    BF->Length    = 0;
    BF->Buffer    = NULL;
    BF->Ptr       = NULL;
    BF->BytesLeft = 0;

    /*
     *  Read the file.
     */

    BF->Length = ri.FS_ReadFile((char *) name, (void **) &BF->Buffer);

    /*
     *  Did we get it? Is it big enough?
     */

    if(!(BF->Buffer && (BF->Length > 0)))
    {
        ri.Free(BF);

        return(NULL);
    }

    /*
     *  Set the pointers and counters.
     */

    BF->Ptr       = BF->Buffer;
    BF->BytesLeft = BF->Length;

    return(BF);
}

/*
 *  Close a buffered file.
 */

static void CloseBufferedFile(struct BufferedFile *BF)
{
    if(BF)
    {
        if(BF->Buffer)
        {
            ri.FS_FreeFile(BF->Buffer);
        }
 
        ri.Free(BF);
    }
}

/*
 *  Get a pointer to the requested bytes.
 */

static void *BufferedFileRead(struct BufferedFile *BF, int Length)
{
    void *RetVal;

    /*
     *  input verification
     */

    if(!(BF && Length))
    {
        return(NULL);
    }
 
    /*
     *  not enough bytes left
     */

    if(Length > BF->BytesLeft)
    {
        return(NULL);
    }

    /*
     *  the pointer to the requested data
     */

    RetVal = BF->Ptr;
 
    /*
     *  Raise the pointer and counter.
     */

    BF->Ptr       += Length;
    BF->BytesLeft -= Length;

    return(RetVal);
}

/*
 *  Rewind the buffer.
 */

static qboolean BufferedFileRewind(struct BufferedFile *BF, int Offset)
{
    int BytesRead; 

    /*
     *  input verification
     */

    if(!BF)
    {
        return(qfalse);
    }

    /*
     *  special trick to rewind to the beginning of the buffer
     */

    if(Offset == -1)
    {
        BF->Ptr       = BF->Buffer;
        BF->BytesLeft = BF->Length;
  
        return(qtrue);
    }

    /*
     *  How many bytes do we have already read?
     */

    BytesRead = BF->Ptr - BF->Buffer;

    /*
     *  We can only rewind to the beginning of the BufferedFile.
     */

    if(Offset > BytesRead)
    {
        return(qfalse);
    }

    /*
     *  lower the pointer and counter.
     */

    BF->Ptr       -= Offset;
    BF->BytesLeft += Offset;

    return(qtrue);
}

/*
 *  Skip some bytes.
 */

static qboolean BufferedFileSkip(struct BufferedFile *BF, int Offset)
{
    /*
     *  input verification
     */

    if(!BF)
    {
        return(qfalse);
    }
 
    /*
     *  We can only skip to the end of the BufferedFile.
     */

    if(Offset > BF->BytesLeft)
    {
        return(qfalse);
    }

    /*
     *  lower the pointer and counter.
     */

    BF->Ptr       += Offset;
    BF->BytesLeft -= Offset;

    return(qtrue);
}

/*
 *  Find a chunk
 */

static qboolean FindChunk(struct BufferedFile *BF, uint32_t ChunkType)
{
    struct PNG_ChunkHeader *CH;

    uint32_t Length;
    uint32_t Type;

    /*
     *  input verification
     */

    if(!BF)
    {
        return(qfalse);
    }

    /*
     *  cycle trough the chunks
     */

    while(qtrue)
    {
        /*
         *  Read the chunk-header.
         */

        CH = BufferedFileRead(BF, PNG_ChunkHeader_Size);
        if(!CH)
        {
            return(qfalse);
        }

        /*
         *  Do not swap the original types
         *  they might be needed later.
         */

        Length = BigLong(CH->Length);
        Type   = BigLong(CH->Type);
  
        /*
         *  We found it!
         */

        if(Type == ChunkType)
        {
            /*
             *  Rewind to the start of the chunk.
             */
	     
            BufferedFileRewind(BF, PNG_ChunkHeader_Size);
  
            break;
        }
        else
        {
            /*
             *  Skip the rest of the chunk.
             */

            if(Length)
            {
                if(!BufferedFileSkip(BF, Length + PNG_ChunkCRC_Size))
                {
                    return(qfalse);
                }  
            }
        }
    }

    return(qtrue);
}

/*
 *  Decompress all IDATs
 */

static uint32_t DecompressIDATs(struct BufferedFile *BF, uint8_t **Buffer)
{
    uint8_t  *DecompressedData;
    uint32_t  DecompressedDataLength;

    uint8_t  *CompressedData;
    uint8_t  *CompressedDataPtr;
    uint32_t  CompressedDataLength;

    struct PNG_ChunkHeader *CH;

    uint32_t Length;
    uint32_t Type;

    int BytesToRewind;

    int32_t   puffResult;
    uint8_t  *puffDest;
    uint32_t  puffDestLen;
    uint8_t  *puffSrc;
    uint32_t  puffSrcLen;

    /*
     *  input verification
     */

    if(!(BF && Buffer))
    {
        return(-1);
    }

    /*
     *  some zeroing
     */

    DecompressedData = NULL;
    DecompressedDataLength = 0;
    *Buffer = DecompressedData;

    CompressedData = NULL;
    CompressedDataLength = 0;

    BytesToRewind = 0;

    /*
     *  Find the first IDAT chunk.
     */

    if(!FindChunk(BF, PNG_ChunkType_IDAT))
    {
        return(-1);
    }

    /*
     *  Count the size of the uncompressed data
     */

    while(qtrue)
    {
        /*
         *  Read chunk header
         */

        CH = BufferedFileRead(BF, PNG_ChunkHeader_Size);
        if(!CH)
        {
            /*
             *  Rewind to the start of this adventure
             *  and return unsuccessfull
             */

            BufferedFileRewind(BF, BytesToRewind);

            return(-1);
        }

        /*
         *  Length and Type of chunk
         */

        Length = BigLong(CH->Length);
        Type   = BigLong(CH->Type);

        /*
         *  We have reached the end of the IDAT chunks
         */

        if(!(Type == PNG_ChunkType_IDAT))
        {
            BufferedFileRewind(BF, PNG_ChunkHeader_Size); 
  
            break;
        }

        /*
         *  Add chunk header to count.
         */

        BytesToRewind += PNG_ChunkHeader_Size;

        /*
         *  Skip to next chunk
         */

        if(Length)
        {
            if(!BufferedFileSkip(BF, Length + PNG_ChunkCRC_Size))
            {
                BufferedFileRewind(BF, BytesToRewind);

                return(-1);
            }

            BytesToRewind += Length + PNG_ChunkCRC_Size;
            CompressedDataLength += Length;
        } 
    }

    BufferedFileRewind(BF, BytesToRewind);

    CompressedData = ri.Malloc(CompressedDataLength);
    if(!CompressedData)
    {
        return(-1);
    }
 
    CompressedDataPtr = CompressedData;

    /*
     *  Collect the compressed Data
     */

    while(qtrue)
    {
        /*
         *  Read chunk header
         */

        CH = BufferedFileRead(BF, PNG_ChunkHeader_Size);
        if(!CH)
        {
            ri.Free(CompressedData); 
  
            return(-1);
        }

        /*
         *  Length and Type of chunk
         */

        Length = BigLong(CH->Length);
        Type   = BigLong(CH->Type);

        /*
         *  We have reached the end of the IDAT chunks
         */

        if(!(Type == PNG_ChunkType_IDAT))
        {
            BufferedFileRewind(BF, PNG_ChunkHeader_Size); 
  
            break;
        }

        /*
         *  Copy the Data
         */

        if(Length)
        {
            uint8_t *OrigCompressedData;
   
            OrigCompressedData = BufferedFileRead(BF, Length);
            if(!OrigCompressedData)
            {
                ri.Free(CompressedData); 
  
                return(-1);
            }

            if(!BufferedFileSkip(BF, PNG_ChunkCRC_Size))
            {
                ri.Free(CompressedData); 

                return(-1);
            }
  
            memcpy(CompressedDataPtr, OrigCompressedData, Length);
            CompressedDataPtr += Length;
        } 
    }

    /*
     *  Let puff() calculate the decompressed data length.
     */

    puffDest    = NULL;
    puffDestLen = 0;
 
    /*
     *  The zlib header and checkvalue don't belong to the compressed data.
     */

    puffSrc    = CompressedData + PNG_ZlibHeader_Size;
    puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;

    /*
     *  first puff() to calculate the size of the uncompressed data
     */

    puffResult = puff(puffDest, &puffDestLen, puffSrc, &puffSrcLen);
    if(!((puffResult == 0) && (puffDestLen > 0)))
    {
        ri.Free(CompressedData);
 
        return(-1);
    }

    /*
     *  Allocate the buffer for the uncompressed data.
     */

    DecompressedData = ri.Malloc(puffDestLen);
    if(!DecompressedData)
    {
        ri.Free(CompressedData);
 
        return(-1);
    }

    /*
     *  Set the input again in case something was changed by the last puff() .
     */

    puffDest   = DecompressedData;
    puffSrc    = CompressedData + PNG_ZlibHeader_Size;
    puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;
 
    /*
     *  decompression puff()
     */

    puffResult = puff(puffDest, &puffDestLen, puffSrc, &puffSrcLen);

    /*
     *  The compressed data is not needed anymore.
     */

    ri.Free(CompressedData);

    /*
     *  Check if the last puff() was successfull.
     */

    if(!((puffResult == 0) && (puffDestLen > 0)))
    {
        ri.Free(DecompressedData);
 
        return(-1);
    }

    /*
     *  Set the output of this function.
     */

    DecompressedDataLength = puffDestLen;
    *Buffer = DecompressedData;

    return(DecompressedDataLength);
}

/*
 *  the Paeth predictor
 */

static uint8_t PredictPaeth(uint8_t a, uint8_t b, uint8_t c)
{
    /*
     *  a == Left
     *  b == Up
     *  c == UpLeft
     */

    uint8_t Pr;
    int p;
    int pa, pb, pc;

    Pr = 0;

    p  = ((int) a) + ((int) b) - ((int) c);
    pa = abs(p - ((int) a));
    pb = abs(p - ((int) b));
    pc = abs(p - ((int) c));

    if((pa <= pb) && (pa <= pc))
    {
        Pr = a;
    }
    else if(pb <= pc)
    {
        Pr = b;
    }
    else
    {
        Pr = c;
    }

    return(Pr);

}

/*
 *  Reverse the filters.
 */

static qboolean UnfilterImage(uint8_t  *DecompressedData, 
                              uint32_t  ImageHeight,
		              uint32_t  BytesPerScanline, 
		              uint32_t  BytesPerPixel)
{
    uint8_t   *DecompPtr;
    uint8_t   FilterType;
    uint8_t  *PixelLeft, *PixelUp, *PixelUpLeft;
    uint32_t  w, h, p;

    /*
     *  some zeros for the filters
     */

    uint8_t Zeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    /*
     *  input verification
     *
     *  ImageHeight and BytesPerScanline are not checked,
     *  because these can be zero in some interlace passes.
     */

    if(!(DecompressedData && BytesPerPixel))
    {
	return(qfalse);
    }


    /*
     *  Set the pointer to the start of the decompressed Data.
     */

    DecompPtr = DecompressedData;

    /*
     *  Un-filtering is done in place.
     */

    /*
     *  Go trough all scanlines.
     */

    for(h = 0; h < ImageHeight; h++)
    {
        /*
         *  Every scanline starts with a FilterType byte.
         */

        FilterType = *DecompPtr;
        DecompPtr++;

        /*
         *  Left pixel of the first byte in a scanline is zero.
         */

        PixelLeft = Zeros;

        /*
         *  Set PixelUp to previous line only if we are on the second line or above.
         *
         *  Plus one byte for the FilterType
         */

        if(h > 0)
        {
            PixelUp = DecompPtr - (BytesPerScanline + 1);
        }
        else
        {
            PixelUp = Zeros;
        }

        /*
         * The pixel left to the first pixel of the previous scanline is zero too.
         */

        PixelUpLeft = Zeros;

        /*
         *  Cycle trough all pixels of the scanline.
         */

        for(w = 0; w < (BytesPerScanline / BytesPerPixel); w++)
        {
            /*
             *  Cycle trough the bytes of the pixel.
             */

            for(p = 0; p < BytesPerPixel; p++)
            {
                switch(FilterType)
                { 
                    case PNG_FilterType_None :
                    {
                        /*
                         *  The byte is unfiltered.
                         */

                        break;
                    }

                    case PNG_FilterType_Sub :
                    {
                        DecompPtr[p] += PixelLeft[p];

                        break;
                    }

		    case PNG_FilterType_Up :
                    {
                        DecompPtr[p] += PixelUp[p];

                        break;
                    }

                    case PNG_FilterType_Average :
                    {
                        DecompPtr[p] += ((uint8_t) ((((uint16_t) PixelLeft[p]) + ((uint16_t) PixelUp[p])) / 2));

                        break;
                    }

                    case PNG_FilterType_Paeth :
                    {
                        DecompPtr[p] += PredictPaeth(PixelLeft[p], PixelUp[p], PixelUpLeft[p]);

                        break;
                    }

                    default :
                    {
                        return(qfalse);
                    }
                }
            }
   
            PixelLeft = DecompPtr;

            /*
             *  We only have a upleft pixel if we are on the second line or above.
             */

            if(h > 0)
            {
                PixelUpLeft = DecompPtr - (BytesPerScanline + 1);
            }

	    /*
             *  Skip to the next pixel.
             */

            DecompPtr += BytesPerPixel;
	 
            /*
             *  We only have a previous line if we are on the second line and above.
             */

            if(h > 0)
            {
                PixelUp = DecompPtr - (BytesPerScanline + 1);
            }
        }
    }

 return(qtrue);
}

/*
 *  Convert a raw input pixel to Quake 3 RGA format.
 */

static qboolean ConvertPixel(struct PNG_Chunk_IHDR *IHDR,
			     byte                  *OutPtr,
			     uint8_t               *DecompPtr,
                             qboolean               HasTransparentColour,
                             uint8_t               *TransparentColour,
                             uint8_t               *OutPal)
{
    /*
     *  input verification
     */
    
    if(!(IHDR && OutPtr && DecompPtr && TransparentColour && OutPal))
    {
     return(qfalse);
    }

    switch(IHDR->ColourType)
    {
        case PNG_ColourType_Grey :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
    		    uint8_t Step;
                    uint8_t GreyValue;

                    Step = 0xFF / ((1 << IHDR->BitDepth) - 1);

                    GreyValue = DecompPtr[0] * Step;
  
                    OutPtr[0] = GreyValue;
                    OutPtr[1] = GreyValue;
                    OutPtr[2] = GreyValue;
                    OutPtr[3] = 0xFF;

                    /*
                     *  Grey supports full transparency for one specified colour
                     */

                    if(HasTransparentColour)
                    {
                        if(TransparentColour[1] == DecompPtr[0])
                        {
                            OutPtr[3] = 0x00;
                        }
                    }
	

                    break;
                }
      
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = 0xFF;
      
                    /*
                     *  Grey supports full transparency for one specified colour
                     */

                    if(HasTransparentColour)
                    {
                        if(IHDR->BitDepth == PNG_BitDepth_8)
                        {
                            if(TransparentColour[1] == DecompPtr[0])
                            {
                                OutPtr[3] = 0x00;
                            }
                        }
                        else
                        {
                            if((TransparentColour[0] == DecompPtr[0]) && (TransparentColour[1] == DecompPtr[1]))
                            {
                                OutPtr[3] = 0x00;
                            }
                        }
                    }

                    break;
                }
      
                default :
                {
                    return(qfalse);
                }
            }
    
            break;
        }

        case PNG_ColourType_True :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[1];
                    OutPtr[2] = DecompPtr[2];
                    OutPtr[3] = 0xFF;
      
                    /*
                     *  True supports full transparency for one specified colour
                     */

                    if(HasTransparentColour)
                    {
                        if((TransparentColour[1] == DecompPtr[0]) &&
                           (TransparentColour[3] == DecompPtr[1]) &&
                           (TransparentColour[5] == DecompPtr[3]))
                        {
                            OutPtr[3] = 0x00;
                        }
                    }

                    break;
                }
      
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */

                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[2];
                    OutPtr[2] = DecompPtr[4];
                    OutPtr[3] = 0xFF;
      
                    /*
                     *  True supports full transparency for one specified colour
                     */

                    if(HasTransparentColour)
                    {
                        if((TransparentColour[0] == DecompPtr[0]) && (TransparentColour[1] == DecompPtr[1]) &&
                           (TransparentColour[2] == DecompPtr[2]) && (TransparentColour[3] == DecompPtr[3]) &&
                           (TransparentColour[4] == DecompPtr[4]) && (TransparentColour[5] == DecompPtr[5]))
                        {
                            OutPtr[3] = 0x00;
                        }
                    }

                    break;
                }

                default :
                {
                    return(qfalse);
                }
            }

            break;
        }

        case PNG_ColourType_Indexed :
        {
            OutPtr[0] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 0];
            OutPtr[1] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 1];
            OutPtr[2] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 2];
            OutPtr[3] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 3];
	
            break;
        }

        case PNG_ColourType_GreyAlpha :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = DecompPtr[1];
      
                    break;
                }
  
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */

                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = DecompPtr[2];
      
                    break;
                }

                default :
                {
                    return(qfalse);
                }
            }

            break;
        }

        case PNG_ColourType_TrueAlpha :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[1];
                    OutPtr[2] = DecompPtr[2];
                    OutPtr[3] = DecompPtr[3];
      
                    break;
                }
      
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */

                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[2];
                    OutPtr[2] = DecompPtr[4];
                    OutPtr[3] = DecompPtr[6];
      
                    break;
                }

                default :
                {
                    return(qfalse);
                }
            }

            break;
        }

        default :
        {
            return(qfalse);
        }
    }

    return(qtrue);
}


/*
 *  Decode a non-interlaced image.
 */

static qboolean DecodeImageNonInterlaced(struct PNG_Chunk_IHDR *IHDR,
                                         byte                  *OutBuffer, 
                                         uint8_t               *DecompressedData,
                                         uint32_t               DecompressedDataLength,
                                         qboolean               HasTransparentColour,
                                         uint8_t               *TransparentColour,
                                         uint8_t               *OutPal)
{
    uint32_t IHDR_Width;
    uint32_t IHDR_Height;
    uint32_t BytesPerScanline, BytesPerPixel, PixelsPerByte;
    uint32_t  w, h, p;
    byte *OutPtr;
    uint8_t *DecompPtr;

    /*
     *  input verification
     */

    if(!(IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal))
    {
	return(qfalse);
    }

    /*
     *  byte swapping
     */
     
    IHDR_Width  = BigLong(IHDR->Width);
    IHDR_Height = BigLong(IHDR->Height);

    /*
     *  information for un-filtering
     */

    switch(IHDR->ColourType)
    {
        case PNG_ColourType_Grey :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;

                    break;
                }

                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_Grey;
                    PixelsPerByte    = 1;

                    break;
                }

                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }
  
        case PNG_ColourType_True :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_True;
                    PixelsPerByte    = 1;

                    break;
                }
     
                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }

        case PNG_ColourType_Indexed :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;

                    break;
                }

                case PNG_BitDepth_8 :
                {
                    BytesPerPixel    = PNG_NumColourComponents_Indexed;
                    PixelsPerByte    = 1;

                    break;
                }
         
                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }

        case PNG_ColourType_GreyAlpha :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_GreyAlpha;
                    PixelsPerByte    = 1;

                    break;
                }
     
                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }

        case PNG_ColourType_TrueAlpha :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_TrueAlpha;
                    PixelsPerByte    = 1;

                    break;
                }
    
                default :
                {
                    return(qfalse);
                }
            }

            break;
        }

        default :
        {
            return(qfalse);
        }
    }

    /*
     *  Calculate the size of one scanline
     */

    BytesPerScanline = (IHDR_Width * BytesPerPixel + (PixelsPerByte - 1)) / PixelsPerByte;

    /*
     *  Check if we have enough data for the whole image.
     */

    if(!(DecompressedDataLength == ((BytesPerScanline + 1) * IHDR_Height)))
    {
        return(qfalse);
    }

    /*
     *  Unfilter the image.
     */

    if(!UnfilterImage(DecompressedData, IHDR_Height, BytesPerScanline, BytesPerPixel))
    {
        return(qfalse);
    }

    /*
     *  Set the working pointers to the beginning of the buffers.
     */

    OutPtr = OutBuffer;
    DecompPtr = DecompressedData;

    /*
     *  Create the output image.
     */

    for(h = 0; h < IHDR_Height; h++)
    {
        /*
         *  Count the pixels on the scanline for those multipixel bytes
         */

        uint32_t CurrPixel;
  
        /*
         *  skip FilterType
         */

        DecompPtr++;

        /*
         *  Reset the pixel count.
         */

        CurrPixel = 0;

        for(w = 0; w < (BytesPerScanline / BytesPerPixel); w++)
        {
	    if(PixelsPerByte > 1)
	    {
                uint8_t  Mask;
                uint32_t Shift;
		uint8_t  SinglePixel;

                for(p = 0; p < PixelsPerByte; p++)
                {
                    if(CurrPixel < IHDR_Width)
                    {
                        Mask  = (1 << IHDR->BitDepth) - 1;
                        Shift = (PixelsPerByte - 1 - p) * IHDR->BitDepth;

                        SinglePixel = ((DecompPtr[0] & (Mask << Shift)) >> Shift);

			if(!ConvertPixel(IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal))
			{
			    return(qfalse);
			}

                        OutPtr += Q3IMAGE_BYTESPERPIXEL;
                        CurrPixel++;
                    }
                }
	    
	    }
	    else
	    {
		if(!ConvertPixel(IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal))
		{
		    return(qfalse);
		}
  

                OutPtr += Q3IMAGE_BYTESPERPIXEL;
	    }

            DecompPtr += BytesPerPixel;
        }
    }

    return(qtrue);
}

/*
 *  Decode an interlaced image.
 */

static qboolean DecodeImageInterlaced(struct PNG_Chunk_IHDR *IHDR,
                                      byte                  *OutBuffer, 
                                      uint8_t               *DecompressedData,
                                      uint32_t               DecompressedDataLength,
                                      qboolean               HasTransparentColour,
                                      uint8_t               *TransparentColour,
                                      uint8_t               *OutPal)
{
    uint32_t IHDR_Width;
    uint32_t IHDR_Height;
    uint32_t BytesPerScanline[PNG_Adam7_NumPasses], BytesPerPixel, PixelsPerByte;
    uint32_t PassWidth[PNG_Adam7_NumPasses], PassHeight[PNG_Adam7_NumPasses];
    uint32_t WSkip[PNG_Adam7_NumPasses], WOffset[PNG_Adam7_NumPasses], HSkip[PNG_Adam7_NumPasses], HOffset[PNG_Adam7_NumPasses];
    uint32_t w, h, p, a;
    byte *OutPtr;
    uint8_t *DecompPtr;
    uint32_t TargetLength;

    /*
     *  input verification
     */

    if(!(IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal))
    {
	return(qfalse);
    }

    /*
     *  byte swapping
     */

    IHDR_Width  = BigLong(IHDR->Width);
    IHDR_Height = BigLong(IHDR->Height);

    /*
     *  Skip and Offset for the passes.
     */

    WSkip[0]   = 8;
    WOffset[0] = 0;
    HSkip[0]   = 8;
    HOffset[0] = 0;

    WSkip[1]   = 8;
    WOffset[1] = 4;
    HSkip[1]   = 8;
    HOffset[1] = 0;

    WSkip[2]   = 4;
    WOffset[2] = 0;
    HSkip[2]   = 8;
    HOffset[2] = 4;

    WSkip[3]   = 4;
    WOffset[3] = 2;
    HSkip[3]   = 4;
    HOffset[3] = 0;

    WSkip[4]   = 2;
    WOffset[4] = 0;
    HSkip[4]   = 4;
    HOffset[4] = 2;

    WSkip[5]   = 2;
    WOffset[5] = 1;
    HSkip[5]   = 2;
    HOffset[5] = 0;

    WSkip[6]   = 1;
    WOffset[6] = 0;
    HSkip[6]   = 2;
    HOffset[6] = 1;

    /*
     *  Calculate the sizes of the passes.
     */

    PassWidth[0]  = (IHDR_Width  + 7) / 8;
    PassHeight[0] = (IHDR_Height + 7) / 8;

    PassWidth[1]  = (IHDR_Width  + 3) / 8;
    PassHeight[1] = (IHDR_Height + 7) / 8;

    PassWidth[2]  = (IHDR_Width  + 3) / 4;
    PassHeight[2] = (IHDR_Height + 3) / 8;

    PassWidth[3]  = (IHDR_Width  + 1) / 4;
    PassHeight[3] = (IHDR_Height + 3) / 4;

    PassWidth[4]  = (IHDR_Width  + 1) / 2;
    PassHeight[4] = (IHDR_Height + 1) / 4;

    PassWidth[5]  = (IHDR_Width  + 0) / 2;
    PassHeight[5] = (IHDR_Height + 1) / 2;

    PassWidth[6]  = (IHDR_Width  + 0) / 1;
    PassHeight[6] = (IHDR_Height + 0) / 2;

    /*
     *  information for un-filtering
     */

    switch(IHDR->ColourType)
    {
        case PNG_ColourType_Grey :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;

                    break;
                }

                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_Grey;
                    PixelsPerByte    = 1;

                    break;
                }

                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }
  
        case PNG_ColourType_True :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_True;
                    PixelsPerByte    = 1;

                    break;
                }
     
                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }

        case PNG_ColourType_Indexed :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;

                    break;
                }

                case PNG_BitDepth_8 :
                {
                    BytesPerPixel    = PNG_NumColourComponents_Indexed;
                    PixelsPerByte    = 1;

                    break;
                }
         
                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }

        case PNG_ColourType_GreyAlpha :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_GreyAlpha;
                    PixelsPerByte    = 1;

                    break;
                }
     
                default :
                {
                    return(qfalse);
                }
            }
  
            break;
        }

        case PNG_ColourType_TrueAlpha :
        {
            switch(IHDR->BitDepth)
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_TrueAlpha;
                    PixelsPerByte    = 1;

                    break;
                }
    
                default :
                {
                    return(qfalse);
                }
            }

            break;
        }

        default :
        {
            return(qfalse);
        }
    }

    /*
     *  Calculate the size of the scanlines per pass
     */

    for(a = 0; a < PNG_Adam7_NumPasses; a++)
    {
	BytesPerScanline[a] = (PassWidth[a] * BytesPerPixel + (PixelsPerByte - 1)) / PixelsPerByte;
    }

    /*
     *  Calculate the size of all passes
     */

    TargetLength = 0;

    for(a = 0; a < PNG_Adam7_NumPasses; a++)
    {
	TargetLength += ((BytesPerScanline[a] + (BytesPerScanline[a] ? 1 : 0)) * PassHeight[a]);
    }

    /*
     *  Check if we have enough data for the whole image.
     */

    if(!(DecompressedDataLength == TargetLength))
    {
        return(qfalse);
    }

    /*
     *  Unfilter the image.
     */

    DecompPtr = DecompressedData;

    for(a = 0; a < PNG_Adam7_NumPasses; a++)
    {
        if(!UnfilterImage(DecompPtr, PassHeight[a], BytesPerScanline[a], BytesPerPixel))
        {
            return(qfalse);
        }
	
	DecompPtr += ((BytesPerScanline[a] + (BytesPerScanline[a] ? 1 : 0)) * PassHeight[a]);
    }

    /*
     *  Set the working pointers to the beginning of the buffers.
     */

    DecompPtr = DecompressedData;

    /*
     *  Create the output image.
     */

    for(a = 0; a < PNG_Adam7_NumPasses; a++)
    {
        for(h = 0; h < PassHeight[a]; h++)
        {
            /*
             *  Count the pixels on the scanline for those multipixel bytes
             */

            uint32_t CurrPixel;

            /*
             *  skip FilterType
             */

            DecompPtr++;

            /*
             *  Reset the pixel count.
             */

            CurrPixel = 0;

            for(w = 0; w < (BytesPerScanline[a] / BytesPerPixel); w++)
            {
        	if(PixelsPerByte > 1)
	        {
                    uint8_t  Mask;
                    uint32_t Shift;
		    uint8_t  SinglePixel;

                    for(p = 0; p < PixelsPerByte; p++)
                    {
                        if(CurrPixel < PassWidth[a])
                        {
                            Mask  = (1 << IHDR->BitDepth) - 1;
                            Shift = (PixelsPerByte - 1 - p) * IHDR->BitDepth;

                            SinglePixel = ((DecompPtr[0] & (Mask << Shift)) >> Shift);

    			    OutPtr = OutBuffer + (((((h * HSkip[a]) + HOffset[a]) * IHDR_Width) + ((CurrPixel * WSkip[a]) + WOffset[a])) * Q3IMAGE_BYTESPERPIXEL);

    			    if(!ConvertPixel(IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal))
			    {
			        return(qfalse);
			    }

                            CurrPixel++;
                        }
                    }
	    
	        }
    	        else
	        {
	    	    OutPtr = OutBuffer + (((((h * HSkip[a]) + HOffset[a]) * IHDR_Width) + ((w * WSkip[a]) + WOffset[a])) * Q3IMAGE_BYTESPERPIXEL);

		    if(!ConvertPixel(IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal))
		    {
		        return(qfalse);
		    }
	        }

                DecompPtr += BytesPerPixel;
            }
        }
    }

    return(qtrue);
}

/*
 *  The PNG loader
 */

static void LoadPNG(const char *name, byte **pic, int *width, int *height)
{
    struct BufferedFile *ThePNG;
    byte *OutBuffer;
    uint8_t *Signature;
    struct PNG_ChunkHeader *CH;
    uint32_t ChunkHeaderLength;
    uint32_t ChunkHeaderType;
    struct PNG_Chunk_IHDR *IHDR;
    uint32_t IHDR_Width;
    uint32_t IHDR_Height;
    PNG_ChunkCRC *CRC;
    uint8_t *InPal;
    uint8_t *DecompressedData;
    uint32_t DecompressedDataLength;
    uint32_t i;

    /*
     *  palette with 256 RGBA entries
     */

    uint8_t OutPal[1024];

    /*
     *  transparent colour from the tRNS chunk
     */

    qboolean HasTransparentColour = qfalse;
    uint8_t TransparentColour[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    /*
     *  input verification
     */

    if(!(name && pic))
    {
        return;
    }

    /*
     *  Zero out return values.
     */

    *pic = NULL;

    if(width)
    {
        *width = 0;
    }

    if(height)
    {
        *height = 0;
    }

    /*
     *  Read the file.
     */

    ThePNG = ReadBufferedFile(name);
    if(!ThePNG)
    {
        return;
    }           

    /*
     *  Read the siganture of the file.
     */

    Signature = BufferedFileRead(ThePNG, PNG_Signature_Size);
    if(!Signature)
    {
        CloseBufferedFile(ThePNG);
 
        return;
    }
 
    /*
     *  Is it a PNG?
     */

    if(memcmp(Signature, PNG_Signature, PNG_Signature_Size))
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }

    /*
     *  Read the first chunk-header.
     */

    CH = BufferedFileRead(ThePNG, PNG_ChunkHeader_Size);
    if(!CH)
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }

    /*
     *  PNG multi-byte types are in Big Endian
     */

    ChunkHeaderLength = BigLong(CH->Length);
    ChunkHeaderType   = BigLong(CH->Type);

    /*
     *  Check if the first chunk is an IHDR.
     */

    if(!((ChunkHeaderType == PNG_ChunkType_IHDR) && (ChunkHeaderLength == PNG_Chunk_IHDR_Size)))
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }

    /*
     *  Read the IHDR.
     */ 

    IHDR = BufferedFileRead(ThePNG, PNG_Chunk_IHDR_Size);
    if(!IHDR)
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }

    /*
     *  Read the CRC for IHDR
     */

    CRC = BufferedFileRead(ThePNG, PNG_ChunkCRC_Size);
    if(!CRC)
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }
 
    /*
     *  Here we could check the CRC if we wanted to.
     */
 
    /*
     *  multi-byte type swapping
     */

    IHDR_Width  = BigLong(IHDR->Width);
    IHDR_Height = BigLong(IHDR->Height);
 
    /*
     *  Check if Width and Height are valid.
     */

    if(!((IHDR_Width > 0) && (IHDR_Height > 0)))
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }

    /*
     *  Do we need to check if the dimensions of the image are valid for Quake3?
     */

    /*
     *  Check if CompressionMethod and FilterMethod are valid.
     */

    if(!((IHDR->CompressionMethod == PNG_CompressionMethod_0) && (IHDR->FilterMethod == PNG_FilterMethod_0)))
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }

    /*
     *  Check if InterlaceMethod is valid.
     */

    if(!((IHDR->InterlaceMethod == PNG_InterlaceMethod_NonInterlaced)  || (IHDR->InterlaceMethod == PNG_InterlaceMethod_Interlaced)))
    {
        CloseBufferedFile(ThePNG);
 
        return;
    }

    /*
     *  Read palette for an indexed image.
     */

    if(IHDR->ColourType == PNG_ColourType_Indexed)
    {
        /*
         *  We need the palette first.
         */

        if(!FindChunk(ThePNG, PNG_ChunkType_PLTE))
        {
            CloseBufferedFile(ThePNG);
  
            return;
        }

        /*
         *  Read the chunk-header.
         */

        CH = BufferedFileRead(ThePNG, PNG_ChunkHeader_Size);
        if(!CH)
        {
            CloseBufferedFile(ThePNG);
   
            return; 
        }

        /*
         *  PNG multi-byte types are in Big Endian
         */

        ChunkHeaderLength = BigLong(CH->Length);
        ChunkHeaderType   = BigLong(CH->Type);
  
        /*
         *  Check if the chunk is an PLTE.
         */

        if(!(ChunkHeaderType == PNG_ChunkType_PLTE))
        {
            CloseBufferedFile(ThePNG);
   
            return; 
        }

        /*
         *  Check if Length is divisible by 3
         */

        if(ChunkHeaderLength % 3)
        {
            CloseBufferedFile(ThePNG);
   
            return;   
        }

        /*
         *  Read the raw palette data
         */

        InPal = BufferedFileRead(ThePNG, ChunkHeaderLength);
        if(!InPal)
        {
            CloseBufferedFile(ThePNG);
   
            return; 
        }
   
        /*
         *  Read the CRC for the palette
         */

        CRC = BufferedFileRead(ThePNG, PNG_ChunkCRC_Size);
        if(!CRC)
        {
            CloseBufferedFile(ThePNG);
 
            return; 
        }

        /*
         *  Set some default values.
         */

        for(i = 0; i < 256; i++)
        {
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;  
        }

        /*
         *  Convert to the Quake3 RGBA-format.
         */

        for(i = 0; i < (ChunkHeaderLength / 3); i++)
        {
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = InPal[i*3+0];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = InPal[i*3+1];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = InPal[i*3+2];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;
        }
    }

    /*
     *  transparency information is sometimes stored in an tRNS chunk
     */

    /*
     *  Let's see if there is a tRNS chunk
     */

    if(FindChunk(ThePNG, PNG_ChunkType_tRNS))
    {
        uint8_t *Trans;

        /*
         *  Read the chunk-header.
         */

        CH = BufferedFileRead(ThePNG, PNG_ChunkHeader_Size);
        if(!CH)
        {
            CloseBufferedFile(ThePNG);
 
            return; 
        }

        /*
         *  PNG multi-byte types are in Big Endian
         */

        ChunkHeaderLength = BigLong(CH->Length);
        ChunkHeaderType   = BigLong(CH->Type);

        /*
         *  Check if the chunk is an tRNS.
         */

        if(!(ChunkHeaderType == PNG_ChunkType_tRNS))
        {
            CloseBufferedFile(ThePNG);
 
            return; 
        }

        /*
         *  Read the transparency information.
         */

        Trans = BufferedFileRead(ThePNG, ChunkHeaderLength);
        if(!Trans)
        {
            CloseBufferedFile(ThePNG);
 
            return;  
        }

        /*
         *  Read the CRC.
         */

        CRC = BufferedFileRead(ThePNG, PNG_ChunkCRC_Size);
        if(!CRC)
        {
            CloseBufferedFile(ThePNG);
  
            return; 
        }
 
        /*
         *  Only for Grey, True and Indexed ColourType should tRNS exist.
         */

        switch(IHDR->ColourType)
        {
            case PNG_ColourType_Grey :
            {
                if(!ChunkHeaderLength == 2)
                {
                    CloseBufferedFile(ThePNG);
  
                    return;    
                }
   
                HasTransparentColour = qtrue;
   
		/*
		 *  Grey can have one colour which is completely transparent.
		 *  This colour is always stored in 16 bits.
		 */

                TransparentColour[0] = Trans[0];
                TransparentColour[1] = Trans[1];
   
                break;
            }
   
            case PNG_ColourType_True :
            {
                if(!ChunkHeaderLength == 6)
                {
                    CloseBufferedFile(ThePNG);
  
                    return;    
                }
   
                HasTransparentColour = qtrue;

		/*
		 *  True can have one colour which is completely transparent.
		 *  This colour is always stored in 16 bits.
		 */

                TransparentColour[0] = Trans[0];
                TransparentColour[1] = Trans[1];
                TransparentColour[2] = Trans[2];
                TransparentColour[3] = Trans[3];
                TransparentColour[4] = Trans[4];
                TransparentColour[5] = Trans[5];
   
                break;
            }
   
            case PNG_ColourType_Indexed :
            {
                /*
		 *  Maximum of 256 one byte transparency entries.
		 */
		
		if(ChunkHeaderLength > 256)
                {
                    CloseBufferedFile(ThePNG);
  
                    return;    
                }

                HasTransparentColour = qtrue;

                /*
                 *  alpha values for palette entries
                 */

                for(i = 0; i < ChunkHeaderLength; i++)
                {
                    OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = Trans[i];
                }

                break;
            }
  
            /*
             *  All other ColourTypes should not have tRNS chunks
             */

            default :
            {
                CloseBufferedFile(ThePNG);
  
                return;
            }
        } 
    }

    /*
     *  Rewind to the start of the file.
     */

    if(!BufferedFileRewind(ThePNG, -1))
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }
 
    /*
     *  Skip the signature
     */

    if(!BufferedFileSkip(ThePNG, PNG_Signature_Size))
    {
        CloseBufferedFile(ThePNG);
 
        return; 
    }

    /*
     *  Decompress all IDAT chunks
     */

    DecompressedDataLength = DecompressIDATs(ThePNG, &DecompressedData);
    if(!(DecompressedDataLength && DecompressedData))
    {
        CloseBufferedFile(ThePNG);
 
        return;
    }

    /*
     *  Allocate output buffer.
     */

    OutBuffer = ri.Malloc(IHDR_Width * IHDR_Height * Q3IMAGE_BYTESPERPIXEL); 
    if(!OutBuffer)
    {
        ri.Free(DecompressedData); 
        CloseBufferedFile(ThePNG);
 
        return;  
    }

    /*
     *  Interlaced and Non-interlaced images need to be handled differently.
     */

    switch(IHDR->InterlaceMethod)
    {
	case PNG_InterlaceMethod_NonInterlaced :
	{
	    if(!DecodeImageNonInterlaced(IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal))
	    {
		ri.Free(OutBuffer); 
    		ri.Free(DecompressedData); 
    		CloseBufferedFile(ThePNG);

		return;
	    }
	
	    break;
	}
	
	case PNG_InterlaceMethod_Interlaced :
	{
	    if(!DecodeImageInterlaced(IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal))
	    {
		ri.Free(OutBuffer); 
    		ri.Free(DecompressedData); 
    		CloseBufferedFile(ThePNG);

		return;
	    }
	
	    break;
	}
    
	default :
	{
	    ri.Free(OutBuffer); 
    	    ri.Free(DecompressedData); 
    	    CloseBufferedFile(ThePNG);

	    return;
	}
    }

    /*
     *  update the pointer to the image data
     */

    *pic = OutBuffer;
 
    /*
     *  Fill width and height.
     */

    if(width)
    {
        *width = IHDR_Width;
    }

    if(height)
    {
        *height = IHDR_Height;
    }

    /*
     *  DecompressedData is not needed anymore.
     */

    ri.Free(DecompressedData); 

    /*
     *  We have all data, so close the file.
     */

    CloseBufferedFile(ThePNG);
}

//===================================================================

typedef struct
{
	char *ext;
	void (*ImageLoader)( const char *, unsigned char **, int *, int * );
} imageExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
static imageExtToLoaderMap_t imageLoaders[ ] =
{
	{ "tga",  LoadTGA },
	{ "jpg",  LoadJPG },
	{ "jpeg", LoadJPG },
	{ "png",  LoadPNG },
	{ "pcx",  LoadPCX32 },
	{ "bmp",  LoadBMP }
};

static int numImageLoaders = sizeof( imageLoaders ) /
		sizeof( imageLoaders[ 0 ] );

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *name, byte **pic, int *width, int *height )
{
	qboolean orgNameFailed = qfalse;
	int i;
	char localName[ MAX_QPATH ];
	const char *ext;

	*pic = NULL;
	*width = 0;
	*height = 0;

	Q_strncpyz( localName, name, MAX_QPATH );

	ext = COM_GetExtension( localName );

	if( *ext )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numImageLoaders; i++ )
		{
			if( !Q_stricmp( ext, imageLoaders[ i ].ext ) )
			{
				// Load
				imageLoaders[ i ].ImageLoader( localName, pic, width, height );
				break;
			}
		}

		// A loader was found
		if( i < numImageLoaders )
		{
			if( *pic == NULL )
			{
				// Loader failed, most likely because the file isn't there;
				// try again without the extension
				orgNameFailed = qtrue;
				COM_StripExtension( name, localName, MAX_QPATH );
			}
			else
			{
				// Something loaded
				return;
			}
		}
	}

	// Try and find a suitable match using all
	// the image formats supported
	for( i = 0; i < numImageLoaders; i++ )
	{
		char *altName = va( "%s.%s", localName, imageLoaders[ i ].ext );

		// Load
		imageLoaders[ i ].ImageLoader( altName, pic, width, height );

		if( *pic )
		{
			if( orgNameFailed )
			{
				ri.Printf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n",
						name, altName );
			}

			break;
		}
	}
}


/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t	*R_FindImageFile( const char *name, qboolean mipmap, qboolean allowPicmip, int glWrapClampMode ) {
	image_t	*image;
	int		width, height;
	byte	*pic;
	long	hash;

	if (!name) {
		return NULL;
	}

	hash = generateHashValue(name);

	//
	// see if the image is already loaded
	//
	for (image=hashTable[hash]; image; image=image->next) {
		if ( !strcmp( name, image->imgName ) ) {
			// the white image can be used with any set of parms, but other mismatches are errors
			if ( strcmp( name, "*white" ) ) {
				if ( image->mipmap != mipmap ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed mipmap parm\n", name );
				}
				if ( image->allowPicmip != allowPicmip ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed allowPicmip parm\n", name );
				}
				if ( image->wrapClampMode != glWrapClampMode ) {
					ri.Printf( PRINT_ALL, "WARNING: reused image %s with mixed glWrapClampMode parm\n", name );
				}
			}
			return image;
		}
	}

	//
	// load the pic from disk
	//
	R_LoadImage( name, &pic, &width, &height );
	if ( pic == NULL ) {
		return NULL;
	}

	image = R_CreateImage( ( char * ) name, pic, width, height, mipmap, allowPicmip, glWrapClampMode );
	ri.Free( pic );
	return image;
}


/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	16
static void R_CreateDlightImage( void ) {
	int		x,y;
	byte	data[DLIGHT_SIZE][DLIGHT_SIZE][4];
	int		b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for (x=0 ; x<DLIGHT_SIZE ; x++) {
		for (y=0 ; y<DLIGHT_SIZE ; y++) {
			float	d;

			d = ( DLIGHT_SIZE/2 - 0.5f - x ) * ( DLIGHT_SIZE/2 - 0.5f - x ) +
				( DLIGHT_SIZE/2 - 0.5f - y ) * ( DLIGHT_SIZE/2 - 0.5f - y );
			b = 4000 / d;
			if (b > 255) {
				b = 255;
			} else if ( b < 75 ) {
				b = 0;
			}
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = b;
			data[y][x][3] = 255;			
		}
	}
	tr.dlightImage = R_CreateImage("*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, qfalse, GL_CLAMP );
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int		i;
	float	d;
	float	exp;
	
	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float	R_FogFactor( float s, float t ) {
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32
static void R_CreateFogImage( void ) {
	int		x,y;
	byte	*data;
	float	g;
	float	d;
	float	borderColor[4];

	data = ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	g = 2.0;

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++) {
		for (y=0 ; y<FOG_T ; y++) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[(y*FOG_S+x)*4+0] = 
			data[(y*FOG_S+x)*4+1] = 
			data[(y*FOG_S+x)*4+2] = 255;
			data[(y*FOG_S+x)*4+3] = 255*d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP );
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define	DEFAULT_SIZE	16
static void R_CreateDefaultImage( void ) {
	int		x;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] =
		data[0][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] =
		data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
		data[DEFAULT_SIZE-1][x][1] =
		data[DEFAULT_SIZE-1][x][2] =
		data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
		data[x][DEFAULT_SIZE-1][1] =
		data[x][DEFAULT_SIZE-1][2] =
		data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, GL_REPEAT );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	int		x,y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT );

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; x++) {
		for (y=0 ; y<DEFAULT_SIZE ; y++) {
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;			
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT );


	for(x=0;x<32;x++) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage("*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qtrue, GL_CLAMP );
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int		i, j;
	float	g;
	int		inf;
	int		shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if ( !glConfig.deviceSupportsGamma ) {
		tr.overbrightBits = 0;		// need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if ( !glConfig.isFullscreen ) 
	{
		tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if ( glConfig.colorBits > 16 ) {
		if ( tr.overbrightBits > 2 ) {
			tr.overbrightBits = 2;
		}
	} else {
		if ( tr.overbrightBits > 1 ) {
			tr.overbrightBits = 1;
		}
	}
	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = 255 * tr.identityLight;


	if ( r_intensity->value <= 1 ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f ) {
		ri.Cvar_Set( "r_gamma", "3.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		if (inf < 0) {
			inf = 0;
		}
		if (inf > 255) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for (i=0 ; i<256 ; i++) {
		j = i * r_intensity->value;
		if (j > 255) {
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if ( glConfig.deviceSupportsGamma )
	{
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}

/*
===============
R_InitImages
===============
*/
void	R_InitImages( void ) {
	Com_Memset(hashTable, 0, sizeof(hashTable));
	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures( void ) {
	int		i;

	for ( i=0; i<tr.numImages ; i++ ) {
		qglDeleteTextures( 1, &tr.images[i]->texnum );
	}
	Com_Memset( tr.images, 0, sizeof( tr.images ) );

	tr.numImages = 0;

	Com_Memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( glConfig.numTextureUnits > 1 ) {
		GL_SelectTexture( 1 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
		GL_SelectTexture( 0 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
	} else {
		qglBindTexture( GL_TEXTURE_2D, 0 );
	}
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t	hSkin;
	skin_t		*skin;
	skinSurface_t	*surf;
	char		*text, *text_p;
	char		*token;
	char		surfName[MAX_QPATH];

	if ( !name || !name[0] ) {
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if ( strcmp( name + strlen( name ) - 5, ".skin" ) ) {
		skin->numSurfaces = 1;
		skin->surfaces[0] = ri.Hunk_Alloc( sizeof(skin->surfaces[0]), h_low );
		skin->surfaces[0]->shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
    ri.FS_ReadFile( name, (void **)&text );
	if ( !text ) {
		return 0;
	}

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void	R_InitSkins( void ) {
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void	R_SkinList_f( void ) {
	int			i, j;
	skin_t		*skin;

	ri.Printf (PRINT_ALL, "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n", 
				skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}

