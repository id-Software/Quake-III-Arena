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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../../qcommon/q_platform.h"
#include "cmdlib.h"
#include "mathlib.h"
#include "../../qcommon/qfiles.h"

/* 19079 total symbols in FI, 2002 Jan 23 */
#define DEFAULT_HASHTABLE_SIZE 2048

char	outputFilename[MAX_OS_PATH];

// the zero page size is just used for detecting run time faults
#define	ZERO_PAGE_SIZE	0		// 256

typedef enum {
	OP_UNDEF, 

	OP_IGNORE, 

	OP_BREAK, 

	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,
	OP_LOCAL,

	OP_JUMP,

	//-------------------

	OP_EQ,
	OP_NE,

	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,

	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,

	OP_EQF,
	OP_NEF,

	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,

	//-------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,				// *(stack[top-1]) = stack[yop
	OP_ARG,
	OP_BLOCK_COPY,

	//-------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI
} opcode_t;

typedef struct {
	int		imageBytes;		// after decompression
	int		entryPoint;
	int		stackBase;
	int		stackSize;
} executableHeader_t;

typedef enum {
	CODESEG,
	DATASEG,	// initialized 32 bit data, will be byte swapped
	LITSEG,		// strings
	BSSSEG,		// 0 filled
	JTRGSEG,	// pseudo-segment that contains only jump table targets
	NUM_SEGMENTS
} segmentName_t;

#define	MAX_IMAGE	0x400000

typedef struct {
	byte	image[MAX_IMAGE];
	int		imageUsed;
	int		segmentBase;		// only valid on second pass
} segment_t;

typedef struct symbol_s {
	struct	symbol_s	*next;
	int		hash;
	segment_t	*segment;
	char	*name;
	int		value;
} symbol_t;

typedef struct hashchain_s {
  void *data;
  struct hashchain_s *next;
} hashchain_t;

typedef struct hashtable_s {
  int buckets;
  hashchain_t **table;
} hashtable_t;

int symtablelen = DEFAULT_HASHTABLE_SIZE;
hashtable_t *symtable;
hashtable_t *optable;

segment_t	segment[NUM_SEGMENTS];
segment_t	*currentSegment;

int		passNumber;

int		numSymbols;
int		errorCount;

typedef struct options_s {
	qboolean verbose;
	qboolean writeMapFile;
	qboolean vanillaQ3Compatibility;
} options_t;

options_t options = { 0 };

symbol_t	*symbols;
symbol_t	*lastSymbol = 0;  /* Most recent symbol defined. */


#define	MAX_ASM_FILES	256
int		numAsmFiles;
char	*asmFiles[MAX_ASM_FILES];
char	*asmFileNames[MAX_ASM_FILES];

int		currentFileIndex;
char	*currentFileName;
int		currentFileLine;

//int		stackSize = 16384;
int		stackSize = 0x10000;

// we need to convert arg and ret instructions to
// stores to the local stack frame, so we need to track the
// characteristics of the current functions stack frame
int		currentLocals;			// bytes of locals needed by this function
int		currentArgs;			// bytes of largest argument list called from this function
int		currentArgOffset;		// byte offset in currentArgs to store next arg, reset each call

#define	MAX_LINE_LENGTH	1024
char	lineBuffer[MAX_LINE_LENGTH];
int		lineParseOffset;
char	token[MAX_LINE_LENGTH];

int		instructionCount;

typedef struct {
	char	*name;
	int		opcode;
} sourceOps_t;

sourceOps_t		sourceOps[] = {
#include "opstrings.h"
};

#define	NUM_SOURCE_OPS ( sizeof( sourceOps ) / sizeof( sourceOps[0] ) )

int		opcodesHash[ NUM_SOURCE_OPS ];



static int vreport (const char* fmt, va_list vp)
{
  if (options.verbose != qtrue)
      return 0;
  return vprintf(fmt, vp);
}

static int report (const char *fmt, ...)
{
  va_list va;
  int retval;

  va_start(va, fmt);
  retval = vreport(fmt, va);
  va_end(va);
  return retval;
}

/* The chain-and-bucket hash table.  -PH */

static void hashtable_init (hashtable_t *H, int buckets)
{
  H->buckets = buckets;
  H->table = calloc(H->buckets, sizeof(*(H->table)));
  return;
}

static hashtable_t *hashtable_new (int buckets)
{
  hashtable_t *H;

  H = malloc(sizeof(hashtable_t));
  hashtable_init(H, buckets);
  return H;
}

/* No destroy/destructor.  No need. */

static void hashtable_add (hashtable_t *H, int hashvalue, void *datum)
{
  hashchain_t *hc, **hb;

  hashvalue = (abs(hashvalue) % H->buckets);
  hb = &(H->table[hashvalue]);
  if (*hb == 0)
    {
      /* Empty bucket.  Create new one. */
      *hb = calloc(1, sizeof(**hb));
      hc = *hb;
    }
  else
    {
      /* Get hc to point to last node in chain. */
      for (hc = *hb; hc && hc->next; hc = hc->next);
      hc->next = calloc(1, sizeof(*hc));
      hc = hc->next;
    }
  hc->data = datum;
  hc->next = 0;
  return;
}

static hashchain_t *hashtable_get (hashtable_t *H, int hashvalue)
{
  hashvalue = (abs(hashvalue) % H->buckets);
  return (H->table[hashvalue]);
}

static void hashtable_stats (hashtable_t *H)
{
  int len, empties, longest, nodes;
  int i;
  float meanlen;
  hashchain_t *hc;

  report("Stats for hashtable %08X", H);
  empties = 0;
  longest = 0;
  nodes = 0;
  for (i = 0; i < H->buckets; i++)
    {
      if (H->table[i] == 0)
        { empties++; continue; }
      for (hc = H->table[i], len = 0; hc; hc = hc->next, len++);
      if (len > longest) { longest = len; }
      nodes += len;
    }
  meanlen = (float)(nodes) / (H->buckets - empties);
#if 0
/* Long stats display */
  report(" Total buckets: %d\n", H->buckets);
  report(" Total stored nodes: %d\n", nodes);
  report(" Longest chain: %d\n", longest);
  report(" Empty chains: %d\n", empties);
  report(" Mean non-empty chain length: %f\n", meanlen);
#else //0
/* Short stats display */
  report(", %d buckets, %d nodes", H->buckets, nodes);
  report("\n");
  report(" Longest chain: %d, empty chains: %d, mean non-empty: %f", longest, empties, meanlen);
#endif //0
  report("\n");
}


/* Kludge. */
/* Check if symbol already exists. */
/* Returns 0 if symbol does NOT already exist, non-zero otherwise. */
static int hashtable_symbol_exists (hashtable_t *H, int hash, char *sym)
{
  hashchain_t *hc;
  symbol_t *s;

  hash = (abs(hash) % H->buckets);
  hc = H->table[hash];
  if (hc == 0)
    {
      /* Empty chain means this symbol has not yet been defined. */
      return 0;
    }
  for (; hc; hc = hc->next)
    {
      s = (symbol_t*)hc->data;
//      if ((hash == s->hash) && (strcmp(sym, s->name) == 0))
/* We _already_ know the hash is the same.  That's why we're probing! */
      if (strcmp(sym, s->name) == 0)
        {
          /* Symbol collisions -- symbol already exists. */
          return 1;
        }
    }
  return 0;  /* Can't find collision. */
}




/* Comparator function for quicksorting. */
static int symlist_cmp (const void *e1, const void *e2)
{
  const symbol_t *a, *b;

  a = *(const symbol_t **)e1;
  b = *(const symbol_t **)e2;
//crumb("Symbol comparison (1) %d  to  (2) %d\n", a->value, b->value);
  return ( a->value - b->value);
}

/*
  Sort the symbols list by using QuickSort (qsort()).
  This may take a LOT of memory (a few megabytes?), but memory is cheap these days.
  However, qsort(3) already exists, and I'm really lazy.
 -PH
*/
static void sort_symbols ()
{
  int i, elems;
  symbol_t *s;
  symbol_t **symlist;

  if(!symbols)
  	return;

//crumb("sort_symbols: Constructing symlist array\n");
  for (elems = 0, s = symbols; s; s = s->next, elems++) /* nop */ ;

  symlist = malloc(elems * sizeof(symbol_t*));
  for (i = 0, s = symbols; s; s = s->next, i++)
    {
      symlist[i] = s;
    }
//crumbf("sort_symbols: Quick-sorting %d symbols\n", elems);
  qsort(symlist, elems, sizeof(symbol_t*), symlist_cmp);
//crumbf("sort_symbols: Reconstructing symbols list\n");
  s = symbols = symlist[0];
  for (i = 1; i < elems; i++)
    {
      s->next = symlist[i];
      s = s->next;
    }
  lastSymbol = s;
  s->next = 0;
//crumbf("sort_symbols: verifying..."); fflush(stdout);
  for (i = 0, s = symbols; s; s = s->next, i++) /*nop*/ ;
//crumbf(" %d elements\n", i);
  free(symlist);  /* d'oh.  no gc. */
}


#ifdef _MSC_VER
#define INT64 __int64
#define atoi64 _atoi64
#else
#define INT64 long long int
#define atoi64 atoll
#endif

/*
 Problem:
	BYTE values are specified as signed decimal string.  A properly functional
	atoip() will cap large signed values at 0x7FFFFFFF.  Negative word values are
	often specified as very large decimal values by lcc.  Therefore, values that
	should be between 0x7FFFFFFF and 0xFFFFFFFF come out as 0x7FFFFFFF when using
	atoi().  Bad.

 This function is one big evil hack to work around this problem.
*/
static int atoiNoCap (const char *s)
{
  INT64 l;
  union {
    unsigned int u;
    signed int i;
  } retval;

  l = atoi64(s);
  /* Now smash to signed 32 bits accordingly. */
  if (l < 0) {
    retval.i = (int)l;
  } else {
    retval.u = (unsigned int)l;
  }
  return retval.i;  /* <- union hackage.  I feel dirty with this.  -PH */
}



/*
=============
HashString
=============
*/
/* Default hash function of Kazlib 1.19, slightly modified. */
static unsigned int HashString (const char *key)
{
    static unsigned long randbox[] = {
    0x49848f1bU, 0xe6255dbaU, 0x36da5bdcU, 0x47bf94e9U,
    0x8cbcce22U, 0x559fc06aU, 0xd268f536U, 0xe10af79aU,
    0xc1af4d69U, 0x1d2917b5U, 0xec4c304dU, 0x9ee5016cU,
    0x69232f74U, 0xfead7bb3U, 0xe9089ab6U, 0xf012f6aeU,
    };

    const char *str = key;
    unsigned int acc = 0;

    while (*str) {
    acc ^= randbox[(*str + acc) & 0xf];
    acc = (acc << 1) | (acc >> 31);
    acc &= 0xffffffffU;
    acc ^= randbox[((*str++ >> 4) + acc) & 0xf];
    acc = (acc << 2) | (acc >> 30);
    acc &= 0xffffffffU;
    }
    return abs(acc);
}


/*
============
CodeError
============
*/
static void CodeError( char *fmt, ... ) {
	va_list		argptr;

	errorCount++;

	fprintf( stderr, "%s:%i ", currentFileName, currentFileLine );

	va_start( argptr,fmt );
	vfprintf( stderr, fmt, argptr );
	va_end( argptr );
}

/*
============
EmitByte
============
*/
static void EmitByte( segment_t *seg, int v ) {
	if ( seg->imageUsed >= MAX_IMAGE ) {
		Error( "MAX_IMAGE" );
	}
	seg->image[ seg->imageUsed ] = v;
	seg->imageUsed++;
}

/*
============
EmitInt
============
*/
static void EmitInt( segment_t *seg, int v ) {
	if ( seg->imageUsed >= MAX_IMAGE - 4) {
		Error( "MAX_IMAGE" );
	}
	seg->image[ seg->imageUsed ] = v & 255;
	seg->image[ seg->imageUsed + 1 ] = ( v >> 8 ) & 255;
	seg->image[ seg->imageUsed + 2 ] = ( v >> 16 ) & 255;
	seg->image[ seg->imageUsed + 3 ] = ( v >> 24 ) & 255;
	seg->imageUsed += 4;
}

/*
============
DefineSymbol

Symbols can only be defined on pass 0
============
*/
static void DefineSymbol( char *sym, int value ) {
	/* Hand optimization by PhaethonH */
	symbol_t	*s;
	char		expanded[MAX_LINE_LENGTH];
	int			hash;

	if ( passNumber == 1 ) {
		return;
	}

	// add the file prefix to local symbols to guarantee unique
	if ( sym[0] == '$' ) {
		sprintf( expanded, "%s_%i", sym, currentFileIndex );
		sym = expanded;
	}

	hash = HashString( sym );

	if (hashtable_symbol_exists(symtable, hash, sym)) {
		CodeError( "Multiple definitions for %s\n", sym );
		return;
	}

	s = malloc( sizeof( *s ) );
	s->next = NULL;
	s->name = copystring( sym );
	s->hash = hash;
	s->value = value;
	s->segment = currentSegment;

	hashtable_add(symtable, hash, s);

/*
  Hash table lookup already speeds up symbol lookup enormously.
  We postpone sorting until end of pass 0.
  Since we're not doing the insertion sort, lastSymbol should always
   wind up pointing to the end of list.
  This allows constant time for adding to the list.
 -PH
*/
	if (symbols == 0) {
		lastSymbol = symbols = s;
	} else {
		lastSymbol->next = s;
		lastSymbol = s;
	}
}


/*
============
LookupSymbol

Symbols can only be evaluated on pass 1
============
*/
static int LookupSymbol( char *sym ) {
	symbol_t	*s;
	char		expanded[MAX_LINE_LENGTH];
	int			hash;
	hashchain_t *hc;

	if ( passNumber == 0 ) {
		return 0;
	}

	// add the file prefix to local symbols to guarantee unique
	if ( sym[0] == '$' ) {
		sprintf( expanded, "%s_%i", sym, currentFileIndex );
		sym = expanded;
	}

	hash = HashString( sym );

/*
  Hand optimization by PhaethonH

  Using a hash table with chain/bucket for lookups alone sped up q3asm by almost 3x for me.
 -PH
*/
	for (hc = hashtable_get(symtable, hash); hc; hc = hc->next) {
		s = (symbol_t*)hc->data;  /* ugly typecasting, but it's fast! */
		if ( (hash == s->hash) && !strcmp(sym, s->name) ) {
			return s->segment->segmentBase + s->value;
		}
	}

	CodeError( "error: symbol %s undefined\n", sym );
	passNumber = 0;
	DefineSymbol( sym, 0 );	// so more errors aren't printed
	passNumber = 1;
	return 0;
}


/*
==============
ExtractLine

Extracts the next line from the given text block.
If a full line isn't parsed, returns NULL
Otherwise returns the updated parse pointer
===============
*/
static char *ExtractLine( char *data ) {
/* Goal:
	 Given a string `data', extract one text line into buffer `lineBuffer' that
	 is no longer than MAX_LINE_LENGTH characters long.  Return value is
	 remainder of `data' that isn't part of `lineBuffer'.
 -PH
*/
	/* Hand-optimized by PhaethonH */
	char 	*p, *q;

	currentFileLine++;

	lineParseOffset = 0;
	token[0] = 0;
	*lineBuffer = 0;

	p = q = data;
	if (!*q) {
		return NULL;
	}

	for ( ; !((*p == 0) || (*p == '\n')); p++)  /* nop */ ;

	if ((p - q) >= MAX_LINE_LENGTH) {
		CodeError( "MAX_LINE_LENGTH" );
		return data;
	}

	memcpy( lineBuffer, data, (p - data) );
	lineBuffer[(p - data)] = 0;
	p += (*p == '\n') ? 1 : 0;  /* Skip over final newline. */
	return p;
}


/*
==============
Parse

Parse a token out of linebuffer
==============
*/
static qboolean Parse( void ) {
	/* Hand-optimized by PhaethonH */
	const char 	*p, *q;

	/* Because lineParseOffset is only updated just before exit, this makes this code version somewhat harder to debug under a symbolic debugger. */

	*token = 0;  /* Clear token. */

	// skip whitespace
	for (p = lineBuffer + lineParseOffset; *p && (*p <= ' '); p++) /* nop */ ;

	// skip ; comments
	/* die on end-of-string */
	if ((*p == ';') || (*p == 0)) {
		lineParseOffset = p - lineBuffer;
		return qfalse;
	}

	q = p;  /* Mark the start of token. */
	/* Find separator first. */
	for ( ; *p > 32; p++) /* nop */ ;  /* XXX: unsafe assumptions. */
	/* *p now sits on separator.  Mangle other values accordingly. */
	strncpy(token, q, p - q);
	token[p - q] = 0;

	lineParseOffset = p - lineBuffer;

	return qtrue;
}


/*
==============
ParseValue
==============
*/
static int ParseValue( void ) {
	Parse();
	return atoiNoCap( token );
}


/*
==============
ParseExpression
==============
*/
static int ParseExpression(void) {
	/* Hand optimization, PhaethonH */
	int		i, j;
	char	sym[MAX_LINE_LENGTH];
	int		v;

	/* Skip over a leading minus. */
	for ( i = ((token[0] == '-') ? 1 : 0) ; i < MAX_LINE_LENGTH ; i++ ) {
		if ( token[i] == '+' || token[i] == '-' || token[i] == 0 ) {
			break;
		}
	}

	memcpy( sym, token, i );
	sym[i] = 0;

	switch (*sym) {  /* Resolve depending on first character. */
/* Optimizing compilers can convert cases into "calculated jumps".  I think these are faster.  -PH */
		case '-':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			v = atoiNoCap(sym);
			break;
		default:
			v = LookupSymbol(sym);
			break;
	}

	// parse add / subtract offsets
	while ( token[i] != 0 ) {
		for ( j = i + 1 ; j < MAX_LINE_LENGTH ; j++ ) {
			if ( token[j] == '+' || token[j] == '-' || token[j] == 0 ) {
				break;
			}
		}

		memcpy( sym, token+i+1, j-i-1 );
		sym[j-i-1] = 0;

		switch (token[i]) {
			case '+':
				v += atoiNoCap(sym);
				break;
			case '-':
				v -= atoiNoCap(sym);
				break;
		}

		i = j;
	}

	return v;
}


/*
==============
HackToSegment

BIG HACK: I want to put all 32 bit values in the data
segment so they can be byte swapped, and all char data in the lit
segment, but switch jump tables are emited in the lit segment and
initialized strng variables are put in the data segment.

I can change segments here, but I also need to fixup the
label that was just defined

Note that the lit segment is read-write in the VM, so strings
aren't read only as in some architectures.
==============
*/
static void HackToSegment( segmentName_t seg ) {
	if ( currentSegment == &segment[seg] ) {
		return;
	}

	currentSegment = &segment[seg];
	if ( passNumber == 0 ) {
		lastSymbol->segment = currentSegment;
		lastSymbol->value = currentSegment->imageUsed;
	}
}







//#define STAT(L) report("STAT " L "\n");
#define STAT(L)
#define ASM(O) int TryAssemble##O ()


/*
  These clauses were moved out from AssembleLine() to allow reordering of if's.
  An optimizing compiler should reconstruct these back into inline code.
 -PH
*/

	// call instructions reset currentArgOffset
ASM(CALL)
{
	if ( !strncmp( token, "CALL", 4 ) ) {
STAT("CALL");
		EmitByte( &segment[CODESEG], OP_CALL );
		instructionCount++;
		currentArgOffset = 0;
		return 1;
	}
	return 0;
}

	// arg is converted to a reversed store
ASM(ARG)
{
	if ( !strncmp( token, "ARG", 3 ) ) {
STAT("ARG");
		EmitByte( &segment[CODESEG], OP_ARG );
		instructionCount++;
		if ( 8 + currentArgOffset >= 256 ) {
			CodeError( "currentArgOffset >= 256" );
			return 1;
		}
		EmitByte( &segment[CODESEG], 8 + currentArgOffset );
		currentArgOffset += 4;
		return 1;
	}
	return 0;
}

	// ret just leaves something on the op stack
ASM(RET)
{
	if ( !strncmp( token, "RET", 3 ) ) {
STAT("RET");
		EmitByte( &segment[CODESEG], OP_LEAVE );
		instructionCount++;
		EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );
		return 1;
	}
	return 0;
}

	// pop is needed to discard the return value of 
	// a function
ASM(POP)
{
	if ( !strncmp( token, "pop", 3 ) ) {
STAT("POP");
		EmitByte( &segment[CODESEG], OP_POP );
		instructionCount++;
		return 1;
	}
	return 0;
}

	// address of a parameter is converted to OP_LOCAL
ASM(ADDRF)
{
	int		v;
	if ( !strncmp( token, "ADDRF", 5 ) ) {
STAT("ADDRF");
		instructionCount++;
		Parse();
		v = ParseExpression();
		v = 16 + currentArgs + currentLocals + v;
		EmitByte( &segment[CODESEG], OP_LOCAL );
		EmitInt( &segment[CODESEG], v );
		return 1;
	}
	return 0;
}

	// address of a local is converted to OP_LOCAL
ASM(ADDRL)
{
	int		v;
	if ( !strncmp( token, "ADDRL", 5 ) ) {
STAT("ADDRL");
		instructionCount++;
		Parse();
		v = ParseExpression();
		v = 8 + currentArgs + v;
		EmitByte( &segment[CODESEG], OP_LOCAL );
		EmitInt( &segment[CODESEG], v );
		return 1;
	}
	return 0;
}

ASM(PROC)
{
	char	name[1024];
	if ( !strcmp( token, "proc" ) ) {
STAT("PROC");
		Parse();					// function name
		strcpy( name, token );

		DefineSymbol( token, instructionCount ); // segment[CODESEG].imageUsed );

		currentLocals = ParseValue();	// locals
		currentLocals = ( currentLocals + 3 ) & ~3;
		currentArgs = ParseValue();		// arg marshalling
		currentArgs = ( currentArgs + 3 ) & ~3;

		if ( 8 + currentLocals + currentArgs >= 32767 ) {
			CodeError( "Locals > 32k in %s\n", name );
		}

		instructionCount++;
		EmitByte( &segment[CODESEG], OP_ENTER );
		EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );
		return 1;
	}
	return 0;
}


ASM(ENDPROC)
{
	if ( !strcmp( token, "endproc" ) ) {
STAT("ENDPROC");
		Parse();				// skip the function name
		ParseValue();		// locals
		ParseValue();		// arg marshalling

		// all functions must leave something on the opstack
		instructionCount++;
		EmitByte( &segment[CODESEG], OP_PUSH );

		instructionCount++;
		EmitByte( &segment[CODESEG], OP_LEAVE );
		EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );

		return 1;
	}
	return 0;
}


ASM(ADDRESS)
{
	int		v;
	if ( !strcmp( token, "address" ) ) {
STAT("ADDRESS");
		Parse();
		v = ParseExpression();

/* Addresses are 32 bits wide, and therefore go into data segment. */
		HackToSegment( DATASEG );
		EmitInt( currentSegment, v );
		if( passNumber == 1 && token[ 0 ] == '$' ) // crude test for labels
			EmitInt( &segment[ JTRGSEG ], v );
		return 1;
	}
	return 0;
}

ASM(EXPORT)
{
	if ( !strcmp( token, "export" ) ) {
STAT("EXPORT");
		return 1;
	}
	return 0;
}

ASM(IMPORT)
{
	if ( !strcmp( token, "import" ) ) {
STAT("IMPORT");
		return 1;
	}
	return 0;
}

ASM(CODE)
{
	if ( !strcmp( token, "code" ) ) {
STAT("CODE");
		currentSegment = &segment[CODESEG];
		return 1;
	}
	return 0;
}

ASM(BSS)
{
	if ( !strcmp( token, "bss" ) ) {
STAT("BSS");
		currentSegment = &segment[BSSSEG];
		return 1;
	}
	return 0;
}

ASM(DATA)
{
	if ( !strcmp( token, "data" ) ) {
STAT("DATA");
		currentSegment = &segment[DATASEG];
		return 1;
	}
	return 0;
}

ASM(LIT)
{
	if ( !strcmp( token, "lit" ) ) {
STAT("LIT");
		currentSegment = &segment[LITSEG];
		return 1;
	}
	return 0;
}

ASM(LINE)
{
	if ( !strcmp( token, "line" ) ) {
STAT("LINE");
		return 1;
	}
	return 0;
}

ASM(FILE)
{
	if ( !strcmp( token, "file" ) ) {
STAT("FILE");
		return 1;
	}
	return 0;
}

ASM(EQU)
{
	char	name[1024];
	if ( !strcmp( token, "equ" ) ) {
STAT("EQU");
		Parse();
		strcpy( name, token );
		Parse();
		DefineSymbol( name, atoiNoCap(token) );
		return 1;
	}
	return 0;
}

ASM(ALIGN)
{
	int		v;
	if ( !strcmp( token, "align" ) ) {
STAT("ALIGN");
		v = ParseValue();
		currentSegment->imageUsed = (currentSegment->imageUsed + v - 1 ) & ~( v - 1 );
		return 1;
	}
	return 0;
}

ASM(SKIP)
{
	int		v;
	if ( !strcmp( token, "skip" ) ) {
STAT("SKIP");
		v = ParseValue();
		currentSegment->imageUsed += v;
		return 1;
	}
	return 0;
}

ASM(BYTE)
{
	int		i, v, v2;
	if ( !strcmp( token, "byte" ) ) {
STAT("BYTE");
		v = ParseValue();
		v2 = ParseValue();

		if ( v == 1 ) {
/* Character (1-byte) values go into lit(eral) segment. */
			HackToSegment( LITSEG );
		} else if ( v == 4 ) {
/* 32-bit (4-byte) values go into data segment. */
			HackToSegment( DATASEG );
		} else if ( v == 2 ) {
/* and 16-bit (2-byte) values will cause q3asm to barf. */
			CodeError( "16 bit initialized data not supported" );
		}

		// emit little endien
		for ( i = 0 ; i < v ; i++ ) {
			EmitByte( currentSegment, (v2 & 0xFF) ); /* paranoid ANDing  -PH */
			v2 >>= 8;
		}
		return 1;
	}
	return 0;
}

	// code labels are emited as instruction counts, not byte offsets,
	// because the physical size of the code will change with
	// different run time compilers and we want to minimize the
	// size of the required translation table
ASM(LABEL)
{
	if ( !strncmp( token, "LABEL", 5 ) ) {
STAT("LABEL");
		Parse();
		if ( currentSegment == &segment[CODESEG] ) {
			DefineSymbol( token, instructionCount );
		} else {
			DefineSymbol( token, currentSegment->imageUsed );
		}
		return 1;
	}
	return 0;
}



/*
==============
AssembleLine

==============
*/
static void AssembleLine( void ) {
	hashchain_t *hc;
	sourceOps_t *op;
	int		i;
	int		hash;

	Parse();
	if ( !token[0] ) {
		return;
	}

	hash = HashString( token );

/*
  Opcode search using hash table.
  Since the opcodes stays mostly fixed, this may benefit even more from a tree.
  Always with the tree :)
 -PH
*/
	for (hc = hashtable_get(optable, hash); hc; hc = hc->next) {
		op = (sourceOps_t*)(hc->data);
		i = op - sourceOps;
		if ((hash == opcodesHash[i]) && (!strcmp(token, op->name))) {
			int		opcode;
			int		expression;

			if ( op->opcode == OP_UNDEF ) {
				CodeError( "Undefined opcode: %s\n", token );
			}
			if ( op->opcode == OP_IGNORE ) {
				return;		// we ignore most conversions
			}

			// sign extensions need to check next parm
			opcode = op->opcode;
			if ( opcode == OP_SEX8 ) {
				Parse();
				if ( token[0] == '1' ) {
					opcode = OP_SEX8;
				} else if ( token[0] == '2' ) {
					opcode = OP_SEX16;
				} else {
					CodeError( "Bad sign extension: %s\n", token );
					return;
				}
			}

			// check for expression
			Parse();
			if ( token[0] && op->opcode != OP_CVIF
					&& op->opcode != OP_CVFI ) {
				expression = ParseExpression();

				// code like this can generate non-dword block copies:
				// auto char buf[2] = " ";
				// we are just going to round up.  This might conceivably
				// be incorrect if other initialized chars follow.
				if ( opcode == OP_BLOCK_COPY ) {
					expression = ( expression + 3 ) & ~3;
				}

				EmitByte( &segment[CODESEG], opcode );
				EmitInt( &segment[CODESEG], expression );
			} else {
				EmitByte( &segment[CODESEG], opcode );
			}

			instructionCount++;
			return;
		}
	}

/* This falls through if an assembly opcode is not found.  -PH */

/* The following should be sorted in sequence of statistical frequency, most frequent first.  -PH */
/*
Empirical frequency statistics from FI 2001.01.23:
 109892	STAT ADDRL
  72188	STAT BYTE
  51150	STAT LINE
  50906	STAT ARG
  43704	STAT IMPORT
  34902	STAT LABEL
  32066	STAT ADDRF
  23704	STAT CALL
   7720	STAT POP
   7256	STAT RET
   5198	STAT ALIGN
   3292	STAT EXPORT
   2878	STAT PROC
   2878	STAT ENDPROC
   2812	STAT ADDRESS
    738	STAT SKIP
    374	STAT EQU
    280	STAT CODE
    176	STAT LIT
    102	STAT FILE
    100	STAT BSS
     68	STAT DATA

 -PH
*/

#undef ASM
#define ASM(O) if (TryAssemble##O ()) return;

	ASM(ADDRL)
	ASM(BYTE)
	ASM(LINE)
	ASM(ARG)
	ASM(IMPORT)
	ASM(LABEL)
	ASM(ADDRF)
	ASM(CALL)
	ASM(POP)
	ASM(RET)
	ASM(ALIGN)
	ASM(EXPORT)
	ASM(PROC)
	ASM(ENDPROC)
	ASM(ADDRESS)
	ASM(SKIP)
	ASM(EQU)
	ASM(CODE)
	ASM(LIT)
	ASM(FILE)
	ASM(BSS)
	ASM(DATA)

	CodeError( "Unknown token: %s\n", token );
}

/*
==============
InitTables
==============
*/
void InitTables( void ) {
	int i;

	symtable = hashtable_new(symtablelen);
	optable = hashtable_new(100);  /* There's hardly 100 opcodes anyway. */

	for ( i = 0 ; i < NUM_SOURCE_OPS ; i++ ) {
		opcodesHash[i] = HashString( sourceOps[i].name );
		hashtable_add(optable, opcodesHash[i], sourceOps + i);
	}
}


/*
==============
WriteMapFile
==============
*/
static void WriteMapFile( void ) {
	FILE		*f;
	symbol_t	*s;
	char		imageName[MAX_OS_PATH];
	int			seg;

	strcpy( imageName, outputFilename );
	StripExtension( imageName );
	strcat( imageName, ".map" );

	report( "Writing %s...\n", imageName );

	f = SafeOpenWrite( imageName );
	for ( seg = CODESEG ; seg <= BSSSEG ; seg++ ) {
		for ( s = symbols ; s ; s = s->next ) {
			if ( s->name[0] == '$' ) {
				continue;	// skip locals
			}
			if ( &segment[seg] != s->segment ) {
				continue;
			}
			fprintf( f, "%i %8x %s\n", seg, s->value, s->name );
		}
	}
	fclose( f );
}

/*
===============
WriteVmFile
===============
*/
static void WriteVmFile( void ) {
	char	imageName[MAX_OS_PATH];
	vmHeader_t	header;
	FILE	*f;
	int		headerSize;

	report( "%i total errors\n", errorCount );

	strcpy( imageName, outputFilename );
	StripExtension( imageName );
	strcat( imageName, ".qvm" );

	remove( imageName );

	report( "code segment: %7i\n", segment[CODESEG].imageUsed );
	report( "data segment: %7i\n", segment[DATASEG].imageUsed );
	report( "lit  segment: %7i\n", segment[LITSEG].imageUsed );
	report( "bss  segment: %7i\n", segment[BSSSEG].imageUsed );
	report( "instruction count: %i\n", instructionCount );
  
	if ( errorCount != 0 ) {
		report( "Not writing a file due to errors\n" );
		return;
	}

	if( !options.vanillaQ3Compatibility ) {
		header.vmMagic = VM_MAGIC_VER2;
		headerSize = sizeof( header );
	} else {
		header.vmMagic = VM_MAGIC;

		// Don't write the VM_MAGIC_VER2 bits when maintaining 1.32b compatibility.
		// (I know this isn't strictly correct due to padding, but then platforms
		// that pad wouldn't be able to write a correct header anyway).  Note: if
		// vmHeader_t changes, this needs to be adjusted too.
		headerSize = sizeof( header ) - sizeof( header.jtrgLength );
	}

	header.instructionCount = instructionCount;
	header.codeOffset = headerSize;
	header.codeLength = segment[CODESEG].imageUsed;
	header.dataOffset = header.codeOffset + segment[CODESEG].imageUsed;
	header.dataLength = segment[DATASEG].imageUsed;
	header.litLength = segment[LITSEG].imageUsed;
	header.bssLength = segment[BSSSEG].imageUsed;
	header.jtrgLength = segment[JTRGSEG].imageUsed;

	report( "Writing to %s\n", imageName );

#ifdef Q3_BIG_ENDIAN
	{
		int i;

		// byte swap the header
		for ( i = 0 ; i < sizeof( vmHeader_t ) / 4 ; i++ ) {
			((int *)&header)[i] = LittleLong( ((int *)&header)[i] );
		}
	}
#endif

	CreatePath( imageName );
	f = SafeOpenWrite( imageName );
	SafeWrite( f, &header, headerSize );
	SafeWrite( f, &segment[CODESEG].image, segment[CODESEG].imageUsed );
	SafeWrite( f, &segment[DATASEG].image, segment[DATASEG].imageUsed );
	SafeWrite( f, &segment[LITSEG].image, segment[LITSEG].imageUsed );

	if( !options.vanillaQ3Compatibility ) {
		SafeWrite( f, &segment[JTRGSEG].image, segment[JTRGSEG].imageUsed );
	}

	fclose( f );
}

/*
===============
Assemble
===============
*/
static void Assemble( void ) {
	int		i;
	char	filename[MAX_OS_PATH];
	char		*ptr;

	report( "outputFilename: %s\n", outputFilename );

	for ( i = 0 ; i < numAsmFiles ; i++ ) {
		strcpy( filename, asmFileNames[ i ] );
		DefaultExtension( filename, ".asm" );
		LoadFile( filename, (void **)&asmFiles[i] );
	}

	// assemble
	for ( passNumber = 0 ; passNumber < 2 ; passNumber++ ) {
		segment[LITSEG].segmentBase = segment[DATASEG].imageUsed;
		segment[BSSSEG].segmentBase = segment[LITSEG].segmentBase + segment[LITSEG].imageUsed;
		segment[JTRGSEG].segmentBase = segment[BSSSEG].segmentBase + segment[BSSSEG].imageUsed;
		for ( i = 0 ; i < NUM_SEGMENTS ; i++ ) {
			segment[i].imageUsed = 0;
		}
		segment[DATASEG].imageUsed = 4;		// skip the 0 byte, so NULL pointers are fixed up properly
		instructionCount = 0;

		for ( i = 0 ; i < numAsmFiles ; i++ ) {
			currentFileIndex = i;
			currentFileName = asmFileNames[ i ];
			currentFileLine = 0;
			report("pass %i: %s\n", passNumber, currentFileName );
			fflush( NULL );
			ptr = asmFiles[i];
			while ( ptr ) {
				ptr = ExtractLine( ptr );
				AssembleLine();
			}
		}

		// align all segment
		for ( i = 0 ; i < NUM_SEGMENTS ; i++ ) {
			segment[i].imageUsed = (segment[i].imageUsed + 3) & ~3;
		}
		if (passNumber == 0) {
			sort_symbols();
		}
	}

	// reserve the stack in bss
	DefineSymbol( "_stackStart", segment[BSSSEG].imageUsed );
	segment[BSSSEG].imageUsed += stackSize;
	DefineSymbol( "_stackEnd", segment[BSSSEG].imageUsed );

	// write the image
	WriteVmFile();

	// write the map file even if there were errors
	if( options.writeMapFile ) {
		WriteMapFile();
	}
}


/*
=============
ParseOptionFile

=============
*/
static void ParseOptionFile( const char *filename ) {
	char		expanded[MAX_OS_PATH];
	char		*text, *text_p;

	strcpy( expanded, filename );
	DefaultExtension( expanded, ".q3asm" );
	LoadFile( expanded, (void **)&text );
	if ( !text ) {
		return;
	}

	text_p = text;

	while( ( text_p = COM_Parse( text_p ) ) != 0 ) {
		if ( !strcmp( com_token, "-o" ) ) {
			// allow output override in option file
			text_p = COM_Parse( text_p );
			if ( text_p ) {
				strcpy( outputFilename, com_token );
			}
			continue;
		}

		asmFileNames[ numAsmFiles ] = copystring( com_token );
		numAsmFiles++;
	}
}

static void ShowHelp( char *argv0 ) {
	Error("Usage: %s [OPTION]... [FILES]...\n\
Assemble LCC bytecode assembly to Q3VM bytecode.\n\
\n\
  -o OUTPUT      Write assembled output to file OUTPUT.qvm\n\
  -f LISTFILE    Read options and list of files to assemble from LISTFILE.q3asm\n\
  -b BUCKETS     Set symbol hash table to BUCKETS buckets\n\
  -m             Generate a mapfile for each OUTPUT.qvm\n\
  -v             Verbose compilation report\n\
  -vq3           Produce a qvm file compatible with Q3 1.32b\n\
  -h --help -?   Show this help\n\
", argv0);
}

/*
==============
main
==============
*/
int main( int argc, char **argv ) {
	int			i;
	double		start, end;

//	_chdir( "/quake3/jccode/cgame/lccout" );	// hack for vc profiler

	if ( argc < 2 ) {
		ShowHelp( argv[0] );
	}

	start = I_FloatTime ();

	// default filename is "q3asm"
	strcpy( outputFilename, "q3asm" );
	numAsmFiles = 0;	

	for ( i = 1 ; i < argc ; i++ ) {
		if ( argv[i][0] != '-' ) {
			break;
		}
		if( !strcmp( argv[ i ], "-h" ) || 
		    !strcmp( argv[ i ], "--help" ) ||
		    !strcmp( argv[ i ], "-?") ) {
			ShowHelp( argv[0] );
		}

		if ( !strcmp( argv[i], "-o" ) ) {
			if ( i == argc - 1 ) {
				Error( "-o must preceed a filename" );
			}
/* Timbo of Tremulous pointed out -o not working; stock ID q3asm folded in the change. Yay. */
			strcpy( outputFilename, argv[ i+1 ] );
			i++;
			continue;
		}

		if ( !strcmp( argv[i], "-f" ) ) {
			if ( i == argc - 1 ) {
				Error( "-f must preceed a filename" );
			}
			ParseOptionFile( argv[ i+1 ] );
			i++;
			continue;
		}

		if (!strcmp(argv[i], "-b")) {
			if (i == argc - 1) {
				Error("-b requires an argument");
			}
			i++;
			symtablelen = atoiNoCap(argv[i]);
			continue;
		}

		if( !strcmp( argv[ i ], "-v" ) ) {
/* Verbosity option added by Timbo, 2002.09.14.
By default (no -v option), q3asm remains silent except for critical errors.
Verbosity turns on all messages, error or not.
Motivation: not wanting to scrollback for pages to find asm error.
*/
			options.verbose = qtrue;
			continue;
		}

		if( !strcmp( argv[ i ], "-m" ) ) {
			options.writeMapFile = qtrue;
			continue;
		}

		if( !strcmp( argv[ i ], "-vq3" ) ) {
			options.vanillaQ3Compatibility = qtrue;
			continue;
		}

		Error( "Unknown option: %s", argv[i] );
	}

	// the rest of the command line args are asm files
	for ( ; i < argc ; i++ ) {
		asmFileNames[ numAsmFiles ] = copystring( argv[ i ] );
		numAsmFiles++;
	}
	// In some case it Segfault without this check
	if ( numAsmFiles == 0 ) {
		Error( "No file to assemble\n" );
	}

	InitTables();
	Assemble();

	{
		symbol_t *s;

		for ( i = 0, s = symbols ; s ; s = s->next, i++ ) /* nop */ ;

		if (options.verbose)
		{
			report("%d symbols defined\n", i);
			hashtable_stats(symtable);
			hashtable_stats(optable);
		}
	}

	end = I_FloatTime ();
	report ("%5.0f seconds elapsed\n", end-start);

	return errorCount;
}

