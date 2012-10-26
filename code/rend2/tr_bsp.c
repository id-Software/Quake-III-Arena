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
// tr_map.c

#include "tr_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

static	world_t		s_worldData;
static	byte		*fileBase;

int			c_subdivisions;
int			c_gridVerts;

//===============================================================================

static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
static	void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int		shift, r, g, b;

	// shift the color data based on overbright range
	shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}


/*
===============
R_ColorShiftLightingBytes

===============
*/
static void R_ColorShiftLightingFloats(float in[4], float out[4], float scale )
{
	scale *= pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits);

	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3];
}


void ColorToRGBE(const vec3_t color, unsigned char rgbe[4])
{
	vec3_t          sample;
	float			maxComponent;
	int				e;

	VectorCopy(color, sample);

	maxComponent = sample[0];
	if(sample[1] > maxComponent)
		maxComponent = sample[1];
	if(sample[2] > maxComponent)
		maxComponent = sample[2];

	if(maxComponent < 1e-32)
	{
		rgbe[0] = 0;
		rgbe[1] = 0;
		rgbe[2] = 0;
		rgbe[3] = 0;
	}
	else
	{
#if 0
		maxComponent = frexp(maxComponent, &e) * 255.0 / maxComponent;
		rgbe[0] = (unsigned char) (sample[0] * maxComponent);
		rgbe[1] = (unsigned char) (sample[1] * maxComponent);
		rgbe[2] = (unsigned char) (sample[2] * maxComponent);
		rgbe[3] = (unsigned char) (e + 128);
#else
		e = ceil(log(maxComponent) / log(2.0f));//ceil(log2(maxComponent));
		VectorScale(sample, 1.0 / pow(2.0f, e)/*exp2(e)*/, sample);

		rgbe[0] = (unsigned char) (sample[0] * 255);
		rgbe[1] = (unsigned char) (sample[1] * 255);
		rgbe[2] = (unsigned char) (sample[2] * 255);
		rgbe[3] = (unsigned char) (e + 128);
#endif
	}
}


void ColorToRGBA16F(const vec3_t color, unsigned short rgba16f[4])
{
	rgba16f[0] = FloatToHalf(color[0]);
	rgba16f[1] = FloatToHalf(color[1]);
	rgba16f[2] = FloatToHalf(color[2]);
	rgba16f[3] = FloatToHalf(1.0f);
}


/*
===============
R_LoadLightmaps

===============
*/
#define	DEFAULT_LIGHTMAP_SIZE	128
#define MAX_LIGHTMAP_PAGES 2
static	void R_LoadLightmaps( lump_t *l, lump_t *surfs ) {
	byte		*buf, *buf_p;
	dsurface_t  *surf;
	int			len;
	byte		*image;
	int			i, j, numLightmaps, textureInternalFormat = 0;
	float maxIntensity = 0;
	double sumIntensity = 0;

	len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_SyncRenderThread();

	tr.lightmapSize = DEFAULT_LIGHTMAP_SIZE;
	numLightmaps = len / (tr.lightmapSize * tr.lightmapSize * 3);

	// check for deluxe mapping
	if (numLightmaps <= 1)
	{
		tr.worldDeluxeMapping = qfalse;
	}
	else
	{
		tr.worldDeluxeMapping = qtrue;
		for( i = 0, surf = (dsurface_t *)(fileBase + surfs->fileofs);
			i < surfs->filelen / sizeof(dsurface_t); i++, surf++ ) {
			int lightmapNum = LittleLong( surf->lightmapNum );

			if ( lightmapNum >= 0 && (lightmapNum & 1) != 0 ) {
				tr.worldDeluxeMapping = qfalse;
				break;
			}
		}
	}

	image = ri.Malloc(tr.lightmapSize * tr.lightmapSize * 4 * 2);

	if (tr.worldDeluxeMapping)
		numLightmaps >>= 1;

	if(numLightmaps == 1)
	{
		//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
		//this avoids this, but isn't the correct solution.
		numLightmaps++;
	}
	else if (r_mergeLightmaps->integer && numLightmaps >= 1024 )
	{
		// FIXME: fat light maps don't support more than 1024 light maps
		ri.Printf(PRINT_WARNING, "WARNING: number of lightmaps > 1024\n");
		numLightmaps = 1024;
	}

	// use fat lightmaps of an appropriate size
	if (r_mergeLightmaps->integer)
	{
		tr.fatLightmapSize = 512;
		tr.fatLightmapStep = tr.fatLightmapSize / tr.lightmapSize;

		// at most MAX_LIGHTMAP_PAGES
		while (tr.fatLightmapStep * tr.fatLightmapStep * MAX_LIGHTMAP_PAGES < numLightmaps && tr.fatLightmapSize != glConfig.maxTextureSize )
		{
			tr.fatLightmapSize <<= 1;
			tr.fatLightmapStep = tr.fatLightmapSize / tr.lightmapSize;
		}

		tr.numLightmaps = numLightmaps / (tr.fatLightmapStep * tr.fatLightmapStep);

		if (numLightmaps % (tr.fatLightmapStep * tr.fatLightmapStep) != 0)
			tr.numLightmaps++;
	}
	else
	{
		tr.numLightmaps = numLightmaps;
	}

	tr.lightmaps = ri.Hunk_Alloc( tr.numLightmaps * sizeof(image_t *), h_low );

	if (tr.worldDeluxeMapping)
	{
		tr.deluxemaps = ri.Hunk_Alloc( tr.numLightmaps * sizeof(image_t *), h_low );
	}

	if (r_hdr->integer && glRefConfig.textureFloat && glRefConfig.halfFloatPixel)
		textureInternalFormat = GL_RGBA16F_ARB;

	if (r_mergeLightmaps->integer)
	{
		for (i = 0; i < tr.numLightmaps; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("_fatlightmap%d", i), NULL, tr.fatLightmapSize, tr.fatLightmapSize, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, textureInternalFormat );

			if (tr.worldDeluxeMapping)
			{
				tr.deluxemaps[i] = R_CreateImage(va("_fatdeluxemap%d", i), NULL, tr.fatLightmapSize, tr.fatLightmapSize, IMGTYPE_DELUXE, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
			}
		}
	}

	for(i = 0; i < numLightmaps; i++)
	{
		int xoff = 0, yoff = 0;
		int lightmapnum = i;
		// expand the 24 bit on-disk to 32 bit

		if (r_mergeLightmaps->integer)
		{
			int lightmaponpage = i % (tr.fatLightmapStep * tr.fatLightmapStep);
			xoff = (lightmaponpage % tr.fatLightmapStep) * tr.lightmapSize;
			yoff = (lightmaponpage / tr.fatLightmapStep) * tr.lightmapSize;

			lightmapnum /= (tr.fatLightmapStep * tr.fatLightmapStep);
		}

		// if (tr.worldLightmapping)
		{
			char filename[MAX_QPATH];
			byte *hdrLightmap = NULL;
			float lightScale = 1.0f;
			int size = 0;

			// look for hdr lightmaps
			if (r_hdr->integer)
			{
				Com_sprintf( filename, sizeof( filename ), "maps/%s/lm_%04d.hdr", s_worldData.baseName, i * (tr.worldDeluxeMapping ? 2 : 1) );
				//ri.Printf(PRINT_ALL, "looking for %s\n", filename);

				size = ri.FS_ReadFile(filename, (void **)&hdrLightmap);
			}

			if (hdrLightmap)
			{
				byte *p = hdrLightmap;
				//ri.Printf(PRINT_ALL, "found!\n");
				
				/* FIXME: don't just skip over this header and actually parse it */
				while (size && !(*p == '\n' && *(p+1) == '\n'))
				{
					size--;
					p++;
				}

				if (!size)
					ri.Error(ERR_DROP, "Bad header for %s!\n", filename);

				size -= 2;
				p += 2;
				
				while (size && !(*p == '\n'))
				{
					size--;
					p++;
				}

				size--;
				p++;

				buf_p = (byte *)p;

#if 0 // HDRFILE_RGBE
				if (size != tr.lightmapSize * tr.lightmapSize * 4)
					ri.Error(ERR_DROP, "Bad size for %s (%i)!\n", filename, size);
#else // HDRFILE_FLOAT
				if (size != tr.lightmapSize * tr.lightmapSize * 12)
					ri.Error(ERR_DROP, "Bad size for %s (%i)!\n", filename, size);
#endif
			}
			else
			{
				if (tr.worldDeluxeMapping)
					buf_p = buf + (i * 2) * tr.lightmapSize * tr.lightmapSize * 3;
				else
					buf_p = buf + i * tr.lightmapSize * tr.lightmapSize * 3;
			}

			lightScale = pow(2, r_mapOverBrightBits->integer - tr.overbrightBits - 8); //exp2(r_mapOverBrightBits->integer - tr.overbrightBits - 8);

			for ( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ ) 
			{
				if (r_hdr->integer)
				{
					float color[3];

					if (hdrLightmap)
					{
#if 0 // HDRFILE_RGBE
						float exponent = exp2(buf_p[j*4+3] - 128);

						color[0] = buf_p[j*4+0] * exponent;
						color[1] = buf_p[j*4+1] * exponent;
						color[2] = buf_p[j*4+2] * exponent;
#else // HDRFILE_FLOAT
						memcpy(color, &buf_p[j*12], 12);

						color[0] = LittleFloat(color[0]);
						color[1] = LittleFloat(color[1]);
						color[2] = LittleFloat(color[2]);
#endif
					}
					else
					{
						//hack: convert LDR lightmap to HDR one
						color[0] = (buf_p[j*3+0] + 1.0f);
						color[1] = (buf_p[j*3+1] + 1.0f);
						color[2] = (buf_p[j*3+2] + 1.0f);

						// if under an arbitrary value (say 12) grey it out
						// this prevents weird splotches in dimly lit areas
						if (color[0] + color[1] + color[2] < 12.0f)
						{
							float avg = (color[0] + color[1] + color[2]) * 0.3333f;
							color[0] = avg;
							color[1] = avg;
							color[2] = avg;
						}
					}

					VectorScale(color, lightScale, color);

					if (glRefConfig.textureFloat && glRefConfig.halfFloatPixel)
						ColorToRGBA16F(color, (unsigned short *)(&image[j*8]));
					else
						ColorToRGBE(color, &image[j*4]);
				}
				else
				{
					if ( r_lightmap->integer == 2 )
					{	// color code by intensity as development tool	(FIXME: check range)
						float r = buf_p[j*3+0];
						float g = buf_p[j*3+1];
						float b = buf_p[j*3+2];
						float intensity;
						float out[3] = {0.0, 0.0, 0.0};

						intensity = 0.33f * r + 0.685f * g + 0.063f * b;

						if ( intensity > 255 )
							intensity = 1.0f;
						else
							intensity /= 255.0f;

						if ( intensity > maxIntensity )
							maxIntensity = intensity;

						HSVtoRGB( intensity, 1.00, 0.50, out );

						image[j*4+0] = out[0] * 255;
						image[j*4+1] = out[1] * 255;
						image[j*4+2] = out[2] * 255;
						image[j*4+3] = 255;

						sumIntensity += intensity;
					}
					else
					{
						R_ColorShiftLightingBytes( &buf_p[j*3], &image[j*4] );
						image[j*4+3] = 255;
					}
				}
			}

			if (r_mergeLightmaps->integer)
				R_UpdateSubImage(tr.lightmaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize);
			else
				tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, textureInternalFormat );

			if (hdrLightmap)
				ri.FS_FreeFile(hdrLightmap);
		}

		if (tr.worldDeluxeMapping)
		{
			buf_p = buf + (i * 2 + 1) * tr.lightmapSize * tr.lightmapSize * 3;

			for ( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ ) {
				image[j*4+0] = buf_p[j*3+0];
				image[j*4+1] = buf_p[j*3+1];
				image[j*4+2] = buf_p[j*3+2];

				// make 0,0,0 into 127,127,127
				if ((image[j*4+0] == 0) && (image[j*4+0] == 0) && (image[j*4+2] == 0))
				{
					image[j*4+0] =
					image[j*4+1] =
					image[j*4+2] = 127;
				}

				image[j*4+3] = 255;
			}

			if (r_mergeLightmaps->integer)
			{
				R_UpdateSubImage(tr.deluxemaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize );
			}
			else
			{
				tr.deluxemaps[i] = R_CreateImage(va("*deluxemap%d", i), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_DELUXE, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
			}
		}
	}

	if ( r_lightmap->integer == 2 )	{
		ri.Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}

	ri.Free(image);
}


static float FatPackU(float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	lightmapnum %= (tr.fatLightmapStep * tr.fatLightmapStep);

	if(tr.fatLightmapSize > 0)
	{
		int             x = lightmapnum % tr.fatLightmapStep;

		return (input / ((float)tr.fatLightmapStep)) + ((1.0 / ((float)tr.fatLightmapStep)) * (float)x);
	}

	return input;
}

static float FatPackV(float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	lightmapnum %= (tr.fatLightmapStep * tr.fatLightmapStep);

	if(tr.fatLightmapSize > 0)
	{
		int             y = lightmapnum / tr.fatLightmapStep;

		return (input / ((float)tr.fatLightmapStep)) + ((1.0 / ((float)tr.fatLightmapStep)) * (float)y);
	}

	return input;
}


static int FatLightmap(int lightmapnum)
{
	if (lightmapnum < 0)
		return lightmapnum;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	if (tr.fatLightmapSize > 0)
	{
		return lightmapnum / (tr.fatLightmapStep * tr.fatLightmapStep);
	}
	
	return lightmapnum;
}

/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void		RE_SetWorldVisData( const byte *vis ) {
	tr.externalVisData = vis;
}


/*
=================
R_LoadVisibility
=================
*/
static	void R_LoadVisibility( lump_t *l ) {
	int		len;
	byte	*buf;

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = ri.Hunk_Alloc( len, h_low );
	Com_Memset( s_worldData.novis, 0xff, len );

    len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ((int *)buf)[0] );
	s_worldData.clusterBytes = LittleLong( ((int *)buf)[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData ) {
		s_worldData.vis = tr.externalVisData;
	} else {
		byte	*dest;

		dest = ri.Hunk_Alloc( len - 8, h_low );
		Com_Memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
	}
}

//===============================================================================


/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum, int lightmapNum ) {
	shader_t	*shader;
	dshader_t	*dsh;

	int _shaderNum = LittleLong( shaderNum );
	if ( _shaderNum < 0 || _shaderNum >= s_worldData.numShaders ) {
		ri.Error( ERR_DROP, "ShaderForShaderNum: bad num %i", _shaderNum );
	}
	dsh = &s_worldData.shaders[ _shaderNum ];

	if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		lightmapNum = LIGHTMAP_BY_VERTEX;
	}

	if ( r_fullbright->integer ) {
		lightmapNum = LIGHTMAP_WHITEIMAGE;
	}

	shader = R_FindShader( dsh->shader, lightmapNum, qtrue );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf, int *indexes  ) {
	int			i, j;
	srfSurfaceFace_t	*cv;
	srfTriangle_t  *tri;
	int			numVerts, numTriangles, badTriangles;
	int realLightmapNum;

	realLightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, FatLightmap(realLightmapNum) );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	if (numVerts > MAX_FACE_POINTS) {
		ri.Printf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numVerts);
		numVerts = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	numTriangles = LittleLong(ds->numIndexes) / 3;

	//cv = ri.Hunk_Alloc(sizeof(*cv), h_low);
	cv = (void *)surf->data;
	cv->surfaceType = SF_FACE;

	cv->numTriangles = numTriangles;
	cv->triangles = ri.Hunk_Alloc(numTriangles * sizeof(cv->triangles[0]), h_low);

	cv->numVerts = numVerts;
	cv->verts = ri.Hunk_Alloc(numVerts * sizeof(cv->verts[0]), h_low);

	// copy vertexes
	surf->cullinfo.type = CULLINFO_PLANE | CULLINFO_BOX;
	ClearBounds(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
	verts += LittleLong(ds->firstVert);
	for(i = 0; i < numVerts; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		AddPointToBounds(cv->verts[i].xyz, surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
		for(j = 0; j < 2; j++)
		{
			cv->verts[i].st[j] = LittleFloat(verts[i].st[j]);
			//cv->verts[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}
		cv->verts[i].lightmap[0] = FatPackU(LittleFloat(verts[i].lightmap[0]), realLightmapNum);
		cv->verts[i].lightmap[1] = FatPackV(LittleFloat(verts[i].lightmap[1]), realLightmapNum);

		if (hdrVertColors)
		{
			color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
			color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
			color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
		}
		else
		{
			//hack: convert LDR vertex colors to HDR
			if (r_hdr->integer)
			{
				color[0] = verts[i].color[0] + 1.0f;
				color[1] = verts[i].color[1] + 1.0f;
				color[2] = verts[i].color[2] + 1.0f;
			}
			else
			{
				color[0] = verts[i].color[0];
				color[1] = verts[i].color[1];
				color[2] = verts[i].color[2];
			}

		}
		color[3] = verts[i].color[3] / 255.0f;

		R_ColorShiftLightingFloats( color, cv->verts[i].vertexColors, 1.0f / 255.0f );
	}

	// copy triangles
	badTriangles = 0;
	indexes += LittleLong(ds->firstIndex);
	for(i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
	{
		for(j = 0; j < 3; j++)
		{
			tri->indexes[j] = LittleLong(indexes[i * 3 + j]);

			if(tri->indexes[j] < 0 || tri->indexes[j] >= numVerts)
			{
				ri.Error(ERR_DROP, "Bad index in face surface");
			}
		}

		if ((tri->indexes[0] == tri->indexes[1]) || (tri->indexes[1] == tri->indexes[2]) || (tri->indexes[0] == tri->indexes[2]))
		{
			tri--;
			badTriangles++;
		}
	}

	if (badTriangles)
	{
		ri.Printf(PRINT_WARNING, "Face has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numTriangles, numVerts, numTriangles - badTriangles);
		cv->numTriangles -= badTriangles;
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->plane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
	cv->plane.dist = DotProduct( cv->verts[0].xyz, cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );
	surf->cullinfo.plane = cv->plane;

	surf->data = (surfaceType_t *)cv;

#ifdef USE_VERT_TANGENT_SPACE
	// Tr3B - calc tangent spaces
	{
		srfVert_t      *dv[3];

		for(i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
		{
			dv[0] = &cv->verts[tri->indexes[0]];
			dv[1] = &cv->verts[tri->indexes[1]];
			dv[2] = &cv->verts[tri->indexes[2]];

			R_CalcTangentVectors(dv);
		}
	}
#endif
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh ( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf ) {
	srfGridMesh_t	*grid;
	int				i, j;
	int				width, height, numPoints;
	srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE];
	vec3_t			bounds[2];
	vec3_t			tmpVec;
	static surfaceType_t	skipData = SF_SKIP;
	int realLightmapNum;

	realLightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, FatLightmap(realLightmapNum) );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	if(width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE)
		ri.Error(ERR_DROP, "ParseMesh: bad size");

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;
	for(i = 0; i < numPoints; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			points[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			points[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		for(j = 0; j < 2; j++)
		{
			points[i].st[j] = LittleFloat(verts[i].st[j]);
			//points[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}
		points[i].lightmap[0] = FatPackU(LittleFloat(verts[i].lightmap[0]), realLightmapNum);
		points[i].lightmap[1] = FatPackV(LittleFloat(verts[i].lightmap[1]), realLightmapNum);

		if (hdrVertColors)
		{
			color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
			color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
			color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
		}
		else
		{
			//hack: convert LDR vertex colors to HDR
			if (r_hdr->integer)
			{
				color[0] = verts[i].color[0] + 1.0f;
				color[1] = verts[i].color[1] + 1.0f;
				color[2] = verts[i].color[2] + 1.0f;
			}
			else
			{
				color[0] = verts[i].color[0];
				color[1] = verts[i].color[1];
				color[2] = verts[i].color[2];
			}
		}
		color[3] = verts[i].color[3] / 255.0f;

		R_ColorShiftLightingFloats( color, points[i].vertexColors, 1.0f / 255.0f );
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
		bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
	}
	VectorAdd( bounds[0], bounds[1], bounds[1] );
	VectorScale( bounds[1], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf, int *indexes ) {
	srfTriangles_t *cv;
	srfTriangle_t  *tri;
	int             i, j;
	int             numVerts, numTriangles, badTriangles;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	numTriangles = LittleLong(ds->numIndexes) / 3;

	//cv = ri.Hunk_Alloc(sizeof(*cv), h_low);
	cv = (void *)surf->data;
	cv->surfaceType = SF_TRIANGLES;

	cv->numTriangles = numTriangles;
	cv->triangles = ri.Hunk_Alloc(numTriangles * sizeof(cv->triangles[0]), h_low);

	cv->numVerts = numVerts;
	cv->verts = ri.Hunk_Alloc(numVerts * sizeof(cv->verts[0]), h_low);

	surf->data = (surfaceType_t *) cv;

	// copy vertexes
	surf->cullinfo.type = CULLINFO_BOX;
	ClearBounds(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
	verts += LittleLong(ds->firstVert);
	for(i = 0; i < numVerts; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		AddPointToBounds( cv->verts[i].xyz, surf->cullinfo.bounds[0], surf->cullinfo.bounds[1] );

		for(j = 0; j < 2; j++)
		{
			cv->verts[i].st[j] = LittleFloat(verts[i].st[j]);
			cv->verts[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}

		if (hdrVertColors)
		{
			color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
			color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
			color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
		}
		else
		{
			//hack: convert LDR vertex colors to HDR
			if (r_hdr->integer)
			{
				color[0] = verts[i].color[0] + 1.0f;
				color[1] = verts[i].color[1] + 1.0f;
				color[2] = verts[i].color[2] + 1.0f;
			}
			else
			{
				color[0] = verts[i].color[0];
				color[1] = verts[i].color[1];
				color[2] = verts[i].color[2];
			}
		}
		color[3] = verts[i].color[3] / 255.0f;

		R_ColorShiftLightingFloats( color, cv->verts[i].vertexColors, 1.0f / 255.0f );
	}

	// copy triangles
	badTriangles = 0;
	indexes += LittleLong(ds->firstIndex);
	for(i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
	{
		for(j = 0; j < 3; j++)
		{
			tri->indexes[j] = LittleLong(indexes[i * 3 + j]);

			if(tri->indexes[j] < 0 || tri->indexes[j] >= numVerts)
			{
				ri.Error(ERR_DROP, "Bad index in face surface");
			}
		}

		if ((tri->indexes[0] == tri->indexes[1]) || (tri->indexes[1] == tri->indexes[2]) || (tri->indexes[0] == tri->indexes[2]))
		{
			tri--;
			badTriangles++;
		}
	}

	if (badTriangles)
	{
		ri.Printf(PRINT_WARNING, "Trisurf has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numTriangles, numVerts, numTriangles - badTriangles);
		cv->numTriangles -= badTriangles;
	}

#ifdef USE_VERT_TANGENT_SPACE
	// Tr3B - calc tangent spaces
	{
		srfVert_t      *dv[3];

		for(i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
		{
			dv[0] = &cv->verts[tri->indexes[0]];
			dv[1] = &cv->verts[tri->indexes[1]];
			dv[2] = &cv->verts[tri->indexes[2]];

			R_CalcTangentVectors(dv);
		}
	}
#endif
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfFlare_t		*flare;
	int				i;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	//flare = ri.Hunk_Alloc( sizeof( *flare ), h_low );
	flare = (void *)surf->data;
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
		flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
		flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints(srfGridMesh_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->width-1; i++) {
		for (j = i + 1; j < grid->width-1; j++) {
			if ( fabs(grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints(srfGridMesh_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->height-1; i++) {
		for (j = i + 1; j < grid->height-1; j++) {
			if ( fabs(grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfGridMesh_t *grid1 ) {
	int j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for ( j = start; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		touch = qfalse;
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = (grid1->height-1) * grid1->width;
			else offset1 = 0;
			if (R_MergedWidthPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->width-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = grid1->width-1;
			else offset1 = 0;
			if (R_MergedHeightPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->height-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		if (touch) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r ( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void ) {
	int i;
	srfGridMesh_t *grid1;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID )
			continue;
		//
		if ( grid1->lodFixed )
			continue;
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1);
	}
}


/*
===============
R_StitchPatches
===============
*/
int R_StitchPatches( int grid1num, int grid2num ) {
	float *v1, *v2;
	srfGridMesh_t *grid1, *grid2;
	int k, l, m, n, offset1, offset2, row, column;

	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	grid2 = (srfGridMesh_t *) s_worldData.surfaces[grid2num].data;
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->width-2; k += 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->height-2; k += 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = grid1->width-1; k > 1; k -= 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					if (!grid2)
						break;
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = grid1->height-1; k > 1; k -= 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num ) {
	int j, numstitches;
	srfGridMesh_t *grid1, *grid2;

	numstitches = 0;
	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	for ( j = 0; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		while (R_StitchPatches(grid1num, j))
		{
			numstitches++;
		}
	}
	return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches( void ) {
	int i, stitched, numstitches;
	srfGridMesh_t *grid1;

	numstitches = 0;
	do
	{
		stitched = qfalse;
		for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
			//
			grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID )
				continue;
			//
			if ( grid1->lodStitched )
				continue;
			//
			grid1->lodStitched = qtrue;
			stitched = qtrue;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while (stitched);
	ri.Printf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk(void) {
	int i, size;
	srfGridMesh_t *grid, *hunkgrid;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID )
			continue;
		//
		size = sizeof(*grid);
		hunkgrid = ri.Hunk_Alloc(size, h_low);
		Com_Memcpy(hunkgrid, grid, size);

		hunkgrid->widthLodError = ri.Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = ri.Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( hunkgrid->heightLodError, grid->heightLodError, grid->height * 4 );

		hunkgrid->numTriangles = grid->numTriangles;
		hunkgrid->triangles = ri.Hunk_Alloc(grid->numTriangles * sizeof(srfTriangle_t), h_low);
		Com_Memcpy(hunkgrid->triangles, grid->triangles, grid->numTriangles * sizeof(srfTriangle_t));

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts = ri.Hunk_Alloc(grid->numVerts * sizeof(srfVert_t), h_low);
		Com_Memcpy(hunkgrid->verts, grid->verts, grid->numVerts * sizeof(srfVert_t));

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[i].data = (void *) hunkgrid;
	}
}


/*
=================
BSPSurfaceCompare
compare function for qsort()
=================
*/
static int BSPSurfaceCompare(const void *a, const void *b)
{
	msurface_t   *aa, *bb;

	aa = *(msurface_t **) a;
	bb = *(msurface_t **) b;

	// shader first
	if(aa->shader->sortedIndex < bb->shader->sortedIndex)
		return -1;

	else if(aa->shader->sortedIndex > bb->shader->sortedIndex)
		return 1;

	// by fogIndex
	if(aa->fogIndex < bb->fogIndex)
		return -1;

	else if(aa->fogIndex > bb->fogIndex)
		return 1;

	return 0;
}


static void CopyVert(const srfVert_t * in, srfVert_t * out)
{
	int             j;

	for(j = 0; j < 3; j++)
	{
		out->xyz[j]       = in->xyz[j];
#ifdef USE_VERT_TANGENT_SPACE
		out->tangent[j]   = in->tangent[j];
		out->bitangent[j] = in->bitangent[j];
#endif
		out->normal[j]    = in->normal[j];
		out->lightdir[j]  = in->lightdir[j];
	}

	for(j = 0; j < 2; j++)
	{
		out->st[j] = in->st[j];
		out->lightmap[j] = in->lightmap[j];
	}

	for(j = 0; j < 4; j++)
	{
		out->vertexColors[j] = in->vertexColors[j];
	}
}


/*
===============
R_CreateWorldVBO
===============
*/
static void R_CreateWorldVBO(void)
{
	int             i, j, k;

	int             numVerts;
	srfVert_t      *verts;

	int             numTriangles;
	srfTriangle_t  *triangles;

    int             numSurfaces;
	msurface_t   *surface;
	msurface_t  **surfacesSorted;

	int             startTime, endTime;

	startTime = ri.Milliseconds();

	numVerts = 0;
	numTriangles = 0;
	numSurfaces = 0;
	for(k = 0, surface = &s_worldData.surfaces[0]; k < s_worldData.numsurfaces /* s_worldData.numWorldSurfaces */; k++, surface++)
	{
		if(*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *face = (srfSurfaceFace_t *) surface->data;

			if(face->numVerts)
				numVerts += face->numVerts;

			if(face->numTriangles)
				numTriangles += face->numTriangles;

			numSurfaces++;
		}
		else if(*surface->data == SF_GRID)
		{
			srfGridMesh_t  *grid = (srfGridMesh_t *) surface->data;

			if(grid->numVerts)
				numVerts += grid->numVerts;

			if(grid->numTriangles)
				numTriangles += grid->numTriangles;

			numSurfaces++;
		}
		else if(*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *tri = (srfTriangles_t *) surface->data;

			if(tri->numVerts)
				numVerts += tri->numVerts;

			if(tri->numTriangles)
				numTriangles += tri->numTriangles;

			numSurfaces++;
		}
	}

	if(!numVerts || !numTriangles)
		return;

	ri.Printf(PRINT_ALL, "...calculating world VBO ( %i verts %i tris )\n", numVerts, numTriangles);

	// create arrays

	verts = ri.Hunk_AllocateTempMemory(numVerts * sizeof(srfVert_t));

	triangles = ri.Hunk_AllocateTempMemory(numTriangles * sizeof(srfTriangle_t));

	// presort surfaces
	surfacesSorted = ri.Malloc(numSurfaces * sizeof(*surfacesSorted));

	j = 0;
	for(k = 0, surface = &s_worldData.surfaces[0]; k < s_worldData.numsurfaces; k++, surface++)
	{
		if(*surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES)
		{
			surfacesSorted[j++] = surface;
		}
	}

	qsort(surfacesSorted, numSurfaces, sizeof(*surfacesSorted), BSPSurfaceCompare);

	// set up triangle indices
	numVerts = 0;
	numTriangles = 0;
	for(k = 0, surface = surfacesSorted[k]; k < numSurfaces; k++, surface = surfacesSorted[k])
	{
		if(*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

			srf->firstIndex = numTriangles * 3;

			if(srf->numTriangles)
			{
				srfTriangle_t  *tri;

				for(i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
				{
					for(j = 0; j < 3; j++)
					{
						triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if(srf->numVerts)
				numVerts += srf->numVerts;
		}
		else if(*surface->data == SF_GRID)
		{
			srfGridMesh_t  *srf = (srfGridMesh_t *) surface->data;

			srf->firstIndex = numTriangles * 3;

			if(srf->numTriangles)
			{
				srfTriangle_t  *tri;

				for(i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
				{
					for(j = 0; j < 3; j++)
					{
						triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if(srf->numVerts)
				numVerts += srf->numVerts;
		}
		else if(*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *srf = (srfTriangles_t *) surface->data;

			srf->firstIndex = numTriangles * 3;

			if(srf->numTriangles)
			{
				srfTriangle_t  *tri;

				for(i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
				{
					for(j = 0; j < 3; j++)
					{
						triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if(srf->numVerts)
				numVerts += srf->numVerts;
		}
	}

	// build vertices
	numVerts = 0;
	for(k = 0, surface = surfacesSorted[k]; k < numSurfaces; k++, surface = surfacesSorted[k])
	{
		if(*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

			srf->firstVert = numVerts;

			if(srf->numVerts)
			{
				for(i = 0; i < srf->numVerts; i++)
				{
					CopyVert(&srf->verts[i], &verts[numVerts + i]);
				}

				numVerts += srf->numVerts;
			}
		}
		else if(*surface->data == SF_GRID)
		{
			srfGridMesh_t  *srf = (srfGridMesh_t *) surface->data;

			srf->firstVert = numVerts;

			if(srf->numVerts)
			{
				for(i = 0; i < srf->numVerts; i++)
				{
					CopyVert(&srf->verts[i], &verts[numVerts + i]);
				}

				numVerts += srf->numVerts;
			}
		}
		else if(*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *srf = (srfTriangles_t *) surface->data;

			srf->firstVert = numVerts;

			if(srf->numVerts)
			{
				for(i = 0; i < srf->numVerts; i++)
				{
					CopyVert(&srf->verts[i], &verts[numVerts + i]);
				}

				numVerts += srf->numVerts;
			}
		}
	}

#ifdef USE_VERT_TANGENT_SPACE
	s_worldData.vbo = R_CreateVBO2(va("staticBspModel0_VBO %i", 0), numVerts, verts,
								   ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BITANGENT |
								   ATTR_NORMAL | ATTR_COLOR | ATTR_LIGHTDIRECTION, VBO_USAGE_STATIC);
#else
	s_worldData.vbo = R_CreateVBO2(va("staticBspModel0_VBO %i", 0), numVerts, verts,
								   ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD |
								   ATTR_NORMAL | ATTR_COLOR | ATTR_LIGHTDIRECTION, VBO_USAGE_STATIC);
#endif

	s_worldData.ibo = R_CreateIBO2(va("staticBspModel0_IBO %i", 0), numTriangles, triangles, VBO_USAGE_STATIC);

	endTime = ri.Milliseconds();
	ri.Printf(PRINT_ALL, "world VBO calculation time = %5.2f seconds\n", (endTime - startTime) / 1000.0);

	// point triangle surfaces to world VBO
	for(k = 0, surface = surfacesSorted[k]; k < numSurfaces; k++, surface = surfacesSorted[k])
	{
		if(*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

			if( srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
			}
		}
		else if(*surface->data == SF_GRID)
		{
			srfGridMesh_t  *srf = (srfGridMesh_t *) surface->data;

			if( srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
			}
		}
		else if(*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *srf = (srfTriangles_t *) surface->data;

			if( srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
			}
		}
	}


	startTime = ri.Milliseconds();

	ri.Free(surfacesSorted);

	ri.Hunk_FreeTempMemory(triangles);
	ri.Hunk_FreeTempMemory(verts);
}

/*
===============
R_LoadSurfaces
===============
*/
static	void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump ) {
	dsurface_t	*in;
	msurface_t	*out;
	drawVert_t	*dv;
	int			*indexes;
	int			count;
	int			numFaces, numMeshes, numTriSurfs, numFlares;
	int			i;
	float *hdrVertColors = NULL;

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	in = (void *)(fileBase + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = surfs->filelen / sizeof(*in);

	dv = (void *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	indexes = (void *)(fileBase + indexLump->fileofs);
	if ( indexLump->filelen % sizeof(*indexes))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	out = ri.Hunk_Alloc ( count * sizeof(*out), h_low );	

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;
	s_worldData.surfacesViewCount = ri.Hunk_Alloc ( count * sizeof(*s_worldData.surfacesViewCount), h_low );
	s_worldData.surfacesDlightBits = ri.Hunk_Alloc ( count * sizeof(*s_worldData.surfacesDlightBits), h_low );
	s_worldData.surfacesPshadowBits = ri.Hunk_Alloc ( count * sizeof(*s_worldData.surfacesPshadowBits), h_low );

	// load hdr vertex colors
	if (r_hdr->integer)
	{
		char filename[MAX_QPATH];
		int size;

		Com_sprintf( filename, sizeof( filename ), "maps/%s/vertlight.raw", s_worldData.baseName);
		//ri.Printf(PRINT_ALL, "looking for %s\n", filename);

		size = ri.FS_ReadFile(filename, (void **)&hdrVertColors);

		if (hdrVertColors)
		{
			//ri.Printf(PRINT_ALL, "Found!\n");
			if (size != sizeof(float) * 3 * (verts->filelen / sizeof(*dv)))
				ri.Error(ERR_DROP, "Bad size for %s (%i, expected %i)!\n", filename, size, (int)(sizeof(float)) * 3 * (verts->filelen / sizeof(*dv)));
		}
	}


	// Two passes, allocate surfaces first, then load them full of data
	// This ensures surfaces are close together to reduce L2 cache misses when using VBOs,
	// which don't actually use the verts and tris
	in = (void *)(fileBase + surfs->fileofs);
	out = s_worldData.surfaces;
	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
			case MST_PATCH:
				// FIXME: do this
				break;
			case MST_TRIANGLE_SOUP:
				out->data = ri.Hunk_Alloc( sizeof(srfTriangles_t), h_low);
				break;
			case MST_PLANAR:
				out->data = ri.Hunk_Alloc( sizeof(srfSurfaceFace_t), h_low);
				break;
			case MST_FLARE:
				out->data = ri.Hunk_Alloc( sizeof(srfFlare_t), h_low);
				break;
			default:
				break;
		}
	}

	in = (void *)(fileBase + surfs->fileofs);
	out = s_worldData.surfaces;
	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
		case MST_PATCH:
			ParseMesh ( in, dv, hdrVertColors, out );
			{
				srfGridMesh_t *surface = (srfGridMesh_t *)out->data;

				out->cullinfo.type = CULLINFO_BOX | CULLINFO_SPHERE;
				VectorCopy(surface->meshBounds[0], out->cullinfo.bounds[0]);
				VectorCopy(surface->meshBounds[1], out->cullinfo.bounds[1]);
				VectorCopy(surface->localOrigin, out->cullinfo.localOrigin);
				out->cullinfo.radius = surface->meshRadius;
			}
			numMeshes++;
			break;
		case MST_TRIANGLE_SOUP:
			ParseTriSurf( in, dv, hdrVertColors, out, indexes );
			numTriSurfs++;
			break;
		case MST_PLANAR:
			ParseFace( in, dv, hdrVertColors, out, indexes );
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare( in, dv, out, indexes );
			{
				out->cullinfo.type = CULLINFO_NONE;
			}
			numFlares++;
			break;
		default:
			ri.Error( ERR_DROP, "Bad surfaceType" );
		}
	}

	if (hdrVertColors)
	{
		ri.FS_FreeFile(hdrVertColors);
	}

#ifdef PATCH_STITCHING
	R_StitchAllPatches();
#endif

	R_FixSharedVertexLodError();

#ifdef PATCH_STITCHING
	R_MovePatchSurfacesToHunk();
#endif

	ri.Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", 
		numFaces, numMeshes, numTriSurfs, numFlares );
}



/*
=================
R_LoadSubmodels
=================
*/
static	void R_LoadSubmodels( lump_t *l ) {
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);

	s_worldData.numBModels = count;
	s_worldData.bmodels = out = ri.Hunk_Alloc( count * sizeof(*out), h_low );

	for ( i=0 ; i<count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );			// this should never happen
		if ( model == NULL ) {
			ri.Error(ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed");
		}

		model->type = MOD_BRUSH;
		model->bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for (j=0 ; j<3 ; j++) {
			out->bounds[0][j] = LittleFloat (in->mins[j]);
			out->bounds[1][j] = LittleFloat (in->maxs[j]);
		}

		out->firstSurface = LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );

		if(i == 0)
		{
			// Tr3B: add this for limiting VBO surface creation
			s_worldData.numWorldSurfaces = out->numSurfaces;
		}
	}
}



//==================================================================

/*
=================
R_SetParent
=================
*/
static	void R_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	R_SetParent (node->children[0], node);
	R_SetParent (node->children[1], node);
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static	void R_LoadNodesAndLeafs (lump_t *nodeLump, lump_t *leafLump) {
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*out;
	int			numNodes, numLeafs;

	in = (void *)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(dnode_t) ||
		leafLump->filelen % sizeof(dleaf_t) ) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	numNodes = nodeLump->filelen / sizeof(dnode_t);
	numLeafs = leafLump->filelen / sizeof(dleaf_t);

	out = ri.Hunk_Alloc ( (numNodes + numLeafs) * sizeof(*out), h_low);	

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// load nodes
	for ( i=0 ; i<numNodes; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (in->mins[j]);
			out->maxs[j] = LittleLong (in->maxs[j]);
		}
	
		p = LittleLong(in->planeNum);
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = s_worldData.nodes + p;
			else
				out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
		}
	}
	
	// load leafs
	inLeaf = (void *)(fileBase + leafLump->fileofs);
	for ( i=0 ; i<numLeafs ; i++, inLeaf++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (inLeaf->mins[j]);
			out->maxs[j] = LittleLong (inLeaf->maxs[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if ( out->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = LittleLong(inLeaf->firstLeafSurface);
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}	

	// chain decendants
	R_SetParent (s_worldData.nodes, NULL);
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static	void R_LoadShaders( lump_t *l ) {	
	int		i, count;
	dshader_t	*in, *out;
	
	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri.Hunk_Alloc ( count*sizeof(*out), h_low );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	Com_Memcpy( out, in, count*sizeof(*out) );

	for ( i=0 ; i<count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
	}
}


/*
=================
R_LoadMarksurfaces
=================
*/
static	void R_LoadMarksurfaces (lump_t *l)
{	
	int		i, j, count;
	int		*in;
	int     *out;
	
	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri.Hunk_Alloc ( count*sizeof(*out), h_low);	

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		out[i] = j;
	}
}


/*
=================
R_LoadPlanes
=================
*/
static	void R_LoadPlanes( lump_t *l ) {
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri.Hunk_Alloc ( count*2*sizeof(*out), h_low);	
	
	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++) {
		bits = 0;
		for (j=0 ; j<3 ; j++) {
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0) {
				bits |= 1<<j;
			}
		}

		out->dist = LittleFloat (in->dist);
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
R_LoadFogs

=================
*/
static	void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump ) {
	int			i;
	fog_t		*out;
	dfog_t		*fogs;
	dbrush_t 	*brushes, *brush;
	dbrushside_t	*sides;
	int			count, brushesCount, sidesCount;
	int			sideNum;
	int			planeNum;
	shader_t	*shader;
	float		d;
	int			firstSide;

	fogs = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs)) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = ri.Hunk_Alloc ( s_worldData.numfogs*sizeof(*out), h_low);
	out = s_worldData.fogs + 1;

	if ( !count ) {
		return;
	}

	brushes = (void *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes)) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (void *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides)) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	sidesCount = sidesLump->filelen / sizeof(*sides);

	for ( i=0 ; i<count ; i++, fogs++) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		if ( (unsigned)out->originalBrushNumber >= brushesCount ) {
			ri.Error( ERR_DROP, "fog brushNumber out of range" );
		}
		brush = brushes + out->originalBrushNumber;

		firstSide = LittleLong( brush->firstSide );

			if ( (unsigned)firstSide > sidesCount - 6 ) {
			ri.Error( ERR_DROP, "fog brush sideNumber out of range" );
		}

		// brushes are always sorted with the axial sides first
		sideNum = firstSide + 0;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][0] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 1;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][0] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 2;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][1] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 3;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][1] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 4;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][2] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 5;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][2] = s_worldData.planes[ planeNum ].dist;

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, LIGHTMAP_NONE, qtrue );

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4 ( shader->fogParms.color[0] * tr.identityLight, 
			                          shader->fogParms.color[1] * tr.identityLight, 
			                          shader->fogParms.color[2] * tr.identityLight, 1.0 );

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		if ( sideNum == -1 ) {
			out->hasSurface = qfalse;
		} else {
			out->hasSurface = qtrue;
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[3] = -s_worldData.planes[ planeNum ].dist;
		}

		out++;
	}

}


/*
================
R_LoadLightGrid

================
*/
void R_LoadLightGrid( lump_t *l ) {
	int		i;
	vec3_t	maxs;
	int		numGridPoints;
	world_t	*w;
	float	*wMins, *wMaxs;

	w = &s_worldData;

	w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

	wMins = w->bmodels[0].bounds[0];
	wMaxs = w->bmodels[0].bounds[1];

	for ( i = 0 ; i < 3 ; i++ ) {
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
		maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i])/w->lightGridSize[i] + 1;
	}

	numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if ( l->filelen != numGridPoints * 8 ) {
		ri.Printf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridData = ri.Hunk_Alloc( l->filelen, h_low );
	Com_Memcpy( w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen );

	// deal with overbright bits
	for ( i = 0 ; i < numGridPoints ; i++ ) {
		R_ColorShiftLightingBytes( &w->lightGridData[i*8], &w->lightGridData[i*8] );
		R_ColorShiftLightingBytes( &w->lightGridData[i*8+3], &w->lightGridData[i*8+3] );
	}

	// load hdr lightgrid
	if (r_hdr->integer)
	{
		char filename[MAX_QPATH];
		float *hdrLightGrid;
		int size;

		Com_sprintf( filename, sizeof( filename ), "maps/%s/lightgrid.raw", s_worldData.baseName);
		//ri.Printf(PRINT_ALL, "looking for %s\n", filename);

		size = ri.FS_ReadFile(filename, (void **)&hdrLightGrid);

		if (hdrLightGrid)
		{
			float lightScale = pow(2, r_mapOverBrightBits->integer - tr.overbrightBits);

			//ri.Printf(PRINT_ALL, "found!\n");

			if (size != sizeof(float) * 6 * numGridPoints)
			{
				ri.Error(ERR_DROP, "Bad size for %s (%i, expected %i)!\n", filename, size, (int)(sizeof(float)) * 6 * numGridPoints);
			}

			w->hdrLightGrid = ri.Hunk_Alloc(size, h_low);

			for (i = 0; i < numGridPoints ; i++)
			{
				w->hdrLightGrid[i * 6    ] = hdrLightGrid[i * 6    ] * lightScale;
				w->hdrLightGrid[i * 6 + 1] = hdrLightGrid[i * 6 + 1] * lightScale;
				w->hdrLightGrid[i * 6 + 2] = hdrLightGrid[i * 6 + 2] * lightScale;
				w->hdrLightGrid[i * 6 + 3] = hdrLightGrid[i * 6 + 3] * lightScale;
				w->hdrLightGrid[i * 6 + 4] = hdrLightGrid[i * 6 + 4] * lightScale;
				w->hdrLightGrid[i * 6 + 5] = hdrLightGrid[i * 6 + 5] * lightScale;
			}
		}

		if (hdrLightGrid)
			ri.FS_FreeFile(hdrLightGrid);
	}
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( lump_t *l ) {
	char *p, *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t	*w;

	w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	p = (char *)(fileBase + l->fileofs);

	// store for reference by the cgame
	w->entityString = ri.Hunk_Alloc( l->filelen + 1, h_low );
	strcpy( w->entityString, p );
	w->entityParsePoint = w->entityString;

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {	
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(keyname, token, sizeof(keyname));

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(value, token, sizeof(value));

		// check for remapping of shaders for vertex lighting
		s = "vertexremapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			if (r_vertexLight->integer) {
				R_RemapShader(value, s, "0");
			}
			continue;
		}
		// check for remapping of shaders
		s = "remapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			R_RemapShader(value, s, "0");
			continue;
		}
		// check for a different grid size
		if (!Q_stricmp(keyname, "gridsize")) {
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
			continue;
		}

		// check for auto exposure
		if (!Q_stricmp(keyname, "autoExposureMinMax")) {
			sscanf(value, "%f %f", &tr.autoExposureMinMax[0], &tr.autoExposureMinMax[1]);
			continue;
		}
	}
}

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size ) {
	const char	*s;

	s = COM_Parse( &s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[0] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}


/*
=================
R_MergeLeafSurfaces

Merges surfaces that share a common leaf
=================
*/
void R_MergeLeafSurfaces(void)
{
	int i, j, k;
	int numWorldSurfaces;
	int mergedSurfIndex;
	int numMergedSurfaces;
	int numUnmergedSurfaces;
	IBO_t *ibo;

	msurface_t *mergedSurf;

	glIndex_t *iboIndexes, *outIboIndexes;
	int numIboIndexes;

	int startTime, endTime;

	startTime = ri.Milliseconds();

	numWorldSurfaces = s_worldData.numWorldSurfaces;

	// use viewcount to keep track of mergers
	for (i = 0; i < numWorldSurfaces; i++)
	{
		s_worldData.surfacesViewCount[i] = -1;
	}

	// create ibo
	ibo = tr.ibos[tr.numIBOs++] = ri.Hunk_Alloc(sizeof(*ibo), h_low);
	memset(ibo, 0, sizeof(*ibo));
	Q_strncpyz(ibo->name, "staticWorldMesh_IBO_mergedSurfs", sizeof(ibo->name));

	// allocate more than we need
	iboIndexes = outIboIndexes = ri.Malloc(s_worldData.ibo->indexesSize);

	// mark matching surfaces
	for (i = 0; i < s_worldData.numnodes - s_worldData.numDecisionNodes; i++)
	{
		mnode_t *leaf = s_worldData.nodes + s_worldData.numDecisionNodes + i;

		for (j = 0; j < leaf->nummarksurfaces; j++)
		{
			msurface_t *surf1;
			shader_t *shader1;
			int fogIndex1;
			int surfNum1;

			surfNum1 = *(s_worldData.marksurfaces + leaf->firstmarksurface + j);

			if (s_worldData.surfacesViewCount[surfNum1] != -1)
				continue;

			surf1 = s_worldData.surfaces + surfNum1;

			if ((*surf1->data != SF_GRID) && (*surf1->data != SF_TRIANGLES) && (*surf1->data != SF_FACE))
				continue;

			shader1 = surf1->shader;

			if(shader1->isSky)
				continue;

			if(shader1->isPortal)
				continue;

			if(ShaderRequiresCPUDeforms(shader1))
				continue;

			fogIndex1 = surf1->fogIndex;

			s_worldData.surfacesViewCount[surfNum1] = surfNum1;

			for (k = j + 1; k < leaf->nummarksurfaces; k++)
			{
				msurface_t *surf2;
				shader_t *shader2;
				int fogIndex2;
				int surfNum2;

				surfNum2 = *(s_worldData.marksurfaces + leaf->firstmarksurface + k);

				if (s_worldData.surfacesViewCount[surfNum2] != -1)
					continue;
				
				surf2 = s_worldData.surfaces + surfNum2;

				if ((*surf2->data != SF_GRID) && (*surf2->data != SF_TRIANGLES) && (*surf2->data != SF_FACE))
					continue;

				shader2 = surf2->shader;

				if (shader1 != shader2)
					continue;

				fogIndex2 = surf2->fogIndex;

				if (fogIndex1 != fogIndex2)
					continue;

				s_worldData.surfacesViewCount[surfNum2] = surfNum1;
			}
		}
	}

	// count merged/unmerged surfaces
	numMergedSurfaces = 0;
	numUnmergedSurfaces = 0;
	for (i = 0; i < numWorldSurfaces; i++)
	{
		if (s_worldData.surfacesViewCount[i] == i)
		{
			numMergedSurfaces++;
		}
		else if (s_worldData.surfacesViewCount[i] == -1)
		{
			numUnmergedSurfaces++;
		}
	}

	// Allocate merged surfaces
	s_worldData.mergedSurfaces = ri.Hunk_Alloc(sizeof(*s_worldData.mergedSurfaces) * numMergedSurfaces, h_low);
	s_worldData.mergedSurfacesViewCount = ri.Hunk_Alloc(sizeof(*s_worldData.mergedSurfacesViewCount) * numMergedSurfaces, h_low);
	s_worldData.mergedSurfacesDlightBits = ri.Hunk_Alloc(sizeof(*s_worldData.mergedSurfacesDlightBits) * numMergedSurfaces, h_low);
	s_worldData.numMergedSurfaces = numMergedSurfaces;
	
	// view surfaces are like mark surfaces, except negative ones represent merged surfaces
	// -1 represents 0, -2 represents 1, and so on
	s_worldData.viewSurfaces = ri.Hunk_Alloc(sizeof(*s_worldData.viewSurfaces) * s_worldData.nummarksurfaces, h_low);

	// copy view surfaces into mark surfaces
	for (i = 0; i < s_worldData.nummarksurfaces; i++)
	{
		s_worldData.viewSurfaces[i] = s_worldData.marksurfaces[i];
	}

	// actually merge surfaces
	numIboIndexes = 0;
	mergedSurfIndex = 0;
	mergedSurf = s_worldData.mergedSurfaces;
	for (i = 0; i < numWorldSurfaces; i++)
	{
		msurface_t *surf1;

		vec3_t bounds[2];

		int numSurfsToMerge;
		int numTriangles;
		int numVerts;
		int firstIndex;

		srfVBOMesh_t *vboSurf;

		if (s_worldData.surfacesViewCount[i] != i)
			continue;

		surf1 = s_worldData.surfaces + i;

		// count verts, indexes, and surfaces
		numSurfsToMerge = 0;
		numTriangles = 0;
		numVerts = 0;
		for (j = i; j < numWorldSurfaces; j++)
		{
			msurface_t *surf2;

			if (s_worldData.surfacesViewCount[j] != i)
				continue;

			surf2 = s_worldData.surfaces + j;

			switch(*surf2->data)
			{
				case SF_FACE:
					{
						srfSurfaceFace_t *face;

						face = (srfSurfaceFace_t *) surf2->data;
						numTriangles += face->numTriangles;
						numVerts += face->numVerts;
					}
					break;

				case SF_GRID:
					{
						srfGridMesh_t *grid;

						grid = (srfGridMesh_t *) surf2->data;
						numTriangles += grid->numTriangles;
						numVerts += grid->numVerts;
					}
					break;

				case SF_TRIANGLES:
					{
						srfTriangles_t *tris;

						tris = (srfTriangles_t *) surf2->data;
						numTriangles += tris->numTriangles;
						numVerts += tris->numVerts;
					}
					break;

				default:
					break;
			}

			numSurfsToMerge++;
		}

		if (numVerts == 0 || numTriangles == 0 || numSurfsToMerge < 2)
		{
			continue;
		}

		// Merge surfaces (indexes) and calculate bounds
		ClearBounds(bounds[0], bounds[1]);
		firstIndex = numIboIndexes;
		for (j = i; j < numWorldSurfaces; j++)
		{
			msurface_t *surf2;

			if (s_worldData.surfacesViewCount[j] != i)
				continue;

			surf2 = s_worldData.surfaces + j;

			AddPointToBounds(surf2->cullinfo.bounds[0], bounds[0], bounds[1]);
			AddPointToBounds(surf2->cullinfo.bounds[1], bounds[0], bounds[1]);

			switch(*surf2->data)
			{
				case SF_FACE:
					{
						srfSurfaceFace_t *face;

						face = (srfSurfaceFace_t *) surf2->data;

						for (k = 0; k < face->numTriangles; k++)
						{
							*outIboIndexes++ = face->triangles[k].indexes[0] + face->firstVert;
							*outIboIndexes++ = face->triangles[k].indexes[1] + face->firstVert;
							*outIboIndexes++ = face->triangles[k].indexes[2] + face->firstVert;
							numIboIndexes += 3;
						}
					}
					break;

				case SF_GRID:
					{
						srfGridMesh_t *grid;

						grid = (srfGridMesh_t *) surf2->data;

						for (k = 0; k < grid->numTriangles; k++)
						{
							*outIboIndexes++ = grid->triangles[k].indexes[0] + grid->firstVert;
							*outIboIndexes++ = grid->triangles[k].indexes[1] + grid->firstVert;
							*outIboIndexes++ = grid->triangles[k].indexes[2] + grid->firstVert;
							numIboIndexes += 3;
						}
					}
					break;

				case SF_TRIANGLES:
					{
						srfTriangles_t *tris;

						tris = (srfTriangles_t *) surf2->data;

						for (k = 0; k < tris->numTriangles; k++)
						{
							*outIboIndexes++ = tris->triangles[k].indexes[0] + tris->firstVert;
							*outIboIndexes++ = tris->triangles[k].indexes[1] + tris->firstVert;
							*outIboIndexes++ = tris->triangles[k].indexes[2] + tris->firstVert;
							numIboIndexes += 3;
						}
					}
					break;

				// never happens, but silences a compile warning
				default:
					break;
			}
		}

		vboSurf = ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
		memset(vboSurf, 0, sizeof(*vboSurf));
		vboSurf->surfaceType = SF_VBO_MESH;

		vboSurf->vbo = s_worldData.vbo;
		vboSurf->ibo = ibo;

		vboSurf->numIndexes = numTriangles * 3;
		vboSurf->numVerts = numVerts;
		vboSurf->firstIndex = firstIndex;

		vboSurf->shader = surf1->shader;
		vboSurf->fogIndex = surf1->fogIndex;

		VectorCopy(bounds[0], vboSurf->bounds[0]);
		VectorCopy(bounds[1], vboSurf->bounds[1]);

		VectorCopy(bounds[0], mergedSurf->cullinfo.bounds[0]);
		VectorCopy(bounds[1], mergedSurf->cullinfo.bounds[1]);

		mergedSurf->cullinfo.type = CULLINFO_BOX;
		mergedSurf->data          = (surfaceType_t *)vboSurf;
		mergedSurf->fogIndex      = surf1->fogIndex;
		mergedSurf->shader        = surf1->shader;

		// redirect view surfaces to this surf
		for (j = i; j < numWorldSurfaces; j++)
		{
			if (s_worldData.surfacesViewCount[j] != i)
				continue;

			for (k = 0; k < s_worldData.nummarksurfaces; k++)
			{
				int *mark = s_worldData.marksurfaces + k;
				int *view = s_worldData.viewSurfaces + k;

				if (*mark == j)
					*view = -(mergedSurfIndex + 1);
			}
		}

		mergedSurfIndex++;
		mergedSurf++;
	}

	// finish up the ibo
	R_SyncRenderThread();

	qglGenBuffersARB(1, &ibo->indexesVBO);

	R_BindIBO(ibo);

	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, numIboIndexes * sizeof(*iboIndexes), iboIndexes, GL_STATIC_DRAW_ARB);

	R_BindNullIBO();

	GL_CheckErrors();

	ri.Free(iboIndexes);

	endTime = ri.Milliseconds();

	ri.Printf(PRINT_ALL, "Processed %d surfaces into %d merged, %d unmerged in %5.2f seconds\n", 
		numWorldSurfaces, numMergedSurfaces, numUnmergedSurfaces, (endTime - startTime) / 1000.0f);

	// reset viewcounts
	for (i = 0; i < numWorldSurfaces; i++)
	{
		s_worldData.surfacesViewCount[i] = -1;
	}
}


void R_CalcVertexLightDirs( void )
{
	int i, k;
	msurface_t *surface;

	for(k = 0, surface = &s_worldData.surfaces[0]; k < s_worldData.numsurfaces /* s_worldData.numWorldSurfaces */; k++, surface++)
	{
		if(*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

			if(srf->numVerts)
			{
				for(i = 0; i < srf->numVerts; i++)
				{
					R_LightDirForPoint( srf->verts[i].xyz, srf->verts[i].lightdir, srf->verts[i].normal, &s_worldData );
				}
			}
		}
		else if(*surface->data == SF_GRID)
		{
			srfGridMesh_t  *srf = (srfGridMesh_t *) surface->data;

			if(srf->numVerts)
			{
				for(i = 0; i < srf->numVerts; i++)
				{
					R_LightDirForPoint( srf->verts[i].xyz, srf->verts[i].lightdir, srf->verts[i].normal, &s_worldData );
				}
			}
		}
		else if(*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *srf = (srfTriangles_t *) surface->data;

			if(srf->numVerts)
			{
				for(i = 0; i < srf->numVerts; i++)
				{
					R_LightDirForPoint( srf->verts[i].xyz, srf->verts[i].lightdir, srf->verts[i].normal, &s_worldData );
				}
			}
		}
	}
}


/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap( const char *name ) {
	int			i;
	dheader_t	*header;
	union {
		byte *b;
		void *v;
	} buffer;
	byte		*startMarker;

	if ( tr.worldMapLoaded ) {
		ri.Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );
	}

	// set default map light scale
	tr.mapLightScale = 1.0f;

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45f;
	tr.sunDirection[1] = 0.3f;
	tr.sunDirection[2] = 0.9f;

	VectorNormalize( tr.sunDirection );

	// set default autoexposure settings
	tr.autoExposureMinMax[0] = -2.0f;
	tr.autoExposureMinMax[1] = 2.0f;

	// set default tone mapping settings
	tr.toneMinAvgMaxLevel[0] = -3.25f;
	tr.toneMinAvgMaxLevel[1] = -1.0f;
	tr.toneMinAvgMaxLevel[2] = 1.0f;

	tr.worldMapLoaded = qtrue;

	// load it
    ri.FS_ReadFile( name, &buffer.v );
	if ( !buffer.b ) {
		ri.Error (ERR_DROP, "RE_LoadWorldMap: %s not found", name);
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	Com_Memset( &s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension(s_worldData.baseName, s_worldData.baseName, sizeof(s_worldData.baseName));

	startMarker = ri.Hunk_Alloc(0, h_low);
	c_gridVerts = 0;

	header = (dheader_t *)buffer.b;
	fileBase = (byte *)header;

	i = LittleLong (header->version);
	if ( i != BSP_VERSION ) {
		ri.Error (ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", 
			name, i, BSP_VERSION);
	}

	// swap all the lumps
	for (i=0 ; i<sizeof(dheader_t)/4 ; i++) {
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);
	}

	// load into heap
	R_LoadEntities( &header->lumps[LUMP_ENTITIES] );
	R_LoadShaders( &header->lumps[LUMP_SHADERS] );
	R_LoadLightmaps( &header->lumps[LUMP_LIGHTMAPS], &header->lumps[LUMP_SURFACES] );
	R_LoadPlanes (&header->lumps[LUMP_PLANES]);
	R_LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES] );
	R_LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES] );
	R_LoadMarksurfaces (&header->lumps[LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs (&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);
	R_LoadSubmodels (&header->lumps[LUMP_MODELS]);
	R_LoadVisibility( &header->lumps[LUMP_VISIBILITY] );
	R_LoadLightGrid( &header->lumps[LUMP_LIGHTGRID] );

	// determine vertex light directions
	R_CalcVertexLightDirs();

	// create static VBOS from the world
	R_CreateWorldVBO();
	if (r_mergeLeafSurfaces->integer)
	{
		R_MergeLeafSurfaces();
	}

	s_worldData.dataSize = (byte *)ri.Hunk_Alloc(0, h_low) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	// make sure the VBO glState entries are safe
	R_BindNullVBO();
	R_BindNullIBO();

    ri.FS_FreeFile( buffer.v );
}
