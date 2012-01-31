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
//
// trilib.c: library for loading triangles from an Alias triangle file
//

#include <stdio.h>
#include "cmdlib.h"
#include "mathlib.h"
#include "polyset.h"
#include "trilib.h"

// on disk representation of a face


#define	FLOAT_START	99999.0
#define	FLOAT_END	-FLOAT_START
#define MAGIC       123322

//#define NOISY 1

typedef struct {
	float v[3];
} vector;

typedef struct
{
	vector n;    /* normal */
	vector p;    /* point */
	vector c;    /* color */
	float  u;    /* u */
	float  v;    /* v */
} aliaspoint_t;

typedef struct {
	aliaspoint_t	pt[3];
} tf_triangle;


static void ByteSwapTri (tf_triangle *tri)
{
	int		i;
	
	for (i=0 ; i<sizeof(tf_triangle)/4 ; i++)
	{
		((int *)tri)[i] = BigLong (((int *)tri)[i]);
	}
}

static void ReadPolysetGeometry( triangle_t *tripool, FILE *input, int count, triangle_t *ptri )
{
	tf_triangle tri;
	int i;

	for (i = 0; i < count; ++i) {
		int		j;

		fread( &tri, sizeof(tf_triangle), 1, input );
		ByteSwapTri (&tri);
		for (j=0 ; j<3 ; j++)
		{
			int		k;

			for (k=0 ; k<3 ; k++)
			{
				ptri->verts[j][k] = tri.pt[j].p.v[k];
				ptri->normals[j][k] = tri.pt[j].n.v[k];
//				ptri->colors[j][k] = tri.pt[j].c.v[k];
			}

			ptri->texcoords[j][0] = tri.pt[j].u;
			ptri->texcoords[j][1] = tri.pt[j].v;
		}

		ptri++;
		if ((ptri - tripool ) >= POLYSET_MAXTRIANGLES)
			Error ("Error: too many triangles; increase POLYSET_MAXTRIANGLES\n");
	}
}

void TRI_LoadPolysets( const char *filename, polyset_t **ppPSET, int *numpsets )
{
	FILE        *input;
	float       start;
	char        name[256], tex[256];
	int         i, count, magic, pset = 0;
	triangle_t	*ptri;
	polyset_t	*pPSET;
	int			iLevel;
	int			exitpattern;
	float		t;

	t = -FLOAT_START;
	*((unsigned char *)&exitpattern + 0) = *((unsigned char *)&t + 3);
	*((unsigned char *)&exitpattern + 1) = *((unsigned char *)&t + 2);
	*((unsigned char *)&exitpattern + 2) = *((unsigned char *)&t + 1);
	*((unsigned char *)&exitpattern + 3) = *((unsigned char *)&t + 0);

	if ((input = fopen(filename, "rb")) == 0)
		Error ("reader: could not open file '%s'", filename);

	iLevel = 0;

	fread(&magic, sizeof(int), 1, input);
	if (BigLong(magic) != MAGIC)
		Error ("%s is not a Alias object separated triangle file, magic number is wrong.", filename);

	pPSET = calloc( 1, POLYSET_MAXPOLYSETS * sizeof( polyset_t ) );
	ptri = calloc( 1, POLYSET_MAXTRIANGLES * sizeof( triangle_t ) );

	*ppPSET = pPSET;

	while (feof(input) == 0) {
		if (fread(&start,  sizeof(float), 1, input) < 1)
			break;
		*(int *)&start = BigLong(*(int *)&start);
		if (*(int *)&start != exitpattern)
		{
			if (start == FLOAT_START) {
				/* Start of an object or group of objects. */
				i = -1;
				do {
					/* There are probably better ways to read a string from */
					/* a file, but this does allow you to do error checking */
					/* (which I'm not doing) on a per character basis.      */
					++i;
					fread( &(name[i]), sizeof( char ), 1, input);
				} while( name[i] != '\0' );
	
				if ( i != 0 )
					strncpy( pPSET[pset].name, name, sizeof( pPSET[pset].name ) - 1 );
				else
					strcpy( pPSET[pset].name , "(unnamed)" );
				strlwr( pPSET[pset].name );

//				indent();
//				fprintf(stdout,"OBJECT START: %s\n",name);
				fread( &count, sizeof(int), 1, input);
				count = BigLong(count);
				++iLevel;
				if (count != 0) {
//					indent();
//					fprintf(stdout,"NUMBER OF TRIANGLES: %d\n",count);
	
					i = -1;
					do {
						++i;
						fread( &(tex[i]), sizeof( char ), 1, input);
					} while( tex[i] != '\0' );

/*
					if ( i != 0 )
						strncpy( pPSET[pset].texname, tex, sizeof( pPSET[pset].texname ) - 1 );
					else
						strcpy( pPSET[pset].texname, "(unnamed)" );
					strlwr( pPSET[pset].texname );
*/

//					indent();
//					fprintf(stdout,"  Object texture name: '%s'\n",tex);
				}
	
				/* Else (count == 0) this is the start of a group, and */
				/* no texture name is present. */
			}
			else if (start == FLOAT_END) {
				/* End of an object or group. Yes, the name should be */
				/* obvious from context, but it is in here just to be */
				/* safe and to provide a little extra information for */
				/* those who do not wish to write a recursive reader. */
				/* Mea culpa. */
				--iLevel;
				i = -1;
				do {
					++i;
					fread( &(name[i]), sizeof( char ), 1, input);
				} while( name[i] != '\0' );

				if ( i != 0 )
					strncpy( pPSET[pset].name, name, sizeof( pPSET[pset].name ) - 1 );
				else
					strcpy( pPSET[pset].name , "(unnamed)" );

				strlwr( pPSET[pset].name );
	
//				indent();
//				fprintf(stdout,"OBJECT END: %s\n",name);
				continue;
			}
		}

//
// read the triangles
//		
		if ( count > 0 )
		{
			pPSET[pset].triangles = ptri;
			ReadPolysetGeometry( pPSET[0].triangles, input, count, ptri );
			ptri += count;
			pPSET[pset].numtriangles = count;
			if ( ++pset >= POLYSET_MAXPOLYSETS )
			{
				Error ("Error: too many polysets; increase POLYSET_MAXPOLYSETS\n");
			}
		}
	}

	*numpsets = pset;

	fclose (input);
}

