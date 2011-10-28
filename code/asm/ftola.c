/*
===========================================================================
Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>

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

#include "qasm-inline.h"

static const unsigned short fpucw = 0x0C7F;

/*
 * GNU inline asm ftol conversion functions using SSE or FPU
 */

long qftolsse(float f)
{
  long retval;
  
  __asm__ volatile
  (
    "cvttss2si %1, %0\n"
    : "=r" (retval)
    : "x" (f)
  );
  
  return retval;
}

int qvmftolsse(void)
{
  int retval;
  
  __asm__ volatile
  (
    "movss (" EDI ", " EBX ", 4), %%xmm0\n"
    "cvttss2si %%xmm0, %0\n"
    : "=r" (retval)
    :
    : "%xmm0"
  );
  
  return retval;
}

long qftolx87(float f)
{
  long retval;
  unsigned short oldcw;

  __asm__ volatile
  (
    "fnstcw %2\n"
    "fldcw %3\n"
    "flds %1\n"
    "fistpl %1\n"
    "fldcw %2\n"
    "mov %1, %0\n"
    : "=r" (retval)
    : "m" (f), "m" (oldcw), "m" (fpucw)
  );
  
  return retval;
}

int qvmftolx87(void)
{
  int retval;
  unsigned short oldcw;

  __asm__ volatile
  (
    "fnstcw %1\n"
    "fldcw %2\n"
    "flds (" EDI ", " EBX ", 4)\n"
    "fistpl (" EDI ", " EBX ", 4)\n"
    "fldcw %1\n"
    "mov (" EDI ", " EBX ", 4), %0\n"
    : "=r" (retval)
    : "m" (oldcw), "m" (fpucw)
  );
  
  return retval;
}
