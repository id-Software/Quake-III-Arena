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
#ifdef OMNI_TIMER

#import "macosx_timers.h"

#import <Foundation/NSString.h>
#import <stdio.h>

OTStackNode *rootNode;
OTStackNode *markFragmentsNode1;
OTStackNode *markFragmentsNode2;
OTStackNode *markFragmentsGrid;
OTStackNode *markFragmentsNode4;
OTStackNode *addMarkFragmentsNode;
OTStackNode *chopPolyNode;
OTStackNode *boxTraceNode;
OTStackNode *boxOnPlaneSideNode;
OTStackNode *recursiveWorldNode;
OTStackNode *surfaceAnimNode;
OTStackNode *surfaceFaceNode;
OTStackNode *surfaceMeshNode;
OTStackNode *surfaceEndNode;
OTStackNode *shadowEndNode;
OTStackNode *stageIteratorGenericNode;
OTStackNode *computeColorAndTexNode;
OTStackNode *mp3DecodeNode;

void InitializeTimers()
{
    const char *env;
    
    OTSetup();
    
    rootNode = OTStackNodeCreate("root");
    markFragmentsNode1 = OTStackNodeCreate("R_MarkFragments 1");
    markFragmentsNode2 = OTStackNodeCreate("R_MarkFragments 2");
    markFragmentsGrid = OTStackNodeCreate("R_MarkFragmentsGrid");
    markFragmentsNode4 = OTStackNodeCreate("R_MarkFragments 4");
    addMarkFragmentsNode = OTStackNodeCreate("R_AddMarkFragments");
    chopPolyNode = OTStackNodeCreate("R_ChopPolyBehindPlane");
    boxTraceNode = OTStackNodeCreate("CM_BoxTrace");
    boxOnPlaneSideNode = OTStackNodeCreate("BoxOnPlaneSide");
    recursiveWorldNode = OTStackNodeCreate("R_RecursiveWorldNode");
    surfaceAnimNode = OTStackNodeCreate("RB_SurfaceAnim");
    surfaceFaceNode = OTStackNodeCreate("RB_SurfaceFace");
    surfaceMeshNode = OTStackNodeCreate("RB_SurfaceMesh");
    surfaceEndNode = OTStackNodeCreate("RB_EndSurface");
    shadowEndNode = OTStackNodeCreate("RB_ShadowTessEnd");
    stageIteratorGenericNode = OTStackNodeCreate("RB_StageIteratorGeneric");
    computeColorAndTexNode = OTStackNodeCreate("ComputeColors & ComputeTexCoords");
    mp3DecodeNode = OTStackNodeCreate("MP3Stream_Decode");
}

#endif // OMNI_TIMER

