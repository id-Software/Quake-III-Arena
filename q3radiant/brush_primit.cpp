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
#include "qe3.h"

// compute a determinant using Sarrus rule
//++timo "inline" this with a macro
// NOTE : the three vec3_t are understood as columns of the matrix
vec_t SarrusDet(vec3_t a, vec3_t b, vec3_t c)
{
	return a[0]*b[1]*c[2]+b[0]*c[1]*a[2]+c[0]*a[1]*b[2]
		-c[0]*b[1]*a[2]-a[1]*b[0]*c[2]-a[0]*b[2]*c[1];
}

//++timo replace everywhere texX by texS etc. ( ----> and in q3map !) 
// NOTE : ComputeAxisBase here and in q3map code must always BE THE SAME !
// WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
// rotation by (0,RotY,RotZ) assigns X to normal
void ComputeAxisBase(vec3_t normal,vec3_t texS,vec3_t texT )
{
	vec_t RotY,RotZ;
	// do some cleaning
	if (fabs(normal[0])<1e-6)
		normal[0]=0.0f;
	if (fabs(normal[1])<1e-6)
		normal[1]=0.0f;
	if (fabs(normal[2])<1e-6)
		normal[2]=0.0f;
	RotY=-atan2(normal[2],sqrt(normal[1]*normal[1]+normal[0]*normal[0]));
	RotZ=atan2(normal[1],normal[0]);
	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0]=-sin(RotZ);
	texS[1]=cos(RotZ);
	texS[2]=0;
	// the texT vector is along -Z ( T texture coorinates axis )
	texT[0]=-sin(RotY)*cos(RotZ);
	texT[1]=-sin(RotY)*sin(RotZ);
	texT[2]=-cos(RotY);
}

void FaceToBrushPrimitFace(face_t *f)
{
	vec3_t texX,texY;
	vec3_t proj;
	// ST of (0,0) (1,0) (0,1)
	vec_t ST[3][5]; // [ point index ] [ xyz ST ]
	//++timo not used as long as brushprimit_texdef and texdef are static
/*	f->brushprimit_texdef.contents=f->texdef.contents;
	f->brushprimit_texdef.flags=f->texdef.flags;
	f->brushprimit_texdef.value=f->texdef.value;
	strcpy(f->brushprimit_texdef.name,f->texdef.name); */
#ifdef _DEBUG
	if ( f->plane.normal[0]==0.0f && f->plane.normal[1]==0.0f && f->plane.normal[2]==0.0f )
	{
		Sys_Printf("Warning : f->plane.normal is (0,0,0) in FaceToBrushPrimitFace\n");
	}
	// check d_texture
	if (!f->d_texture)
	{
		Sys_Printf("Warning : f.d_texture is NULL in FaceToBrushPrimitFace\n");
		return;
	}
#endif
	// compute axis base
	ComputeAxisBase(f->plane.normal,texX,texY);
	// compute projection vector
	VectorCopy(f->plane.normal,proj);
	VectorScale(proj,f->plane.dist,proj);
	// (0,0) in plane axis base is (0,0,0) in world coordinates + projection on the affine plane
	// (1,0) in plane axis base is texX in world coordinates + projection on the affine plane
	// (0,1) in plane axis base is texY in world coordinates + projection on the affine plane
	// use old texture code to compute the ST coords of these points
	VectorCopy(proj,ST[0]);
	EmitTextureCoordinates(ST[0], f->d_texture, f);
	VectorCopy(texX,ST[1]);
	VectorAdd(ST[1],proj,ST[1]);
	EmitTextureCoordinates(ST[1], f->d_texture, f);
	VectorCopy(texY,ST[2]);
	VectorAdd(ST[2],proj,ST[2]);
	EmitTextureCoordinates(ST[2], f->d_texture, f);
	// compute texture matrix
	f->brushprimit_texdef.coords[0][2]=ST[0][3];
	f->brushprimit_texdef.coords[1][2]=ST[0][4];
	f->brushprimit_texdef.coords[0][0]=ST[1][3]-f->brushprimit_texdef.coords[0][2];
	f->brushprimit_texdef.coords[1][0]=ST[1][4]-f->brushprimit_texdef.coords[1][2];
	f->brushprimit_texdef.coords[0][1]=ST[2][3]-f->brushprimit_texdef.coords[0][2];
	f->brushprimit_texdef.coords[1][1]=ST[2][4]-f->brushprimit_texdef.coords[1][2];
}

// compute texture coordinates for the winding points
void EmitBrushPrimitTextureCoordinates(face_t * f, winding_t * w)
{
	vec3_t texX,texY;
	vec_t x,y;
	// compute axis base
	ComputeAxisBase(f->plane.normal,texX,texY);
	// in case the texcoords matrix is empty, build a default one
	// same behaviour as if scale[0]==0 && scale[1]==0 in old code
	if (f->brushprimit_texdef.coords[0][0]==0 && f->brushprimit_texdef.coords[1][0]==0 && f->brushprimit_texdef.coords[0][1]==0 && f->brushprimit_texdef.coords[1][1]==0)
	{
		f->brushprimit_texdef.coords[0][0] = 1.0f;
		f->brushprimit_texdef.coords[1][1] = 1.0f;
		ConvertTexMatWithQTexture( &f->brushprimit_texdef, NULL, &f->brushprimit_texdef, f->d_texture );
	}
	int i;
    for (i=0 ; i<w->numpoints ; i++)
	{
		x=DotProduct(w->points[i],texX);
		y=DotProduct(w->points[i],texY);
#ifdef _DEBUG
		if (g_qeglobals.bNeedConvert)
		{
			// check we compute the same ST as the traditional texture computation used before
			vec_t S=f->brushprimit_texdef.coords[0][0]*x+f->brushprimit_texdef.coords[0][1]*y+f->brushprimit_texdef.coords[0][2];
			vec_t T=f->brushprimit_texdef.coords[1][0]*x+f->brushprimit_texdef.coords[1][1]*y+f->brushprimit_texdef.coords[1][2];
			if ( fabs(S-w->points[i][3])>1e-2 || fabs(T-w->points[i][4])>1e-2 )
			{
				if ( fabs(S-w->points[i][3])>1e-4 || fabs(T-w->points[i][4])>1e-4 )
					Sys_Printf("Warning : precision loss in brush -> brush primitive texture computation\n");
				else
					Sys_Printf("Warning : brush -> brush primitive texture computation bug detected\n");
			}
		}
#endif
		w->points[i][3]=f->brushprimit_texdef.coords[0][0]*x+f->brushprimit_texdef.coords[0][1]*y+f->brushprimit_texdef.coords[0][2];
		w->points[i][4]=f->brushprimit_texdef.coords[1][0]*x+f->brushprimit_texdef.coords[1][1]*y+f->brushprimit_texdef.coords[1][2];
	}
}

// parse a brush in brush primitive format
void BrushPrimit_Parse(brush_t	*b)
{
	epair_t		*ep;
	face_t		*f;
	int			i,j;
	GetToken (true);
	if (strcmp (token, "{"))
	{
		Warning ("parsing brush primitive");
		return;
	}
	do
	{
		if (!GetToken (true))
			break;
		if (!strcmp (token, "}") )
			break;
		// reading of b->epairs if any
		if (strcmp (token, "(") )
		{
			ep = ParseEpair();
			ep->next = b->epairs;
			b->epairs = ep;
		}
		else
		// it's a face
		{
			f = Face_Alloc();
			f->next = NULL;
			if (!b->brush_faces)
			  	b->brush_faces = f;
		  	else
			{
				face_t *scan;
				for (scan=b->brush_faces ; scan->next ; scan=scan->next)
					;
				scan->next = f;
		  	}

			// read the three point plane definition
			for (i=0 ; i<3 ; i++)
			{
				if (i != 0)
					GetToken (true);
				if (strcmp (token, "(") )
				{
					Warning ("parsing brush");
					return;
				}
				for (j=0 ; j<3 ; j++)
				{
					GetToken (false);
					f->planepts[i][j] = atof(token);
				}
				GetToken (false);
				if (strcmp (token, ")") )
				{
					Warning ("parsing brush");
					return;
				}
			}
			// texture coordinates
			GetToken (false);
			if (strcmp(token, "("))
			{
				Warning ("parsing brush primitive");
				return;
			}
			GetToken (false);
			if (strcmp(token, "("))
			{
				Warning ("parsing brush primitive");
				return;
			}
			for (j=0;j<3;j++)
			{
				GetToken(false);
				f->brushprimit_texdef.coords[0][j]=atof(token);
			}
			GetToken (false);
			if (strcmp(token, ")"))
			{
				Warning ("parsing brush primitive");
				return;
			}
			GetToken (false);
			if (strcmp(token, "("))
			{
				Warning ("parsing brush primitive");
				return;
			}
			for (j=0;j<3;j++)
			{
				GetToken(false);
				f->brushprimit_texdef.coords[1][j]=atof(token);
			}
			GetToken (false);
			if (strcmp(token, ")"))
			{
				Warning ("parsing brush primitive");
				return;
			}
			GetToken (false);
			if (strcmp(token, ")"))
			{
				Warning ("parsing brush primitive");
				return;
			}
			// read the texturedef
			GetToken (false);
			//strcpy(f->texdef.name, token);
			f->texdef.SetName(token);
			if (TokenAvailable ())
			{
				GetToken (false);
				f->texdef.contents = atoi(token);
        GetToken (false);
				f->texdef.flags = atoi(token);
				GetToken (false);
				f->texdef.value = atoi(token);
			}
		}
	} while (1);
}

// compute a fake shift scale rot representation from the texture matrix
// these shift scale rot values are to be understood in the local axis base
void TexMatToFakeTexCoords( vec_t texMat[2][3], float shift[2], float *rot, float scale[2] )
{
#ifdef _DEBUG
	// check this matrix is orthogonal
	if (fabs(texMat[0][0]*texMat[0][1]+texMat[1][0]*texMat[1][1])>ZERO_EPSILON)
		Sys_Printf("Warning : non orthogonal texture matrix in TexMatToFakeTexCoords\n");
#endif
	scale[0]=sqrt(texMat[0][0]*texMat[0][0]+texMat[1][0]*texMat[1][0]);
	scale[1]=sqrt(texMat[0][1]*texMat[0][1]+texMat[1][1]*texMat[1][1]);
#ifdef _DEBUG
	if (scale[0]<ZERO_EPSILON || scale[1]<ZERO_EPSILON)
		Sys_Printf("Warning : unexpected scale==0 in TexMatToFakeTexCoords\n");
#endif
	// compute rotate value
	if (fabs(texMat[0][0])<ZERO_EPSILON)
	{
#ifdef _DEBUG
		// check brushprimit_texdef[1][0] is not zero
		if (fabs(texMat[1][0])<ZERO_EPSILON)
			Sys_Printf("Warning : unexpected texdef[1][0]==0 in TexMatToFakeTexCoords\n");
#endif
		// rotate is +-90
		if (texMat[1][0]>0)
			*rot=90.0f;
		else
			*rot=-90.0f;
	}
	else
	*rot = RAD2DEG( atan2( texMat[1][0], texMat[0][0] ) );
	shift[0] = -texMat[0][2];
	shift[1] = texMat[1][2];
}

// compute back the texture matrix from fake shift scale rot
// the matrix returned must be understood as a qtexture_t with width=2 height=2 ( the default one )
void FakeTexCoordsToTexMat( float shift[2], float rot, float scale[2], vec_t texMat[2][3] )
{
	texMat[0][0] = scale[0] * cos( DEG2RAD( rot ) );
	texMat[1][0] = scale[0] * sin( DEG2RAD( rot ) );
	texMat[0][1] = -1.0f * scale[1] * sin( DEG2RAD( rot ) );
	texMat[1][1] = scale[1] * cos( DEG2RAD( rot ) );
	texMat[0][2] = -shift[0];
	texMat[1][2] = shift[1];
}

// convert a texture matrix between two qtexture_t
// if NULL for qtexture_t, basic 2x2 texture is assumed ( straight mapping between s/t coordinates and geometric coordinates )
void ConvertTexMatWithQTexture( brushprimit_texdef_t *texMat1, qtexture_t *qtex1, brushprimit_texdef_t *texMat2, qtexture_t *qtex2 )
{
	float s1,s2;
	s1 = ( qtex1 ? static_cast<float>( qtex1->width ) : 2.0f ) / ( qtex2 ? static_cast<float>( qtex2->width ) : 2.0f );
	s2 = ( qtex1 ? static_cast<float>( qtex1->height ) : 2.0f ) / ( qtex2 ? static_cast<float>( qtex2->height ) : 2.0f );
	texMat2->coords[0][0]=s1*texMat1->coords[0][0];
	texMat2->coords[0][1]=s1*texMat1->coords[0][1];
	texMat2->coords[0][2]=s1*texMat1->coords[0][2];
	texMat2->coords[1][0]=s2*texMat1->coords[1][0];
	texMat2->coords[1][1]=s2*texMat1->coords[1][1];
	texMat2->coords[1][2]=s2*texMat1->coords[1][2];
}

// texture locking
void Face_MoveTexture_BrushPrimit(face_t *f, vec3_t delta)
{
	vec3_t texS,texT;
	vec_t tx,ty;
	vec3_t M[3]; // columns of the matrix .. easier that way
	vec_t det;
	vec3_t D[2];
	// compute plane axis base ( doesn't change with translation )
	ComputeAxisBase( f->plane.normal, texS, texT );
	// compute translation vector in plane axis base
	tx = DotProduct( delta, texS );
	ty = DotProduct( delta, texT );
	// fill the data vectors
	M[0][0]=tx; M[0][1]=1.0f+tx; M[0][2]=tx;
	M[1][0]=ty; M[1][1]=ty; M[1][2]=1.0f+ty;
	M[2][0]=1.0f; M[2][1]=1.0f; M[2][2]=1.0f;
	D[0][0]=f->brushprimit_texdef.coords[0][2];
	D[0][1]=f->brushprimit_texdef.coords[0][0]+f->brushprimit_texdef.coords[0][2];
	D[0][2]=f->brushprimit_texdef.coords[0][1]+f->brushprimit_texdef.coords[0][2];
	D[1][0]=f->brushprimit_texdef.coords[1][2];
	D[1][1]=f->brushprimit_texdef.coords[1][0]+f->brushprimit_texdef.coords[1][2];
	D[1][2]=f->brushprimit_texdef.coords[1][1]+f->brushprimit_texdef.coords[1][2];
	// solve
	det = SarrusDet( M[0], M[1], M[2] );
	f->brushprimit_texdef.coords[0][0] = SarrusDet( D[0], M[1], M[2] ) / det;
	f->brushprimit_texdef.coords[0][1] = SarrusDet( M[0], D[0], M[2] ) / det;
	f->brushprimit_texdef.coords[0][2] = SarrusDet( M[0], M[1], D[0] ) / det;
	f->brushprimit_texdef.coords[1][0] = SarrusDet( D[1], M[1], M[2] ) / det;
	f->brushprimit_texdef.coords[1][1] = SarrusDet( M[0], D[1], M[2] ) / det;
	f->brushprimit_texdef.coords[1][2] = SarrusDet( M[0], M[1], D[1] ) / det;
}

// call Face_MoveTexture_BrushPrimit after vec3_t computation
void Select_ShiftTexture_BrushPrimit( face_t *f, int x, int y )
{
	vec3_t texS,texT;
	vec3_t delta;
	ComputeAxisBase( f->plane.normal, texS, texT );
	VectorScale( texS, static_cast<float>(x), texS );
	VectorScale( texT, static_cast<float>(y), texT );
	VectorCopy( texS, delta );
	VectorAdd( delta, texT, delta );
	Face_MoveTexture_BrushPrimit( f, delta );
}

// texture locking
// called before the points on the face are actually rotated
void RotateFaceTexture_BrushPrimit(face_t *f, int nAxis, float fDeg, vec3_t vOrigin )
{
	vec3_t texS,texT;			// axis base of the initial plane
	vec3_t vRotate;				// rotation vector
	vec3_t Orig;
	vec3_t rOrig,rvecS,rvecT;	// (0,0) (1,0) (0,1) ( initial plane axis base ) after rotation ( world axis base )
	vec3_t rNormal;				// normal of the plane after rotation
	vec3_t rtexS,rtexT;			// axis base of the rotated plane
	vec3_t lOrig,lvecS,lvecT;	// [2] are not used ( but usefull for debugging )
	vec3_t M[3];
	vec_t det;
	vec3_t D[2];
	// compute plane axis base
	ComputeAxisBase( f->plane.normal, texS, texT );
	// compute coordinates of (0,0) (1,0) (0,1) ( initial plane axis base ) after rotation
	// (0,0) (1,0) (0,1) ( initial plane axis base ) <-> (0,0,0) texS texT ( world axis base )
	// rotation vector
	VectorSet( vRotate, 0.0f, 0.0f, 0.0f );
	vRotate[nAxis]=fDeg;
	VectorSet( Orig, 0.0f, 0.0f, 0.0f );
	VectorRotate( Orig, vRotate, vOrigin, rOrig );
	VectorRotate( texS, vRotate, vOrigin, rvecS );
	VectorRotate( texT, vRotate, vOrigin, rvecT );
	// compute normal of plane after rotation
	VectorRotate( f->plane.normal, vRotate, rNormal );
	// compute rotated plane axis base
	ComputeAxisBase( rNormal, rtexS, rtexT );
	// compute S/T coordinates of the three points in rotated axis base ( in M matrix )
	lOrig[0] = DotProduct( rOrig, rtexS );
	lOrig[1] = DotProduct( rOrig, rtexT );
	lvecS[0] = DotProduct( rvecS, rtexS );
	lvecS[1] = DotProduct( rvecS, rtexT );
	lvecT[0] = DotProduct( rvecT, rtexS );
	lvecT[1] = DotProduct( rvecT, rtexT );
	M[0][0] = lOrig[0]; M[1][0] = lOrig[1]; M[2][0] = 1.0f;
	M[0][1] = lvecS[0]; M[1][1] = lvecS[1]; M[2][1] = 1.0f;
	M[0][2] = lvecT[0]; M[1][2] = lvecT[1]; M[2][2] = 1.0f;
	// fill data vector
	D[0][0]=f->brushprimit_texdef.coords[0][2];
	D[0][1]=f->brushprimit_texdef.coords[0][0]+f->brushprimit_texdef.coords[0][2];
	D[0][2]=f->brushprimit_texdef.coords[0][1]+f->brushprimit_texdef.coords[0][2];
	D[1][0]=f->brushprimit_texdef.coords[1][2];
	D[1][1]=f->brushprimit_texdef.coords[1][0]+f->brushprimit_texdef.coords[1][2];
	D[1][2]=f->brushprimit_texdef.coords[1][1]+f->brushprimit_texdef.coords[1][2];
	// solve
	det = SarrusDet( M[0], M[1], M[2] );
	f->brushprimit_texdef.coords[0][0] = SarrusDet( D[0], M[1], M[2] ) / det;
	f->brushprimit_texdef.coords[0][1] = SarrusDet( M[0], D[0], M[2] ) / det;
	f->brushprimit_texdef.coords[0][2] = SarrusDet( M[0], M[1], D[0] ) / det;
	f->brushprimit_texdef.coords[1][0] = SarrusDet( D[1], M[1], M[2] ) / det;
	f->brushprimit_texdef.coords[1][1] = SarrusDet( M[0], D[1], M[2] ) / det;
	f->brushprimit_texdef.coords[1][2] = SarrusDet( M[0], M[1], D[1] ) / det;
}

// best fitted 2D vector is x.X+y.Y
void ComputeBest2DVector( vec3_t v, vec3_t X, vec3_t Y, int &x, int &y )
{
	double sx,sy;
	sx = DotProduct( v, X );
	sy = DotProduct( v, Y );
	if ( fabs(sy) > fabs(sx) )
	{
		x = 0;
		if ( sy > 0.0 )
			y =  1;
		else
			y = -1;
	}
	else
	{
		y = 0;
		if ( sx > 0.0 )
			x =  1;
		else
			x = -1;
	}
}
