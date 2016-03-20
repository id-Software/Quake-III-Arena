#ifndef __SPEEX_TYPES_H__
#define __SPEEX_TYPES_H__

#ifdef _MSC_VER
typedef __int16 spx_int16_t;
typedef __int32 spx_int32_t;
typedef unsigned __int16 spx_uint16_t;
typedef unsigned __int32 spx_uint32_t;
#else
#include <stdint.h>
typedef int16_t spx_int16_t;
typedef int32_t spx_int32_t;
typedef uint16_t spx_uint16_t;
typedef uint32_t spx_uint32_t;
#endif

#endif

