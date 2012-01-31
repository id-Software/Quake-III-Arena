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

#include "stdafx.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "bmp.h"


void Error(char *fmt, ...);


static int GetColorCount(int nbits)
{
    int ncolors = 0;

    if (nbits < 24)
    {
 	    ncolors = 1 << nbits;
    }

    return(ncolors);
}


static void BMPLineNone(FILE *f, char *sline, int pixbytes, int width)
{
    int nbytes, i, k, j;

	switch (pixbytes)
	{
	    case 1 :
            nbytes = (width + 3) / 4;
            nbytes *= 4;

            fread(sline, width, 1, f);
            nbytes -= width;

            while (nbytes-- > 0) fgetc(f);
			return;

		case 3 :
			nbytes = ((width * 3) + 3) / 4;
			nbytes *= 4;

			fread(sline, width, 3, f);
			nbytes -= width * 3;

			while (nbytes-- > 0) fgetc(f);

			// reorder bgr to rgb
			for (i = 0, j = 0; i < width; i++, j += 3)
			{
				k = sline[j];
				sline[j] = sline[j+2];
				sline[j+2] = k;
			}

			return;
	}

	Error("BMPLineNone failed.");
}


static void BMPLineRLE8(FILE *f, char *sline, int pixbytes, int width)
{
    Error("RLE8 not yet supported.");
}


static void BMPLineRLE4(FILE *f, char *sline, int pixbytes, int width)
{
    Error("RLE4 not yet supported.");
}


static void BMPLine(FILE *f, char *scanline, int pixbytes, int width, int rle)
{
    switch (rle)
    {
	    case xBI_NONE : BMPLineNone(f, scanline, pixbytes, width); return;
	    case xBI_RLE8 : BMPLineRLE8(f, scanline, pixbytes, width); return;
	    case xBI_RLE4 : BMPLineRLE4(f, scanline, pixbytes, width); return;
    }

    Error("Unknown compression type.");
}



static void PrintHeader(binfo_t *b)
{
    printf("biSize         : %ld\n", b->biSize);
    printf("biWidth        : %ld\n", b->biWidth);
    printf("biHeight       : %ld\n", b->biHeight);
    printf("biPlanes       : %d\n", b->biPlanes);
    printf("biBitCount     : %d\n", b->biBitCount);
    printf("biCompression  : %ld\n", b->biCompression);
    printf("biSizeImage    : %ld\n", b->biSizeImage);
    printf("biXPelsPerMeter: %ld\n", b->biXPelsPerMeter);
    printf("biYPelsPerMeter: %ld\n", b->biYPelsPerMeter);
    printf("biClrUsed      : %ld\n", b->biClrUsed);
    printf("biClrImportant : %ld\n", b->biClrImportant);
}


void LoadBMP(char *filename, bitmap_t *bit)
{
    FILE    *f;
    bmphd_t  bhd;
    binfo_t  info;
    int      pxlsize = 1;
    int      rowbytes, i, pixbytes;
    char    *scanline;

    // open file
    if ((f = fopen(filename, "rb")) == NULL)
    {
	    Error("Unable to open %s.", filename);
    }

    // read in bitmap header
    if (fread(&bhd, sizeof(bhd), 1, f) != 1)
    {
		fclose(f);
	    Error("Unable to read in bitmap header.");
    }

    // make sure we have a valid bitmap file
    if (bhd.bfType != BMP_SIGNATURE_WORD)
    {
		fclose(f);
	    Error("Invalid BMP file: %s", filename);
    }

    // load in info header
    if (fread(&info, sizeof(info), 1, f) != 1)
    {
		fclose(f);
	    Error("Unable to read bitmap info header.");
    }

    // make sure this is an info type of bitmap
    if (info.biSize != sizeof(binfo_t))
    {
		fclose(f);
	    Error("We only support the info bitmap type.");
    }

    // PrintHeader(&info);

	bit->bpp      = info.biBitCount;
	bit->width    = info.biWidth;
    bit->height   = info.biHeight;
    bit->data     = NULL;
    bit->palette  = NULL;

    //currently we only read in 8 and 24 bit bmp files
	if      (info.biBitCount == 8)  pixbytes = 1;
	else if (info.biBitCount == 24) pixbytes = 3;
	else
    {
		Error("BPP %d not supported.", info.biBitCount);
    }

    // if this is an eight bit image load palette
	if (pixbytes == 1)
    {
	    drgb_t q;

	    bit->palette = reinterpret_cast<rgb_t*>(malloc(sizeof(rgb_t) * 256));

	    for (i = 0; i < 256; i++)
	    {
	        if (fread(&q, sizeof(drgb_t), 1, f) != 1)
	        {
				fclose(f); free(bit->palette);
		        Error("Unable to read palette.");
			}

	        bit->palette[i].r   = q.red;
	        bit->palette[i].g   = q.green;
	        bit->palette[i].b   = q.blue;
		}
    }

    // position to start of bitmap
    fseek(f, bhd.bfOffBits, SEEK_SET);

    // create scanline to read data into
    rowbytes = ((info.biWidth * pixbytes) + 3) / 4;
    rowbytes *= 4;

    scanline = reinterpret_cast<char*>(malloc(rowbytes));

    // alloc space for new bitmap
    bit->data = reinterpret_cast<unsigned char*>(malloc(info.biWidth * pixbytes * info.biHeight));

    // read in image
    for (i = 0; i < info.biHeight; i++)
    {
	    BMPLine(f, scanline, pixbytes, info.biWidth, info.biCompression);

	    // store line
	    memcpy(&bit->data[info.biWidth * pixbytes * (info.biHeight - i - 1)], scanline, info.biWidth * pixbytes);
    }

    free(scanline);
    fclose(f);
}



static void BMPEncodeLine(FILE *f, unsigned char *data, int npxls, int pixbytes)
{
    int nbytes, i, j, k;

	switch (pixbytes)
	{
	    case 1 :
            nbytes = (npxls + 3) / 4;
            nbytes *= 4;

            fwrite(data, npxls, 1, f);
            nbytes -= npxls;

            while (nbytes-- > 0) fputc(0, f);
			return;

        case 3 :
			// reorder rgb to bgr
			for (i = 0, j = 0; i < npxls; i++, j+= 3)
			{
				k = data[j];
				data[j] = data[j + 2];
				data[j + 2] = k;
			}

			nbytes = ((npxls * 3) + 3) / 4;
			nbytes *= 4;

			fwrite(data, npxls, 3, f);
			nbytes -= npxls * 3;

			while (nbytes-- > 0) fputc(0, f);
			return;
	}

	Error("BMPEncodeLine Failed.");
}



void WriteBMP(char *filename, bitmap_t *bit)
{
    FILE    *f;
    bmphd_t  header;
    binfo_t  info;
    drgb_t   q;        // palette that gets written
    long     bmofs;
    int      w, h, i;
	int      pixbytes;

    if      (bit->bpp == 8)  pixbytes = 1;
	else if (bit->bpp == 24) pixbytes = 3;

	else
    {
		Error("BPP %d not supported.", bit->bpp);
    }


    if ((f = fopen(filename, "wb")) == NULL)
    {
	    Error("Unable to open %s.", filename);
    }

    // write out an empty header as a place holder
    if (fwrite(&header, sizeof(header), 1, f) != 1)
    {
	    Error("Unable to fwrite.");
    }

    // init and write info header
    info.biSize          = sizeof(binfo_t);
    info.biWidth         = bit->width;
    info.biHeight        = bit->height;
    info.biPlanes        = 1;
    info.biBitCount      = bit->bpp;
    info.biCompression   = xBI_NONE;
    info.biSizeImage     = bit->width * bit->height;
    info.biXPelsPerMeter = 0;
    info.biYPelsPerMeter = 0;
    info.biClrUsed       = 256;
    info.biClrImportant  = 256;

    if (fwrite(&info, sizeof(binfo_t), 1, f) != 1)
    {
	    Error("fwrite failed.");
    }

    // write out palette if we need to
	if (bit->bpp == 8)
	{
        for (i = 0; i < 256; i++)
        {
	        q.red   = bit->palette[i].r;
	        q.green = bit->palette[i].g;
	        q.blue  = bit->palette[i].b;

	        fwrite(&q, sizeof(q), 1, f);
		}
    }

    // save offset to start of bitmap
    bmofs = ftell(f);

    // output bitmap
    w = bit->width;
    h = bit->height;

    for (i = h - 1; i >= 0; i--)
    {
	    BMPEncodeLine(f, &bit->data[w * pixbytes * i], w, pixbytes);
    }

    // update and rewrite file header
    header.bfType    = BMP_SIGNATURE_WORD;
    header.bfSize    = ftell(f);
    header.bfOffBits = bmofs;

    fseek(f, 0L, SEEK_SET);
    fwrite(&header, sizeof(header), 1, f);

    fclose(f);
}


void NewBMP(int width, int height, int bpp, bitmap_t *bit)
{
	int pixbytes;

	if      (bpp == 8)  pixbytes = 1;
	else if (bpp == 24) pixbytes = 3;

	else
	{
		Error("NewBMP: 8 or 24 bit only.");
	}

	bit->bpp    = bpp;
	bit->width  = width;
	bit->height = height;

	bit->data = reinterpret_cast<unsigned char*>(malloc(width * height * pixbytes));

	if (bit->data == NULL)
	{
		Error("NewBMP: malloc failed.");
	}

	// see if we need to create a palette
	if (pixbytes == 1)
	{
		bit->palette = (rgb_t *) malloc(768);

		if (bit->palette == NULL)
		{
			Error("NewBMP: unable to malloc palette.");
		}
	}
	else
	{
		bit->palette = NULL;
	}
}



void FreeBMP(bitmap_t *bitmap)
{
    if (bitmap->palette)
    {
	    free(bitmap->palette);
	    bitmap->palette = NULL;
    }

    if (bitmap->data)
    {
	    free(bitmap->data);
	    bitmap->data = NULL;
    }
}


