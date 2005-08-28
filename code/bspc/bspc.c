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

#if defined(WIN32) || defined(_WIN32)
#include <direct.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "qbsp.h"
#include "l_mem.h"
#include "../botlib/aasfile.h"
#include "../botlib/be_aas_cluster.h"
#include "../botlib/be_aas_optimize.h"
#include "aas_create.h"
#include "aas_store.h"
#include "aas_file.h"
#include "aas_cfg.h"
#include "be_aas_bspc.h"

extern	int use_nodequeue;		//brushbsp.c
extern	int calcgrapplereach;	//be_aas_reach.c

float			subdivide_size = 240;
char			source[1024];
char			name[1024];
vec_t			microvolume = 1.0;
char			outbase[32];
int				entity_num;
aas_settings_t	aassettings;

qboolean	noprune;			//don't prune nodes (bspc.c)
qboolean	glview;				//create a gl view
qboolean	nodetail;			//don't use detail brushes (map.c)
qboolean	fulldetail;			//use but don't mark detail brushes (map.c)
qboolean	onlyents;			//only process the entities (bspc.c)
qboolean	nomerge;			//don't merge bsp node faces (faces.c)
qboolean	nowater;			//don't use the water brushes (map.c)
qboolean	nocsg;				//don't carve intersecting brushes (bspc.c)
qboolean	noweld;				//use unique face vertexes (faces.c)
qboolean	noshare;			//don't share bsp edges (faces.c)
qboolean	nosubdiv;			//don't subdivide bsp node faces (faces.c)
qboolean	notjunc;			//don't create tjunctions (edge melting) (faces.c)
qboolean	optimize;			//enable optimisation
qboolean	leaktest;			//perform a leak test
qboolean	verboseentities;
qboolean	freetree;			//free the bsp tree when not needed anymore
qboolean	create_aas;			//create an .AAS file
qboolean	nobrushmerge;		//don't merge brushes
qboolean	lessbrushes;		//create less brushes instead of correct texture placement
qboolean	cancelconversion;	//true if the conversion is being cancelled
qboolean	noliquids;			//no liquids when writing map file
qboolean	forcesidesvisible;	//force all brush sides to be visible when loaded from bsp
qboolean	capsule_collision = 0;

/*
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ProcessWorldModel (void)
{
	entity_t	*e;
	tree_t *tree;
	qboolean	leaked;
	int brush_start, brush_end;

	e = &entities[entity_num];

	brush_start = e->firstbrush;
	brush_end = brush_start + e->numbrushes;
	leaked = false;

	//process the whole world in one time
	tree = ProcessWorldBrushes(brush_start, brush_end);
	//create the bsp tree portals
	MakeTreePortals(tree);
	//mark all leafs that can be reached by entities
	if (FloodEntities(tree))
	{
		FillOutside(tree->headnode);
	} //end if
	else
	{
		Log_Print("**** leaked ****\n");
		leaked = true;
		LeakFile(tree);
		if (leaktest)
		{
			Log_Print("--- MAP LEAKED ---\n");
			exit(0);
		} //end if
	} //end else

	MarkVisibleSides (tree, brush_start, brush_end);

	FloodAreas (tree);

#ifndef ME
	if (glview) WriteGLView(tree, source);
#endif
	MakeFaces(tree->headnode);
	FixTjuncs(tree->headnode);

	//NOTE: Never prune the nodes because the portals
	//		are screwed when prunning is done and as
	//		a result portal writing will crash
	//if (!noprune) PruneNodes(tree->headnode);

	WriteBSP(tree->headnode);

	if (!leaked) WritePortalFile(tree);

	Tree_Free(tree);
} //end of the function ProcessWorldModel
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ProcessSubModel (void)
{
	entity_t	*e;
	int start, end;
	tree_t *tree;
	bspbrush_t *list;
	vec3_t mins, maxs;

	e = &entities[entity_num];

	start = e->firstbrush;
	end = start + e->numbrushes;

	mins[0] = mins[1] = mins[2] = -4096;
	maxs[0] = maxs[1] = maxs[2] = 4096;
	list = MakeBspBrushList(start, end, mins, maxs);
	if (!nocsg) list = ChopBrushes (list);
	tree = BrushBSP (list, mins, maxs);
	MakeTreePortals (tree);
	MarkVisibleSides (tree, start, end);
	MakeFaces (tree->headnode);
	FixTjuncs (tree->headnode);
	WriteBSP (tree->headnode);
	Tree_Free(tree);
} //end of the function ProcessSubModel
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ProcessModels (void)
{
	BeginBSPFile();

	for (entity_num = 0; entity_num < num_entities; entity_num++)
	{
		if (!entities[entity_num].numbrushes)
			continue;

		Log_Print("############### model %i ###############\n", nummodels);
		BeginModel();
		if (entity_num == 0) ProcessWorldModel();
		else ProcessSubModel();
		EndModel();

		if (!verboseentities)
			verbose = false;	// don't bother printing submodels
	} //end for
	EndBSPFile();
} //end of the function ProcessModels
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Win_Map2Bsp(char *bspfilename)
{
	double start, end;
	char path[1024];

	start = I_FloatTime();

	ThreadSetDefault();
	//yeah sure Carmack
	//numthreads = 1;		// multiple threads aren't helping...

	strcpy(source, ExpandArg(bspfilename));
	StripExtension(source);

	//delete portal and line files
	sprintf(path, "%s.prt", source);
	remove(path);
	sprintf(path, "%s.lin", source);
	remove(path);

	strcpy(name, ExpandArg(bspfilename));	
	DefaultExtension(name, ".map");	// might be .reg

	Q2_AllocMaxBSP();
	//
	SetModelNumbers();
	SetLightStyles();
	ProcessModels();
	//write the BSP
	Q2_WriteBSPFile(bspfilename);

	Q2_FreeMaxBSP();

	end = I_FloatTime();
	Log_Print("%5.0f seconds elapsed\n", end-start);
} //end of the function Win_Map2Bsp
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Map2Bsp(char *mapfilename, char *outputfilename)
{
	double start, end;
	char path[1024];

	start = I_FloatTime ();

	ThreadSetDefault ();
	//yeah sure Carmack
	//numthreads = 1;		//multiple threads aren't helping...
	//SetQdirFromPath(bspfilename);

	strcpy(source, ExpandArg(mapfilename));
	StripExtension(source);

	// delete portal and line files
	sprintf(path, "%s.prt", source);
	remove(path);
	sprintf(path, "%s.lin", source);
	remove(path);

	strcpy(name, ExpandArg(mapfilename));
	DefaultExtension(name, ".map");	// might be .reg

	//
	// if onlyents, just grab the entites and resave
	//
	if (onlyents)
	{
		char out[1024];

		Q2_AllocMaxBSP();
		sprintf (out, "%s.bsp", source);
		Q2_LoadBSPFile(out, 0, 0);
		num_entities = 0;

		Q2_LoadMapFile(name);
		SetModelNumbers();
		SetLightStyles();

		Q2_UnparseEntities();

		Q2_WriteBSPFile(out);
		//
		Q2_FreeMaxBSP();
	} //end if
	else
	{
		//
		// start from scratch
		//
		Q2_AllocMaxBSP();
		//load the map
		Q2_LoadMapFile(name);
		//create the .bsp file
		SetModelNumbers();
		SetLightStyles();
		ProcessModels();
		//write the BSP
		Q2_WriteBSPFile(outputfilename);
		//
		Q2_FreeMaxBSP();
	} //end else

	end = I_FloatTime();
	Log_Print("%5.0f seconds elapsed\n", end-start);
} //end of the function Map2Bsp
*/
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AASOuputFile(quakefile_t *qf, char *outputpath, char *filename)
{
	char ext[MAX_PATH];

	//
	if (strlen(outputpath))
	{
		strcpy(filename, outputpath);
		//append the bsp file base
		AppendPathSeperator(filename, MAX_PATH);
		ExtractFileBase(qf->origname, &filename[strlen(filename)]);
		//append .aas
		strcat(filename, ".aas");
		return;
	} //end if
	//
	ExtractFileExtension(qf->filename, ext);
	if (!stricmp(ext, "pk3") || !stricmp(ext, "pak") || !stricmp(ext, "sin"))
	{
		strcpy(filename, qf->filename);
		while(strlen(filename) &&
				filename[strlen(filename)-1] != '\\' &&
				filename[strlen(filename)-1] != '/')
		{
			filename[strlen(filename)-1] = '\0';
		} //end while
		strcat(filename, "maps");
		if (access(filename, 0x04)) CreatePath(filename);
		//append the bsp file base
		AppendPathSeperator(filename, MAX_PATH);
		ExtractFileBase(qf->origname, &filename[strlen(filename)]);
		//append .aas
		strcat(filename, ".aas");
	} //end if
	else
	{
		strcpy(filename, qf->filename);
		while(strlen(filename) &&
				filename[strlen(filename)-1] != '.')
		{
			filename[strlen(filename)-1] = '\0';
		} //end while
		strcat(filename, "aas");
	} //end else
} //end of the function AASOutputFile
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void CreateAASFilesForAllBSPFiles(char *quakepath)
{
#if defined(WIN32)|defined(_WIN32)
	WIN32_FIND_DATA filedata;
	HWND handle;
	struct _stat statbuf;
#else
	glob_t globbuf;
	struct stat statbuf;
	int j;
#endif
	int done;
	char filter[_MAX_PATH], bspfilter[_MAX_PATH], aasfilter[_MAX_PATH];
	char aasfile[_MAX_PATH], buf[_MAX_PATH], foldername[_MAX_PATH];
	quakefile_t *qf, *qf2, *files, *bspfiles, *aasfiles;

	strcpy(filter, quakepath);
	AppendPathSeperator(filter, sizeof(filter));
	strcat(filter, "*");

#if defined(WIN32)|defined(_WIN32)
	handle = FindFirstFile(filter, &filedata);
	done = (handle == INVALID_HANDLE_VALUE);
	while(!done)
	{
		_splitpath(filter, foldername, NULL, NULL, NULL);
		_splitpath(filter, NULL, &foldername[strlen(foldername)], NULL, NULL);
		AppendPathSeperator(foldername, _MAX_PATH);
		strcat(foldername, filedata.cFileName);
		_stat(foldername, &statbuf);
#else
	glob(filter, 0, NULL, &globbuf);
	for (j = 0; j < globbuf.gl_pathc; j++)
	{
		strcpy(foldername, globbuf.gl_pathv[j]);
		stat(foldername, &statbuf);
#endif
		//if it is a folder
		if (statbuf.st_mode & S_IFDIR)
		{
			//
			AppendPathSeperator(foldername, sizeof(foldername));
			//get all the bsp files
			strcpy(bspfilter, foldername);
			strcat(bspfilter, "maps/*.bsp");
			files = FindQuakeFiles(bspfilter);
			strcpy(bspfilter, foldername);
			strcat(bspfilter, "*.pk3/maps/*.bsp");
			bspfiles = FindQuakeFiles(bspfilter);
			for (qf = bspfiles; qf; qf = qf->next) if (!qf->next) break;
			if (qf) qf->next = files;
			else bspfiles = files;
			//get all the aas files
			strcpy(aasfilter, foldername);
			strcat(aasfilter, "maps/*.aas");
			files = FindQuakeFiles(aasfilter);
			strcpy(aasfilter, foldername);
			strcat(aasfilter, "*.pk3/maps/*.aas");
			aasfiles = FindQuakeFiles(aasfilter);
			for (qf = aasfiles; qf; qf = qf->next) if (!qf->next) break;
			if (qf) qf->next = files;
			else aasfiles = files;
			//
			for (qf = bspfiles; qf; qf = qf->next)
			{
				sprintf(aasfile, "%s/%s", qf->pakfile, qf->origname);
				Log_Print("found %s\n", aasfile);
				strcpy(&aasfile[strlen(aasfile)-strlen(".bsp")], ".aas");
				for (qf2 = aasfiles; qf2; qf2 = qf2->next)
				{
					sprintf(buf, "%s/%s", qf2->pakfile, qf2->origname);
					if (!stricmp(aasfile, buf))
					{
						Log_Print("found %s\n", buf);
						break;
					} //end if
				} //end for
			} //end for
		} //end if
#if defined(WIN32)|defined(_WIN32)
		//find the next file
		done = !FindNextFile(handle, &filedata);
	} //end while
#else
	} //end for
	globfree(&globbuf);
#endif
} //end of the function CreateAASFilesForAllBSPFiles
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
quakefile_t *GetArgumentFiles(int argc, char *argv[], int *i, char *ext)
{
	quakefile_t *qfiles, *lastqf, *qf;
	int j;
	char buf[1024];

	qfiles = NULL;
	lastqf = NULL;
	for (; (*i)+1 < argc && argv[(*i)+1][0] != '-'; (*i)++)
	{
		strcpy(buf, argv[(*i)+1]);
		for (j = strlen(buf)-1; j >= strlen(buf)-4; j--)
			if (buf[j] == '.') break;
		if (j >= strlen(buf)-4)
			strcpy(&buf[j+1], ext);
		qf = FindQuakeFiles(buf);
		if (!qf) continue;
		if (lastqf) lastqf->next = qf;
		else qfiles = qf;
		lastqf = qf;
		while(lastqf->next) lastqf = lastqf->next;
	} //end for
	return qfiles;
} //end of the function GetArgumentFiles
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================

#define COMP_BSP2MAP		1
#define COMP_BSP2AAS		2
#define COMP_REACH			3
#define COMP_CLUSTER		4
#define COMP_AASOPTIMIZE	5
#define COMP_AASINFO		6

int main (int argc, char **argv)
{
	int i, comp = 0;
	char outputpath[MAX_PATH] = "";
	char filename[MAX_PATH] = "unknown";
	quakefile_t *qfiles, *qf;
	double start_time;

	myargc = argc;
	myargv = argv;

	start_time = I_FloatTime();

	Log_Open("bspc.log");		//open a log file
	Log_Print("BSPC version "BSPC_VERSION", %s %s\n", __DATE__, __TIME__);

	DefaultCfg();
	for (i = 1; i < argc; i++)
	{
		if (!stricmp(argv[i],"-threads"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			numthreads = atoi(argv[++i]);
			Log_Print("threads = %d\n", numthreads);
		} //end if
		else if (!stricmp(argv[i], "-noverbose"))
		{
			Log_Print("verbose = false\n");
			verbose = false;
		} //end else if
		else if (!stricmp(argv[i], "-nocsg"))
		{
			Log_Print("nocsg = true\n");
			nocsg = true;
		} //end else if
		else if (!stricmp(argv[i], "-optimize"))
		{
			Log_Print("optimize = true\n");
			optimize = true;
		} //end else if
		/*
		else if (!stricmp(argv[i],"-glview"))
		{
			glview = true;
		} //end else if
		else if (!stricmp(argv[i], "-draw"))
		{
			Log_Print("drawflag = true\n");
			drawflag = true;
		} //end else if
		else if (!stricmp(argv[i], "-noweld"))
		{
			Log_Print("noweld = true\n");
			noweld = true;
		} //end else if
		else if (!stricmp(argv[i], "-noshare"))
		{
			Log_Print("noshare = true\n");
			noshare = true;
		} //end else if
		else if (!stricmp(argv[i], "-notjunc"))
		{
			Log_Print("notjunc = true\n");
			notjunc = true;
		} //end else if
		else if (!stricmp(argv[i], "-nowater"))
		{
			Log_Print("nowater = true\n");
			nowater = true;
		} //end else if
		else if (!stricmp(argv[i], "-noprune"))
		{
			Log_Print("noprune = true\n");
			noprune = true;
		} //end else if
		else if (!stricmp(argv[i], "-nomerge"))
		{
			Log_Print("nomerge = true\n");
			nomerge = true;
		} //end else if
		else if (!stricmp(argv[i], "-nosubdiv"))
		{
			Log_Print("nosubdiv = true\n");
			nosubdiv = true;
		} //end else if
		else if (!stricmp(argv[i], "-nodetail"))
		{
			Log_Print("nodetail = true\n");
			nodetail = true;
		} //end else if
		else if (!stricmp(argv[i], "-fulldetail"))
		{
			Log_Print("fulldetail = true\n");
			fulldetail = true;
		} //end else if
		else if (!stricmp(argv[i], "-onlyents"))
		{
			Log_Print("onlyents = true\n");
			onlyents = true;
		} //end else if
		else if (!stricmp(argv[i], "-micro"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			microvolume = atof(argv[++i]);
			Log_Print("microvolume = %f\n", microvolume);
		} //end else if
		else if (!stricmp(argv[i], "-leaktest"))
		{
			Log_Print("leaktest = true\n");
			leaktest = true;
		} //end else if
		else if (!stricmp(argv[i], "-verboseentities"))
		{
			Log_Print("verboseentities = true\n");
			verboseentities = true;
		} //end else if
		else if (!stricmp(argv[i], "-chop"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			subdivide_size = atof(argv[++i]);
			Log_Print("subdivide_size = %f\n", subdivide_size);
		} //end else if
		else if (!stricmp (argv[i], "-tmpout"))
		{
			strcpy (outbase, "/tmp");
			Log_Print("temp output\n");
		} //end else if
		*/
#ifdef ME
		else if (!stricmp(argv[i], "-freetree"))
		{
			freetree = true;
			Log_Print("freetree = true\n");
		} //end else if
		else if (!stricmp(argv[i], "-grapplereach"))
		{
			calcgrapplereach = true;
			Log_Print("grapplereach = true\n");
		} //end else if
		else if (!stricmp(argv[i], "-nobrushmerge"))
		{
			nobrushmerge = true;
			Log_Print("nobrushmerge = true\n");
		} //end else if
		else if (!stricmp(argv[i], "-noliquids"))
		{
			noliquids = true;
			Log_Print("noliquids = true\n");
		} //end else if
		else if (!stricmp(argv[i], "-forcesidesvisible"))
		{
			forcesidesvisible = true;
			Log_Print("forcesidesvisible = true\n");
		} //end else if
		else if (!stricmp(argv[i], "-output"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			if (access(argv[i+1], 0x04)) Warning("the folder %s does not exist", argv[i+1]);
			strcpy(outputpath, argv[++i]);
		} //end else if
		else if (!stricmp(argv[i], "-breadthfirst"))
		{
			use_nodequeue = true;
			Log_Print("breadthfirst = true\n");
		} //end else if
		else if (!stricmp(argv[i], "-capsule"))
		{
			capsule_collision = true;
			Log_Print("capsule_collision = true\n");
		} //end else if
		else if (!stricmp(argv[i], "-cfg"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			if (!LoadCfgFile(argv[++i]))
				exit(0);
		} //end else if
		else if (!stricmp(argv[i], "-bsp2map"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			comp = COMP_BSP2MAP;
			qfiles = GetArgumentFiles(argc, argv, &i, "bsp");
		} //end else if
		else if (!stricmp(argv[i], "-bsp2aas"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			comp = COMP_BSP2AAS;
			qfiles = GetArgumentFiles(argc, argv, &i, "bsp");
		} //end else if
		else if (!stricmp(argv[i], "-aasall"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			CreateAASFilesForAllBSPFiles(argv[++i]);
		} //end else if
		else if (!stricmp(argv[i], "-reach"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			comp = COMP_REACH;
			qfiles = GetArgumentFiles(argc, argv, &i, "bsp");
		} //end else if
		else if (!stricmp(argv[i], "-cluster"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			comp = COMP_CLUSTER;
			qfiles = GetArgumentFiles(argc, argv, &i, "bsp");
		} //end else if
		else if (!stricmp(argv[i], "-aasinfo"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			comp = COMP_AASINFO;
			qfiles = GetArgumentFiles(argc, argv, &i, "aas");
		} //end else if
		else if (!stricmp(argv[i], "-aasopt"))
		{
			if (i + 1 >= argc) {i = 0; break;}
			comp = COMP_AASOPTIMIZE;
			qfiles = GetArgumentFiles(argc, argv, &i, "aas");
		} //end else if
#endif //ME
		else
		{
			Log_Print("unknown parameter %s\n", argv[i]);
			break;
		} //end else
	} //end for

	//if there are parameters and there's no mismatch in one of the parameters
	if (argc > 1 && i == argc)
	{
		switch(comp)
		{
			case COMP_BSP2MAP:
			{
				if (!qfiles) Log_Print("no files found\n");
				for (qf = qfiles; qf; qf = qf->next)
				{
					//copy the output path
					strcpy(filename, outputpath);
					//append the bsp file base
					AppendPathSeperator(filename, MAX_PATH);
					ExtractFileBase(qf->origname, &filename[strlen(filename)]);
					//append .map
					strcat(filename, ".map");
					//
					Log_Print("bsp2map: %s to %s\n", qf->origname, filename);
					if (qf->type != QFILETYPE_BSP) Warning("%s is probably not a BSP file\n", qf->origname);
					//
					LoadMapFromBSP(qf);
					//write the map file
					WriteMapFile(filename);
				} //end for
				break;
			} //end case
			case COMP_BSP2AAS:
			{
				if (!qfiles) Log_Print("no files found\n");
				for (qf = qfiles; qf; qf = qf->next)
				{
					AASOuputFile(qf, outputpath, filename);
					//
					Log_Print("bsp2aas: %s to %s\n", qf->origname, filename);
					if (qf->type != QFILETYPE_BSP) Warning("%s is probably not a BSP file\n", qf->origname);
					//set before map loading
					create_aas = 1;
					LoadMapFromBSP(qf);
					//create the AAS file
					AAS_Create(filename);
					//if it's a Quake3 map calculate the reachabilities and clusters
					if (loadedmaptype == MAPTYPE_QUAKE3) AAS_CalcReachAndClusters(qf);
					//
					if (optimize) AAS_Optimize();
					//
					//write out the stored AAS file
					if (!AAS_WriteAASFile(filename))
					{
						Error("error writing %s\n", filename);
					} //end if
					//deallocate memory
					AAS_FreeMaxAAS();
				} //end for
				break;
			} //end case
			case COMP_REACH:
			{
				if (!qfiles) Log_Print("no files found\n");
				for (qf = qfiles; qf; qf = qf->next)
				{
					AASOuputFile(qf, outputpath, filename);
					//
					Log_Print("reach: %s to %s\n", qf->origname, filename);
					if (qf->type != QFILETYPE_BSP) Warning("%s is probably not a BSP file\n", qf->origname);
					//if the AAS file exists in the output directory
					if (!access(filename, 0x04))
					{
						if (!AAS_LoadAASFile(filename, 0, 0))
						{
							Error("error loading aas file %s\n", filename);
						} //end if
						//assume it's a Quake3 BSP file
						loadedmaptype = MAPTYPE_QUAKE3;
					} //end if
					else
					{
						Warning("AAS file %s not found in output folder\n", filename);
						Log_Print("creating %s...\n", filename);
						//set before map loading
						create_aas = 1;
						LoadMapFromBSP(qf);
						//create the AAS file
						AAS_Create(filename);
					} //end else
					//if it's a Quake3 map calculate the reachabilities and clusters
					if (loadedmaptype == MAPTYPE_QUAKE3)
					{
						AAS_CalcReachAndClusters(qf);
					} //end if
					//
					if (optimize) AAS_Optimize();
					//write out the stored AAS file
					if (!AAS_WriteAASFile(filename))
					{
						Error("error writing %s\n", filename);
					} //end if
					//deallocate memory
					AAS_FreeMaxAAS();
				} //end for
				break;
			} //end case
			case COMP_CLUSTER:
			{
				if (!qfiles) Log_Print("no files found\n");
				for (qf = qfiles; qf; qf = qf->next)
				{
					AASOuputFile(qf, outputpath, filename);
					//
					Log_Print("cluster: %s to %s\n", qf->origname, filename);
					if (qf->type != QFILETYPE_BSP) Warning("%s is probably not a BSP file\n", qf->origname);
					//if the AAS file exists in the output directory
					if (!access(filename, 0x04))
					{
						if (!AAS_LoadAASFile(filename, 0, 0))
						{
							Error("error loading aas file %s\n", filename);
						} //end if
						//assume it's a Quake3 BSP file
						loadedmaptype = MAPTYPE_QUAKE3;
						//if it's a Quake3 map calculate the clusters
						if (loadedmaptype == MAPTYPE_QUAKE3)
						{
							aasworld.numclusters = 0;
							AAS_InitBotImport();
							AAS_InitClustering();
						} //end if
					} //end if
					else
					{
						Warning("AAS file %s not found in output folder\n", filename);
						Log_Print("creating %s...\n", filename);
						//set before map loading
						create_aas = 1;
						LoadMapFromBSP(qf);
						//create the AAS file
						AAS_Create(filename);
						//if it's a Quake3 map calculate the reachabilities and clusters
						if (loadedmaptype == MAPTYPE_QUAKE3) AAS_CalcReachAndClusters(qf);
					} //end else
					//
					if (optimize) AAS_Optimize();
					//write out the stored AAS file
					if (!AAS_WriteAASFile(filename))
					{
						Error("error writing %s\n", filename);
					} //end if
					//deallocate memory
					AAS_FreeMaxAAS();
				} //end for
				break;
			} //end case
			case COMP_AASOPTIMIZE:
			{
				if (!qfiles) Log_Print("no files found\n");
				for (qf = qfiles; qf; qf = qf->next)
				{
					AASOuputFile(qf, outputpath, filename);
					//
					Log_Print("optimizing: %s to %s\n", qf->origname, filename);
					if (qf->type != QFILETYPE_AAS) Warning("%s is probably not a AAS file\n", qf->origname);
					//
					AAS_InitBotImport();
					//
					if (!AAS_LoadAASFile(qf->filename, qf->offset, qf->length))
					{
						Error("error loading aas file %s\n", qf->filename);
					} //end if
					AAS_Optimize();
					//write out the stored AAS file
					if (!AAS_WriteAASFile(filename))
					{
						Error("error writing %s\n", filename);
					} //end if
					//deallocate memory
					AAS_FreeMaxAAS();
				} //end for
				break;
			} //end case
			case COMP_AASINFO:
			{
				if (!qfiles) Log_Print("no files found\n");
				for (qf = qfiles; qf; qf = qf->next)
				{
					AASOuputFile(qf, outputpath, filename);
					//
					Log_Print("aas info for: %s\n", filename);
					if (qf->type != QFILETYPE_AAS) Warning("%s is probably not a AAS file\n", qf->origname);
					//
					AAS_InitBotImport();
					//
					if (!AAS_LoadAASFile(qf->filename, qf->offset, qf->length))
					{
						Error("error loading aas file %s\n", qf->filename);
					} //end if
					AAS_ShowTotals();
				} //end for
			} //end case
			default:
			{
				Log_Print("don't know what to do\n");
				break;
			} //end default
		} //end switch
	} //end if
	else
	{
		Log_Print("Usage:   bspc [-<switch> [-<switch> ...]]\n"
#if defined(WIN32) || defined(_WIN32)
			"Example 1: bspc -bsp2aas d:\\quake3\\baseq3\\maps\\mymap?.bsp\n"
			"Example 2: bspc -bsp2aas d:\\quake3\\baseq3\\pak0.pk3\\maps/q3dm*.bsp\n"
#else
			"Example 1: bspc -bsp2aas /quake3/baseq3/maps/mymap?.bsp\n"
			"Example 2: bspc -bsp2aas /quake3/baseq3/pak0.pk3/maps/q3dm*.bsp\n"
#endif
			"\n"
			"Switches:\n"
			//"   bsp2map  <[pakfilter/]filter.bsp>    = convert BSP to MAP\n"
			//"   aasall   <quake3folder>              = create AAS files for all BSPs\n"
			"   bsp2aas  <[pakfilter/]filter.bsp>    = convert BSP to AAS\n"
			"   reach    <filter.bsp>                = compute reachability & clusters\n"
			"   cluster  <filter.aas>                = compute clusters\n"
			"   aasopt   <filter.aas>                = optimize aas file\n"
			"   aasinfo  <filter.aas>                = show AAS file info\n"
			"   output   <output path>               = set output path\n"
			"   threads  <X>                         = set number of threads to X\n"
			"   cfg      <filename>                  = use this cfg file\n"
			"   optimize                             = enable optimization\n"
			"   noverbose                            = disable verbose output\n"
			"   breadthfirst                         = breadth first bsp building\n"
			"   nobrushmerge                         = don't merge brushes\n"
			"   noliquids                            = don't write liquids to map\n"
			"   freetree                             = free the bsp tree\n"
			"   nocsg                                = disables brush chopping\n"
			"   forcesidesvisible                    = force all sides to be visible\n"
			"   grapplereach                         = calculate grapple reachabilities\n"

/*			"   glview     = output a GL view\n"
			"   draw       = enables drawing\n"
			"   noweld     = disables weld\n"
			"   noshare    = disables sharing\n"
			"   notjunc    = disables juncs\n"
			"   nowater    = disables water brushes\n"
			"   noprune    = disables node prunes\n"
			"   nomerge    = disables face merging\n"
			"   nosubdiv   = disables subdeviding\n"
			"   nodetail   = disables detail brushes\n"
			"   fulldetail = enables full detail\n"
			"   onlyents   = only compile entities with bsp\n"
			"   micro <volume>\n"
			"              = sets the micro volume to the given float\n"
			"   leaktest   = perform a leak test\n"
			"   verboseentities\n"
			"              = enable entity verbose mode\n"
			"   chop <subdivide_size>\n"
			"              = sets the subdivide size to the given float\n"*/
			"\n");
	} //end else
	Log_Print("BSPC run time is %5.0f seconds\n", I_FloatTime() - start_time);
	Log_Close();						//close the log file
	return 0;
} //end of the function main

