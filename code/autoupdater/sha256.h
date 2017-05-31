/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef SHA256_H
#define SHA256_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
#ifdef _MSC_VER
typedef unsigned __int8 uint8;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
#else
#include <stdint.h>
typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
#endif

typedef struct {
	uint8 data[64];
	uint32 datalen;
	uint64 bitlen;
	uint32 state[8];
} SHA256_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8 data[], size_t len);
void sha256_final(SHA256_CTX *ctx, uint8 hash[]);

#endif   // SHA256_H
