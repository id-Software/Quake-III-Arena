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

#import <OmniTimer/OmniTimer.h>

#define OTSTART(node) OTStackPush(node)
#define OTSTOP(node)  OTStackPop()

extern OTStackNode *rootNode;
extern OTStackNode *markFragmentsNode1;
extern OTStackNode *markFragmentsNode2;
extern OTStackNode *markFragmentsGrid;
extern OTStackNode *markFragmentsNode4;
extern OTStackNode *addMarkFragmentsNode;
extern OTStackNode *chopPolyNode;
extern OTStackNode *boxTraceNode;
extern OTStackNode *boxOnPlaneSideNode;
extern OTStackNode *recursiveWorldNode;
extern OTStackNode *surfaceAnimNode;
extern OTStackNode *surfaceFaceNode;
extern OTStackNode *surfaceMeshNode;
extern OTStackNode *surfaceEndNode;
extern OTStackNode *shadowEndNode;
extern OTStackNode *stageIteratorGenericNode;
extern OTStackNode *computeColorAndTexNode;
extern OTStackNode *mp3DecodeNode;

extern void InitializeTimers();

#else

#define OTSTART(node)
#define OTSTOP(node)

#endif

