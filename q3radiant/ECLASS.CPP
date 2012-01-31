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
#include "io.h"
#include "pakstuff.h"
//#include "qertypes.h"

eclass_t	*eclass = NULL;
eclass_t	*eclass_bad = NULL;
char		eclass_directory[1024];

// md3 cache for misc_models
eclass_t *g_md3Cache = NULL;

/*

the classname, color triple, and bounding box are parsed out of comments
A ? size means take the exact brush size.

/*QUAKED <classname> (0 0 0) ?
/*QUAKED <classname> (0 0 0) (-8 -8 -8) (8 8 8)

Flag names can follow the size description:

/*QUAKED func_door (0 .5 .8) ? START_OPEN STONE_SOUND DOOR_DONT_LINK GOLD_KEY SILVER_KEY

*/

void CleanEntityList(eclass_t *&pList)
{
  while (pList)
  {
    eclass_t* pTemp = pList->next;

    entitymodel *model = pList->model;
    while (model != NULL)
    {
      delete []model->pTriList;
      model = model->pNext;
    }
    
    if (pList->modelpath)
      free(pList->modelpath);
	  if (pList->skinpath)			// PGM
		  free(pList->skinpath);		// PGM
    
    free(pList->name);
    free(pList->comments);
    free(pList);
    pList = pTemp;
  }

  pList = NULL;

}


void CleanUpEntities()
{
  CleanEntityList(eclass);
  CleanEntityList(g_md3Cache);
/*
  while (eclass)
  {
    eclass_t* pTemp = eclass->next;
    delete []eclass->pTriList;
    
    if (eclass->modelpath)
      free(eclass->modelpath);
	  if (eclass->skinpath)			// PGM
		  free(eclass->skinpath);		// PGM
    
    free(eclass->name);
    free(eclass->comments);
    free(eclass);
    eclass = pTemp;
  }

  eclass = NULL;
*/
  if (eclass_bad)
  {
    free(eclass_bad->name);
    free(eclass_bad->comments);
    free(eclass_bad);
    eclass_bad = NULL;
  }
}

void ExtendBounds(vec3_t v, vec3_t &vMin, vec3_t &vMax)
{
	for (int i = 0 ;i < 3 ;i++)
	{
		vec_t f = v[i];
		
    if (f < vMin[i])
    {
			vMin[i] = f;
    }

    if (f > vMax[i])
    {
			vMax[i] = f;
    }
	}
}



// FIXME: this code is a TOTAL clusterfuck
//
void LoadModel(const char *pLocation, eclass_t *e, vec3_t &vMin, vec3_t &vMax, entitymodel *&pModel, const char *pSkin)
{
  // this assumes a path only and uses tris.md2
  // for the model and skin.pcx for the skin
  char cPath[1024];
  char cSkin[1024];
  char cFullLocation[1024];
	//struct _finddata_t fileinfo;

  vMin[0] = vMin[1] = vMin[2] = 99999;
  vMax[0] = vMax[1] = vMax[2] = -99999;

  bool bMD3 = false;
  bool bASE = false;

  strcpy( cFullLocation, pLocation );

  if (strstr(pLocation, ".md3"))
  {
    bMD3 = true;
  }
  else if (strstr(pLocation, ".md2") != NULL)
  {
	  sprintf( cFullLocation, "%stris.md2", pLocation);
  }
  else if (strstr(pLocation, ".ase") != NULL)
  {
    bASE = true;
  }

  sprintf( cPath, "%s/%s", ValueForKey(g_qeglobals.d_project_entity, "basepath"), cFullLocation);

  Sys_Printf("Loading model %s...", cPath);
  unsigned char* p = NULL;
  bool bOpen = (LoadFile(cPath, reinterpret_cast<void**>(&p)) > 0);
  if (!bOpen)
  {
    Sys_Printf(" failed. Trying PAK file...");
//    sprintf (cPath, "%stris.md2", pLocation);
	  strcpy (cPath, cFullLocation);
	  bOpen = (PakLoadAnyFile(cPath, reinterpret_cast<void**>(&p)) > 0);
  }

  if (bOpen)
  {
    Sys_Printf(" successful.\n");

    if (bASE)
    {
/*
      free(p);
      CString strOut;
      ::GetTempPath(1024, strOut.GetBuffer(1024));
      strOut.ReleaseBuffer();
      AddSlash(strOut);
      strOut += "Temp.ase";
      CopyFile(cPath, strOut, false);
      CString strIn = strOut;
      FindReplace(strOut, ".ase", ".md3");
      strcpy(cPath, strIn);
      strcpy(cSkin, strOut);
      Q3Data_ProduceTempMD3(ValueForKey(g_qeglobals.d_project_entity, "basepath"), cPath, cSkin);
      CString strModel = cPath;
      if (LoadFile(strOut.GetBuffer(0), reinterpret_cast<void**>(&p)) == 0)
      {
        Sys_Printf(" Conversion from ASE failed.\n");
        return;
      }
      bMD3 = true;
*/
    }

    if (bMD3)
    {
	    md3Header_t header;
	    md3Surface_t *pSurface;
    	header = *(md3Header_t *)p;
      if (pSkin != NULL)
      {
        strcpy(cSkin, pSkin);
      }
	    else
      {
		    cSkin[0] = '\0';
      }
      int n = header.numFrames;
    	pSurface = (md3Surface_t *) (p + header.ofsSurfaces);
    	for (int z = 0; z < header.numSurfaces; z++ )
	    {
        int nTris = pSurface->numTriangles;
        
        //unsigned char* pTris = reinterpret_cast<unsigned char*>(pSurface);
        //pTris += pSurface->ofsTriangles;

        if (nTris > 0)
        {
          int nStart = 0;
          if (pModel->pTriList == NULL)
          {
		        pModel->nModelPosition = 0;
            pModel->pTriList = new trimodel[nTris];
            pModel->nTriCount = nTris;
          }
          else
          {
            // already have one so we need to reallocate
            int nNewCount = pModel->nTriCount + nTris;
            trimodel* pNewModels = new trimodel[nNewCount];
            for (int i = 0; i < pModel->nTriCount; i++)
            {
              memcpy(&pNewModels[i], &pModel->pTriList[i], sizeof(trimodel));
            }
            nStart = pModel->nTriCount;
            pModel->nTriCount = nNewCount;
            //nTris = nNewCount;
            delete [] pModel->pTriList;
            pModel->pTriList = pNewModels;
          }
          
          md3Triangle_t *pTris = reinterpret_cast<md3Triangle_t*>((reinterpret_cast<unsigned char*>(pSurface) + pSurface->ofsTriangles));
          md3XyzNormal_t *pXyz = reinterpret_cast<md3XyzNormal_t*>((reinterpret_cast<unsigned char*>(pSurface) + pSurface->ofsXyzNormals));
          if (e->nFrame < pSurface->numFrames)
          {
            pXyz += (e->nFrame * pSurface->numVerts);
          }

          md3St_t *pST = reinterpret_cast<md3St_t*>((reinterpret_cast<unsigned char*>(pSurface) + pSurface->ofsSt)); 

          for (int i = 0; i < nTris; i++)
          {
            for (int k = 0; k < 3; k ++)
            {
              for (int j = 0; j < 3; j++)
              {
                //e->pTriList[i].v[k][j] = (f->verts[tri.index_xyz[k]].v[j] * f->scale[j] + f->translate[j]);
                pModel->pTriList[nStart].v[k][j] = pXyz[pTris[i].indexes[k]].xyz[j] * MD3_XYZ_SCALE;
              }
		          pModel->pTriList[nStart].st[k][0] = pST[pTris[i].indexes[k]].st[0];
		          pModel->pTriList[nStart].st[k][1] = pST[pTris[i].indexes[k]].st[1];
 		          ExtendBounds (pModel->pTriList[nStart].v[k], vMin, vMax);
		        }
            nStart++;
		      }

        }

        md3Shader_t *pShader = reinterpret_cast<md3Shader_t*>((reinterpret_cast<unsigned char*>(pSurface) + pSurface->ofsShaders)); 
        sprintf (cPath, "%s/%s", ValueForKey(g_qeglobals.d_project_entity, "basepath"), pShader->name);
        strlwr(cPath);
 	      pModel->nTextureBind = Texture_LoadSkin(cPath, &pModel->nSkinWidth, &pModel->nSkinHeight);
        if (pModel->nTextureBind == -1)
        {
          Sys_Printf("Model skin load failed on texture %s\n", cPath);
        }
		    pSurface = (md3Surface_t *) ((( char * ) pSurface) + pSurface->ofsEnd);
        pModel->pNext = reinterpret_cast<entitymodel_t*>(qmalloc(sizeof(entitymodel_t)));
        pModel = pModel->pNext;
      }
    }
    else
    {

      dmdl_t model;
      daliasframe_t *f;
      unsigned char* pTris = p;
      dstvert_t *pST = NULL;
      int nTris = 0;

      // grab model params
      memcpy(&model, p, sizeof(dmdl_t));
      f = (daliasframe_t*)(p + model.ofs_frames);
      pTris += model.ofs_tris;
      pST = reinterpret_cast<dstvert_t*>(p + model.ofs_st);
      nTris = model.num_tris;

	    if(pSkin)
	    {
		    strcpy (cSkin, pSkin);
		    if ((cSkin[strlen(cSkin)-1] == '\\') || (cSkin[strlen(cSkin)-1] == '/'))
			    strcat(cSkin, "skin.pcx\0");
	    }
	    else
      {
		    strcpy(cSkin, (char *)(p + model.ofs_skins));
      }

      sprintf (cPath, "%s/%s", ValueForKey(g_qeglobals.d_project_entity, "basepath"), cSkin);
      strlwr(cPath);
      pModel->nTextureBind = Texture_LoadSkin(cPath, &pModel->nSkinWidth, &pModel->nSkinHeight);
      if (pModel->nTextureBind == -1)
      {
        Sys_Printf("Model skin load failed on texture %s\n", cPath);
      }
      int nStart = 0;
      if (pModel->pTriList == NULL)
      {
		    pModel->nModelPosition = 0;
        pModel->pTriList = new trimodel[nTris];
        pModel->nTriCount = nTris;
      }
      else
      {
        // already have one so we need to reallocate
        int nNewCount = pModel->nTriCount + nTris;
        trimodel* pNewModels = new trimodel[nNewCount];
        for (int i = 0; i < pModel->nTriCount; i++)
        {
          memcpy(&pNewModels[i], &pModel->pTriList[i], sizeof(trimodel));
        }
        nStart = pModel->nTriCount;
        pModel->nTriCount = nNewCount;
        nTris = nNewCount;
        delete [] pModel->pTriList;
        pModel->pTriList = pNewModels;
      }
      
      for (int i = nStart; i < nTris; i++)
      {
        dtriangle_t tri;
        memcpy(&tri, pTris, sizeof(dtriangle_t));
        for (int k = 0; k < 3; k ++)
        {
          for (int j = 0; j < 3; j++)
          {
            pModel->pTriList[i].v[k][j] = (f->verts[tri.index_xyz[k]].v[j] * f->scale[j] + f->translate[j]);
          }

          pModel->pTriList[i].st[k][0] = pST[tri.index_st[k]].s / pModel->nSkinWidth;
          pModel->pTriList[i].st[k][1] = pST[tri.index_st[k]].t / pModel->nSkinHeight;;
          ExtendBounds (pModel->pTriList[i].v[k], vMin, vMax);
		    }
        pTris += sizeof(dtriangle_t);
		  }
    }
    free(p);
  }
  else
  {
    Sys_Printf(" failed.\n");
  }

#if 0
  if (pModel->pTriList != NULL && pModel->nTriCount > 0 && !bMD3)
  {
	  if(fabs(vMin[2]) < ((vMax[2]-vMin[2]) / 10.0))	// > 90% above 0 point.
	    pModel->nModelPosition = 1;
//	sprintf (cPath, "%s/%sskin.pcx", ValueForKey(g_qeglobals.d_project_entity, "basepath"), pLocation);
    sprintf (cPath, "%s/%s", ValueForKey(g_qeglobals.d_project_entity, "basepath"), cSkin);
 	  pModel->nTextureBind = Texture_LoadSkin(cPath, &pModel->nSkinWidth, &pModel->nSkinHeight);
    if (pModel->nTextureBind == -1)
    {
//      sprintf (cPath, "%sskin.pcx", pLocation);
		  strcpy (cPath, cSkin);
      pModel->nTextureBind = Texture_LoadSkin(cPath, &pModel->nSkinWidth, &pModel->nSkinHeight);
    }
  }
#endif

}

void setSpecialLoad(eclass_t *e, const char* pWhat, char*& p)
{
  CString str = e->comments;
  int n = str.Find(pWhat);
  if (n >= 0)
  {
    char* pText = e->comments + n + strlen(pWhat);
    if (*pText == '\"')
      pText++;

    str = "";
    while (*pText != '\"' && *pText != '\0')
    {
      str += *pText;
      pText++;
    }
    if (str.GetLength() > 0)
    {         
      p = strdup(str);
      //--LoadModel(str, e);
    }
  }
}

char	*debugname;

eclass_t *Eclass_InitFromText (char *text)
{
	char	*t;
	int		len;
	int		r, i;
	char	parms[256], *p;
	eclass_t	*e;
	char	color[128];

	e = (eclass_t*)qmalloc(sizeof(*e));
	memset (e, 0, sizeof(*e));
	
	text += strlen("/*QUAKED ");
	
// grab the name
	text = COM_Parse (text);
	e->name = (char*)qmalloc (strlen(com_token)+1);
	strcpy (e->name, com_token);
	debugname = e->name;
	
// grab the color, reformat as texture name
	r = sscanf (text," (%f %f %f)", &e->color[0], &e->color[1], &e->color[2]);
	if (r != 3)
		return e;
	sprintf (color, "(%f %f %f)", e->color[0], e->color[1], e->color[2]);
	//strcpy (e->texdef.name, color);
	e->texdef.SetName(color);

	while (*text != ')')
	{
		if (!*text)
			return e;
		text++;
	}
	text++;
	
// get the size	
	text = COM_Parse (text);
	if (com_token[0] == '(')
	{	// parse the size as two vectors
		e->fixedsize = true;
		r = sscanf (text,"%f %f %f) (%f %f %f)", &e->mins[0], &e->mins[1], &e->mins[2],
			&e->maxs[0], &e->maxs[1], &e->maxs[2]);
		if (r != 6)
			return e;

		for (i=0 ; i<2 ; i++)
		{
			while (*text != ')')
			{
				if (!*text)
					return e;
				text++;
			}
			text++;
		}
	}
	else
	{	// use the brushes
	}
	
// get the flags
	

// copy to the first /n
	p = parms;
	while (*text && *text != '\n')
		*p++ = *text++;
	*p = 0;
	text++;
	
// any remaining words are parm flags
	p = parms;
	for (i=0 ; i<8 ; i++)
	{
		p = COM_Parse (p);
		if (!p)
			break;
		strcpy (e->flagnames[i], com_token);
	} 

// find the length until close comment
	for (t=text ; t[0] && !(t[0]=='*' && t[1]=='/') ; t++)
	;
	
// copy the comment block out
	len = t-text;
	e->comments = (char*)qmalloc (len+1);
	memcpy (e->comments, text, len);
#if 0
	for (i=0 ; i<len ; i++)
		if (text[i] == '\n')
			e->comments[i] = '\r';
		else
			e->comments[i] = text[i];
#endif
	e->comments[len] = 0;
	
  setSpecialLoad(e, "model=", e->modelpath);
  setSpecialLoad(e, "skin=", e->skinpath);
  char *pFrame = NULL;
  setSpecialLoad(e, "frame=", pFrame);
  if (pFrame != NULL)
  {
    e->nFrame = atoi(pFrame);
  }

  if(!e->skinpath)
	  setSpecialLoad(e, "texture=", e->skinpath);

  // setup show flags
  e->nShowFlags = 0;
  if (strcmpi(e->name, "light") == 0)
  {
    e->nShowFlags |= ECLASS_LIGHT;
  }

  if (  (strnicmp(e->name, "info_player", strlen("info_player")) == 0)
      ||(strnicmp(e->name, "path_corner", strlen("path_corner")) == 0) 
      ||(strnicmp(e->name, "team_ctf", strlen("team_ctf")) == 0) )
  {
    e->nShowFlags |= ECLASS_ANGLE;
  }
  if (strcmpi(e->name, "path") == 0)
  {
    e->nShowFlags |= ECLASS_PATH;
  }
  if (strcmpi(e->name, "misc_model") == 0)
  {
    e->nShowFlags |= ECLASS_MISCMODEL;
  }


  return e;
}

qboolean Eclass_hasModel(eclass_t *e, vec3_t &vMin, vec3_t &vMax)
{
  if (e->modelpath != NULL)
  {
    if (e->model == NULL)
    {
      e->model = reinterpret_cast<entitymodel_t*>(qmalloc(sizeof(entitymodel_t)));
    }
    char *pModelBuff = strdup(e->modelpath);
    char *pSkinBuff = NULL;
    if (e->skinpath)
    {
      pSkinBuff = strdup(e->skinpath);
    }

    CStringList Models;
    CStringList Skins;
    char* pToken = strtok(pModelBuff, ";\0");
    while (pToken != NULL)
    {
      Models.AddTail(pToken);
      pToken = strtok(NULL, ";\0");
    }

    if (pSkinBuff != NULL)
    {
      pToken = strtok(pSkinBuff, ";\0");
      while (pToken != NULL)
      {
        Skins.AddTail(pToken);
        pToken = strtok(NULL, ";\0");
      }
    }

    entitymodel *model = e->model;
    for (int i = 0; i < Models.GetCount(); i++)
    {
      char *pSkin = NULL;
      if (i < Skins.GetCount())
      {
        pSkin = Skins.GetAt(Skins.FindIndex(i)).GetBuffer(0);
      }
      LoadModel(Models.GetAt(Models.FindIndex(i)), e, vMin, vMax, model, pSkin);
      model->pNext = reinterpret_cast<entitymodel_t*>(qmalloc(sizeof(entitymodel_t)));
      model = model->pNext;
    }

    // at this poitn vMin and vMax contain the min max of the model
    // which needs to be centered at origin 0, 0, 0

    VectorSnap(vMin);
    VectorSnap(vMax);

    if (vMax[0] - vMin[0] < 2)
    {
      vMin[0] -= 1;
      vMax[0] += 1;
    }

    if (vMin[1] - vMax[1] < 2)
    {
      vMin[1] -= 1;
      vMax[1] += 1;
    }

    if (vMax[2] - vMin[2] < 2)
    {
      vMax[2] -= 1;
      vMax[2] += 1;
    }

    vec3_t vTemp;
    VectorAdd(vMin, vMax, vTemp);
    VectorScale(vTemp, 0.5, vTemp);
    model = e->model;
    while (model != NULL)
    {
      for (i = 0; i < model->nTriCount; i++)
      {
        for (int j = 0; j < 3; j++)
        {
          ;//VectorSubtract(model->pTriList[i].v[j], vTemp, model->pTriList[i].v[j]);
        }
      }
      model = model->pNext;
    }

    free(pModelBuff);
    free(e->modelpath);
    e->modelpath = NULL;
	  
    if(e->skinpath)
	  {
		  free(e->skinpath);
		  e->skinpath = NULL;
      free(pSkinBuff);
	  }

  }
  return (e->model != NULL && e->model->nTriCount > 0);
}


void EClass_InsertSortedList(eclass_t *&pList, eclass_t *e)
{
	eclass_t	*s;
	
	if (!pList)
	{
		pList = e;
		return;
	}


	s = pList;
	if (stricmp (e->name, s->name) < 0)
	{
		e->next = s;
		pList = e;
		return;
	}

	do
	{
		if (!s->next || stricmp (e->name, s->next->name) < 0)
		{
			e->next = s->next;
			s->next = e;
			return;
		}
		s=s->next;
	} while (1);
}

/*
=================
Eclass_InsertAlphabetized
=================
*/
void Eclass_InsertAlphabetized (eclass_t *e)
{
#if 1
  EClass_InsertSortedList(eclass, e);
#else
	eclass_t	*s;
	
	if (!eclass)
	{
		eclass = e;
		return;
	}


	s = eclass;
	if (stricmp (e->name, s->name) < 0)
	{
		e->next = s;
		eclass = e;
		return;
	}

	do
	{
		if (!s->next || stricmp (e->name, s->next->name) < 0)
		{
			e->next = s->next;
			s->next = e;
			return;
		}
		s=s->next;
	} while (1);
#endif
}


/*
=================
Eclass_ScanFile
=================
*/

qboolean parsing_single = false;
qboolean eclass_found;
eclass_t *eclass_e;
//#ifdef BUILD_LIST
extern bool g_bBuildList;
CString strDefFile;
//#endif
void Eclass_ScanFile (char *filename)
{
	int		size;
	char	*data;
	eclass_t	*e;
	int		i;
	char    temp[1024];
	
	QE_ConvertDOSToUnixName( temp, filename );
	
	Sys_Printf ("ScanFile: %s\n", temp);
	
	// BUG
	size = LoadFile (filename, (void**)&data);
	eclass_found = false;
	for (i=0 ; i<size ; i++)
		if (!strncmp(data+i, "/*QUAKED",8))
		{
			
			//#ifdef BUILD_LIST
			if (g_bBuildList)
			{
				CString strDef = "";
				int j = i;
				while (1)
				{
					strDef += *(data+j);
					if (*(data+j) == '/' && *(data+j-1) == '*')
						break;
					j++;
				}
				strDef += "\r\n\r\n\r\n";
				strDefFile += strDef;
			}
			//#endif
			e = Eclass_InitFromText (data+i);
			if (e)
				Eclass_InsertAlphabetized (e);
			else
				printf ("Error parsing: %s in %s\n",debugname, filename);

			// single ?
			eclass_e = e;
			eclass_found = true;
			if ( parsing_single )
				break;
		}
		
		free (data);		
}



void Eclass_InitForSourceDirectory (char *path)
{
	struct _finddata_t fileinfo;
	int		handle;
	char	filename[1024];
	char	filebase[1024];
	char    temp[1024];
	char	*s;

	QE_ConvertDOSToUnixName( temp, path );

	Sys_Printf ("Eclass_InitForSourceDirectory: %s\n", temp );

	strcpy (filebase, path);
	s = filebase + strlen(filebase)-1;
	while (*s != '\\' && *s != '/' && s!=filebase)
		s--;
	*s = 0;

  CleanUpEntities();
	eclass = NULL;
//#ifdef BUILD_LIST
  if (g_bBuildList)
    strDefFile = "";
//#endif
	handle = _findfirst (path, &fileinfo);
	if (handle != -1)
	{
		do
		{
			sprintf (filename, "%s\\%s", filebase, fileinfo.name);
			Eclass_ScanFile (filename);
		} while (_findnext( handle, &fileinfo ) != -1);

		_findclose (handle);
	}

//#ifdef BUILD_LIST
  if (g_bBuildList)
  {
    CFile file;
    if (file.Open("c:\\entities.def", CFile::modeCreate | CFile::modeWrite)) 
    {
      file.Write(strDefFile.GetBuffer(0), strDefFile.GetLength());
      file.Close();
    }
  }
//#endif

	eclass_bad = Eclass_InitFromText ("/*QUAKED UNKNOWN_CLASS (0 0.5 0) ?");
}

eclass_t *Eclass_ForName (char *name, qboolean has_brushes)
{
	eclass_t	*e;
	char		init[1024];

#ifdef _DEBUG
  // grouping stuff, not an eclass
  if (strcmp(name, "group_info")==0)
    Sys_Printf("WARNING: unexpected group_info entity in Eclass_ForName\n");
#endif

	if (!name)
		return eclass_bad;

	for (e=eclass ; e ; e=e->next)
		if (!strcmp (name, e->name))
			return e;

	// create a new class for it
	if (has_brushes)
	{
		sprintf (init, "/*QUAKED %s (0 0.5 0) ?\nNot found in source.\n", name);
		e = Eclass_InitFromText (init);
	}
	else
	{
		sprintf (init, "/*QUAKED %s (0 0.5 0) (-8 -8 -8) (8 8 8)\nNot found in source.\n", name);
		e = Eclass_InitFromText (init);
	}

	Eclass_InsertAlphabetized (e);

	return e;
}


eclass_t* GetCachedModel(entity_t *pEntity, const char *pName, vec3_t &vMin, vec3_t &vMax)
{

	eclass_t *e = NULL;
  if (pName == NULL || strlen(pName) == 0)
  {
    return NULL;
  }

	for (e = g_md3Cache; e ; e = e->next)
  {
		if (!strcmp (pName, e->name))
    {
      pEntity->md3Class = e;
      VectorCopy(e->mins, vMin);
      VectorCopy(e->maxs, vMax);
			return e;
    }
  }

	e = (eclass_t*)qmalloc(sizeof(*e));
	memset (e, 0, sizeof(*e));
  e->name = strdup(pName);
  e->modelpath = strdup(pName);
  e->skinpath = strdup(pName);
  char *p = strstr(e->skinpath, ".md3");
  if (p != NULL)
  {
    p++;
    strncpy(p, "tga", 3);
  }
  else
  {
    free(e->skinpath);
    e->skinpath = NULL;
  }

  e->color[0] = e->color[2] = 0.85;

  if (Eclass_hasModel(e, vMin, vMax))
  {
    EClass_InsertSortedList(g_md3Cache, e);
    VectorCopy(vMin, e->mins);
    VectorCopy(vMax, e->maxs);
    pEntity->md3Class = e;
    return e;
  }

  return NULL;
}
