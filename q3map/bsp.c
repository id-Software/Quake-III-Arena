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
#include "qbsp.h"

#ifdef _WIN32
#ifdef _TTIMOBUILD
#include "pakstuff.h"
#else
#include "../libs/pakstuff.h"
#endif
extern HWND hwndOut;
#endif

char		source[1024];
char    tempsource[1024];
char		name[1024];

vec_t		microvolume = 1.0;
qboolean	glview;
qboolean	nodetail;
qboolean	fulldetail;
qboolean	onlyents;
qboolean	onlytextures;
qboolean	nowater;
qboolean	nofill;
qboolean	noopt;
qboolean	leaktest;
qboolean	verboseentities;
qboolean	noCurveBrushes;
qboolean	fakemap;
qboolean	notjunc;
qboolean	nomerge;
qboolean	nofog;
qboolean	nosubdivide;
qboolean	testExpand;
qboolean	showseams;

char		outbase[32];

int			entity_num;

/*
============
ProcessWorldModel

============
*/
void ProcessWorldModel( void ) {
	entity_t	*e;
	tree_t		*tree;
	bspface_t	*faces;
	qboolean	leaked;

	BeginModel();

	e = &entities[0];
	e->firstDrawSurf = 0;//numMapDrawSurfs;

	// check for patches with adjacent edges that need to LOD together
	PatchMapDrawSurfs( e );

	// build an initial bsp tree using all of the sides
	// of all of the structural brushes
	faces = MakeStructuralBspFaceList ( entities[0].brushes );
	tree = FaceBSP( faces );
	MakeTreePortals (tree);
	FilterStructuralBrushesIntoTree( e, tree );

	// see if the bsp is completely enclosed
	if ( FloodEntities (tree) ) {
		// rebuild a better bsp tree using only the
		// sides that are visible from the inside
		FillOutside (tree->headnode);

		// chop the sides to the convex hull of
		// their visible fragments, giving us the smallest
		// polygons 
		ClipSidesIntoTree( e, tree );

		faces = MakeVisibleBspFaceList( entities[0].brushes );
		FreeTree (tree);
		tree = FaceBSP( faces );
		MakeTreePortals( tree );
		FilterStructuralBrushesIntoTree( e, tree );
		leaked = qfalse;
	} else {
		_printf ("**********************\n");
		_printf ("******* leaked *******\n");
		_printf ("**********************\n");
		LeakFile (tree);
		if ( leaktest ) {
			_printf ("--- MAP LEAKED, ABORTING LEAKTEST ---\n");
			exit (0);
		}
		leaked = qtrue;

		// chop the sides to the convex hull of
		// their visible fragments, giving us the smallest
		// polygons 
		ClipSidesIntoTree( e, tree );
	}

	// save out information for visibility processing
	NumberClusters( tree );
	if ( !leaked ) {
		WritePortalFile( tree );
	}
	if ( glview ) {
		// dump the portals for debugging
		WriteGLView( tree, source );
	}
	FloodAreas (tree);

	// add references to the detail brushes
	FilterDetailBrushesIntoTree( e, tree );

	// create drawsurfs for triangle models
	AddTriangleModels( tree );

	// drawsurfs that cross fog boundaries will need to
	// be split along the bound
	if ( !nofog ) {
		FogDrawSurfs();		// may fragment drawsurfs
	}

	// subdivide each drawsurf as required by shader tesselation
	if ( !nosubdivide ) {
		SubdivideDrawSurfs( e, tree );
	}

	// merge together all common shaders on the same plane and remove 
	// all colinear points, so extra tjunctions won't be generated
	if ( !nomerge ) {
		MergeSides( e, tree );		// !@# testing
	}

	// add in any vertexes required to fix tjunctions
	if ( !notjunc ) {
		FixTJunctions( e );
	}

	// allocate lightmaps for faces and patches
	AllocateLightmaps( e );

	// add references to the final drawsurfs in the apropriate clusters
	FilterDrawsurfsIntoTree( e, tree );

	EndModel( tree->headnode );

	FreeTree (tree);
}

/*
============
ProcessSubModel

============
*/
void ProcessSubModel( void ) {
	entity_t	*e;
	tree_t		*tree;
	bspbrush_t	*b, *bc;
	node_t		*node;

	BeginModel ();

	e = &entities[entity_num];
	e->firstDrawSurf = numMapDrawSurfs;

	PatchMapDrawSurfs( e );

	// just put all the brushes in an empty leaf
	// FIXME: patches?
	node = AllocNode();
	node->planenum = PLANENUM_LEAF;
	for ( b = e->brushes ; b ; b = b->next ) {
		bc = CopyBrush( b );
		bc->next = node->brushlist;
		node->brushlist = bc;
	}

	tree = AllocTree();
	tree->headnode = node;

	ClipSidesIntoTree( e, tree );

	// subdivide each drawsurf as required by shader tesselation or fog
	if ( !nosubdivide ) {
		SubdivideDrawSurfs( e, tree );
	}

	// merge together all common shaders on the same plane and remove 
	// all colinear points, so extra tjunctions won't be generated
	if ( !nomerge ) {
		MergeSides( e, tree );		// !@# testing
	}

	// add in any vertexes required to fix tjunctions
	if ( !notjunc ) {
		FixTJunctions( e );
	}

	// allocate lightmaps for faces and patches
	AllocateLightmaps( e );

	// add references to the final drawsurfs in the apropriate clusters
	FilterDrawsurfsIntoTree( e, tree );

	EndModel ( node );

	FreeTree( tree );
}


/*
============
ProcessModels
============
*/
void ProcessModels (void)
{
	qboolean	oldVerbose;
	entity_t	*entity;

	oldVerbose = verbose;

	BeginBSPFile ();

	for ( entity_num=0 ; entity_num< num_entities ; entity_num++ ) {
		entity = &entities[entity_num];
	
		if ( !entity->brushes && !entity->patches ) {
			continue;
		}

		qprintf ("############### model %i ###############\n", nummodels);
		if (entity_num == 0)
			ProcessWorldModel ();
		else
			ProcessSubModel ();

		if (!verboseentities)
			verbose = qfalse;	// don't bother printing submodels
	}

	verbose = oldVerbose;
}

/*
============
Bspinfo
============
*/
void Bspinfo( int count, char **fileNames ) {
	int		i;
	char	source[1024];
	int			size;
	FILE		*f;

	if ( count < 1 ) {
		_printf( "No files to dump info for.\n");
		return;
	}

	for ( i = 0 ; i < count ; i++ ) {
		_printf ("---------------------\n");
		strcpy (source, fileNames[ i ] );
		DefaultExtension (source, ".bsp");
		f = fopen (source, "rb");
		if (f)
		{
			size = Q_filelength (f);
			fclose (f);
		}
		else
			size = 0;
		_printf ("%s: %i\n", source, size);
		
		LoadBSPFile (source);		
		PrintBSPFileSizes ();
		_printf ("---------------------\n");
	}
}


/*
============
OnlyEnts
============
*/
void OnlyEnts( void ) {
	char out[1024];

	sprintf (out, "%s.bsp", source);
	LoadBSPFile (out);
	num_entities = 0;

	LoadMapFile (name);
	SetModelNumbers ();
	SetLightStyles ();

	UnparseEntities ();

	WriteBSPFile (out);
}


/*
============
OnlyTextures
============
*/
void OnlyTextures( void ) {		// FIXME!!!
	char	out[1024];
	int		i;

	Error( "-onlytextures isn't working now..." );

	sprintf (out, "%s.bsp", source);

	LoadMapFile (name);

	LoadBSPFile (out);

	// replace all the drawsurface shader names
	for ( i = 0 ; i < numDrawSurfaces ; i++ ) {
	}

	WriteBSPFile (out);
}


/*
============
main
============
*/
int LightMain( int argc, char **argv );
int VLightMain (int argc, char **argv);
int VSoundMain (int argc, char **argv);
int VisMain( int argc, char **argv );

int main (int argc, char **argv) {
	int		i;
	double		start, end;
	char		path[1024];

	_printf ("Q3Map v1.0s (c) 1999 Id Software Inc.\n");
  
	if ( argc < 2 ) {
		Error ("usage: q3map [options] mapfile");
	}

	// check for general program options
	if (!strcmp(argv[1], "-info")) {
		Bspinfo( argc - 2, argv + 2 );
		return 0;
	}
	if (!strcmp(argv[1], "-light")) {
		LightMain( argc - 1, argv + 1 );
		return 0;
	}
	if (!strcmp(argv[1], "-vlight")) {
		VLightMain( argc - 1, argv + 1 );
		return 0;
	}
	if (!strcmp(argv[1], "-vsound")) {
		VSoundMain( argc - 1, argv + 1 );
		return 0;
	}
	if (!strcmp(argv[1], "-vis")) {
		VisMain( argc - 1, argv + 1 );
		return 0;
	}

	// do a bsp if nothing else was specified

	_printf ("---- q3map ----\n");

  tempsource[0] = '\0';

	for (i=1 ; i<argc ; i++)
	{
		if (!strcmp(argv[i],"-tempname"))
    {
      strcpy(tempsource, argv[++i]);
    }
		else if (!strcmp(argv[i],"-threads"))
		{
			numthreads = atoi (argv[i+1]);
			i++;
		}
		else if (!strcmp(argv[i],"-glview"))
		{
			glview = qtrue;
		}
		else if (!strcmp(argv[i], "-v"))
		{
			_printf ("verbose = true\n");
			verbose = qtrue;
		}
		else if (!strcmp(argv[i], "-draw"))
		{
			_printf ("drawflag = true\n");
			drawflag = qtrue;
		}
		else if (!strcmp(argv[i], "-nowater"))
		{
			_printf ("nowater = true\n");
			nowater = qtrue;
		}
		else if (!strcmp(argv[i], "-noopt"))
		{
			_printf ("noopt = true\n");
			noopt = qtrue;
		}
		else if (!strcmp(argv[i], "-nofill"))
		{
			_printf ("nofill = true\n");
			nofill = qtrue;
		}
		else if (!strcmp(argv[i], "-nodetail"))
		{
			_printf ("nodetail = true\n");
			nodetail = qtrue;
		}
		else if (!strcmp(argv[i], "-fulldetail"))
		{
			_printf ("fulldetail = true\n");
			fulldetail = qtrue;
		}
		else if (!strcmp(argv[i], "-onlyents"))
		{
			_printf ("onlyents = true\n");
			onlyents = qtrue;
		}
		else if (!strcmp(argv[i], "-onlytextures"))
		{
			_printf ("onlytextures = true\n");	// FIXME: make work again!
			onlytextures = qtrue;
		}
		else if (!strcmp(argv[i], "-micro"))
		{
			microvolume = atof(argv[i+1]);
			_printf ("microvolume = %f\n", microvolume);
			i++;
		}
		else if (!strcmp(argv[i], "-nofog"))
		{
			_printf ("nofog = true\n");
			nofog = qtrue;
		}
		else if (!strcmp(argv[i], "-nosubdivide"))
		{
			_printf ("nosubdivide = true\n");
			nosubdivide = qtrue;
		}
		else if (!strcmp(argv[i], "-leaktest"))
		{
			_printf ("leaktest = true\n");
			leaktest = qtrue;
		}
		else if (!strcmp(argv[i], "-verboseentities"))
		{
			_printf ("verboseentities = true\n");
			verboseentities = qtrue;
		}
		else if (!strcmp(argv[i], "-nocurves"))
		{
			noCurveBrushes = qtrue;
			_printf ("no curve brushes\n");
		}
		else if (!strcmp(argv[i], "-notjunc"))
		{
			notjunc = qtrue;
			_printf ("no tjunction fixing\n");
		}
		else if (!strcmp(argv[i], "-expand"))
		{
			testExpand = qtrue;
			_printf ("Writing expanded.map.\n");
		}
		else if (!strcmp(argv[i], "-showseams"))
		{
			showseams = qtrue;
			_printf ("Showing seams on terrain.\n");
		}
		else if (!strcmp (argv[i],"-tmpout"))
		{
			strcpy (outbase, "/tmp");
		}
		else if (!strcmp (argv[i],"-fakemap"))
		{
			fakemap = qtrue;
			_printf( "will generate fakemap.map\n");
		}
		else if (!strcmp(argv[i], "-samplesize"))
		{
			samplesize = atoi(argv[i+1]);
			if (samplesize < 1) samplesize = 1;
			i++;
			_printf("lightmap sample size is %dx%d units\n", samplesize, samplesize);
		}
		else if (argv[i][0] == '-')
			Error ("Unknown option \"%s\"", argv[i]);
		else
			break;
	}

	if (i != argc - 1)
		Error ("usage: q3map [options] mapfile");

	start = I_FloatTime ();

	ThreadSetDefault ();
	//numthreads = 1;		// multiple threads aren't helping because of heavy malloc use
	SetQdirFromPath (argv[i]);

#ifdef _WIN32
  InitPakFile(gamedir, NULL);
#endif

	strcpy (source, ExpandArg (argv[i]));
	StripExtension (source);

	// delete portal and line files
	sprintf (path, "%s.prt", source);
	remove (path);
	sprintf (path, "%s.lin", source);
	remove (path);

	strcpy (name, ExpandArg (argv[i]));	
	if ( strcmp(name + strlen(name) - 4, ".reg" ) ) {
		// if we are doing a full map, delete the last saved region map
		sprintf (path, "%s.reg", source);
		remove (path);

		DefaultExtension (name, ".map");	// might be .reg
	}

	//
	// if onlyents, just grab the entites and resave
	//
	if ( onlyents ) {
		OnlyEnts();
		return 0;
	}

	//
	// if onlytextures, just grab the textures and resave
	//
	if ( onlytextures ) {
		OnlyTextures();
		return 0;
	}

	//
	// start from scratch
	//
	LoadShaderInfo();

  // load original file from temp spot in case it was renamed by the editor on the way in
  if (strlen(tempsource) > 0) {
	  LoadMapFile (tempsource);
  } else {
	  LoadMapFile (name);
  }

	SetModelNumbers ();
	SetLightStyles ();

	ProcessModels ();

	EndBSPFile();

	end = I_FloatTime ();
	_printf ("%5.0f seconds elapsed\n", end-start);

  // remove temp name if appropriate
  if (strlen(tempsource) > 0) {
    remove(tempsource);
  }

	return 0;
}

