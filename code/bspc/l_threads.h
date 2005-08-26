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

extern int numthreads;

void ThreadSetDefault (void);
int GetThreadWork (void);
void RunThreadsOnIndividual (int workcnt, qboolean showpacifier, void(*func)(int));
void RunThreadsOn (int workcnt, qboolean showpacifier, void(*func)(int));

//mutex
void ThreadSetupLock(void);
void ThreadShutdownLock(void);
void ThreadLock (void);
void ThreadUnlock (void);
//semaphore
void ThreadSetupSemaphore(void);
void ThreadShutdownSemaphore(void);
void ThreadSemaphoreWait(void);
void ThreadSemaphoreIncrease(int count);
//add/remove threads
void AddThread(void (*func)(int));
void RemoveThread(int threadid);
void WaitForAllThreadsFinished(void);
int GetNumThreads(void);

