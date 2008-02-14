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

#include "tr_local.h"

/*
========================================================================

PCX files are used for 8 bit images

========================================================================
*/

typedef struct {
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
} pcx_t;


static void _LoadPCX ( const char *filename, byte **pic, byte **palette, int *width, int *height)
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


void LoadPCX ( const char *filename, byte **pic, int *width, int *height) {
	byte	*palette;
	byte	*pic8;
	int		i, c, p;
	byte	*pic32;

	_LoadPCX (filename, &pic8, &palette, width, height);
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
