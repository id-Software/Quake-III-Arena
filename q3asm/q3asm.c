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

#include "cmdlib.h"
#include "mathlib.h"
#include "qfiles.h"

/* MSVC-ism fix. */
#define atoi(s) strtoul(s,NULL,10)

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


segment_t	segment[NUM_SEGMENTS];
segment_t	*currentSegment;

int		passNumber;

int		numSymbols;
int		errorCount;

symbol_t	*symbols;
symbol_t	*lastSymbol;


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


/*
=============
HashString
=============
*/
int	HashString( char *s ) {
	int		v = 0;

	while ( *s ) {
		v += *s;
		s++;
	}
	return v;
}


/*
============
CodeError
============
*/
void CodeError( char *fmt, ... ) {
	va_list		argptr;

	errorCount++;

	printf( "%s:%i ", currentFileName, currentFileLine );

	va_start( argptr,fmt );
	vprintf( fmt,argptr );
	va_end( argptr );
}

/*
============
EmitByte
============
*/
void EmitByte( segment_t *seg, int v ) {
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
void EmitInt( segment_t *seg, int v ) {
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
void DefineSymbol( char *sym, int value ) {
	symbol_t	*s, *after;
	char		expanded[MAX_LINE_LENGTH];
	int			hash;

	if ( passNumber == 1 ) {
		return;
	}
  
  // TTimo
  // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=381
  // as a security, bail out if vmMain entry point is not first
  if (!Q_stricmp(sym, "vmMain"))
    if (value)
      Error( "vmMain must be the first symbol in the qvm (got offset %d)\n", value );

	// add the file prefix to local symbols to guarantee unique
	if ( sym[0] == '$' ) {
		sprintf( expanded, "%s_%i", sym, currentFileIndex );
		sym = expanded;
	}

	hash = HashString( sym );

	for ( s = symbols ; s ; s = s->next ) {
		if ( hash == s->hash && !strcmp( sym, s->name ) ) {
			CodeError( "Multiple definitions for %s\n", sym );
			return;
		}
	}

	s = malloc( sizeof( *s ) );
	s->name = copystring( sym );
	s->hash = hash;
	s->value = value;
	s->segment = currentSegment;

	lastSymbol = s;	/* for the move-to-lit-segment byteswap hack */

	// insert it in order
	if ( !symbols || s->value < symbols->value ) {
		s->next = symbols;
		symbols = s;
		return;
	}

	for ( after = symbols ; after->next && after->next->value < value ; after = after->next ) {
	}
	s->next = after->next;
	after->next = s;
}


/*
============
LookupSymbol

Symbols can only be evaluated on pass 1
============
*/
int LookupSymbol( char *sym ) {
	symbol_t	*s;
	char		expanded[MAX_LINE_LENGTH];
	int			hash;

	if ( passNumber == 0 ) {
		return 0;
	}

	// add the file prefix to local symbols to guarantee unique
	if ( sym[0] == '$' ) {
		sprintf( expanded, "%s_%i", sym, currentFileIndex );
		sym = expanded;
	}

	hash = HashString( sym );
	for ( s = symbols ; s ; s = s->next ) {
		if ( hash == s->hash && !strcmp( sym, s->name ) ) {
			return s->segment->segmentBase + s->value;
		}
	}

	CodeError( "ERROR: symbol %s undefined\n", sym );
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
char *ExtractLine( char *data ) {
	int			i;

	currentFileLine++;
	lineParseOffset = 0;
	token[0] = 0;

	if ( data[0] == 0 ) {
		lineBuffer[0] = 0;
		return NULL;
	}

	for ( i = 0 ; i < MAX_LINE_LENGTH ; i++ ) {
		if ( data[i] == 0 || data[i] == '\n' ) {
			break;
		}
	}
	if ( i == MAX_LINE_LENGTH ) {
		CodeError( "MAX_LINE_LENGTH" );
		return data;
	}
	memcpy( lineBuffer, data, i );
	lineBuffer[i] = 0;
	data += i;
	if ( data[0] == '\n' ) {
		data++;
	}
	return data;
}


/*
==============
Parse

Parse a token out of linebuffer
==============
*/
qboolean Parse( void ) {
	int		c;
	int		len;
	
	len = 0;
	token[0] = 0;
	
	// skip whitespace
	while ( lineBuffer[ lineParseOffset ] <= ' ' ) {
		if ( lineBuffer[ lineParseOffset ] == 0 ) {
			return qfalse;
		}
		lineParseOffset++;
	}

	// skip ; comments
	c = lineBuffer[ lineParseOffset ];
	if ( c == ';' ) {
		return qfalse;
	}
	

	// parse a regular word
	do {
		token[len] = c;
		len++;
		lineParseOffset++;
		c = lineBuffer[ lineParseOffset ];
	} while (c>32);
	
	token[len] = 0;
	return qtrue;
}


/*
==============
ParseValue
==============
*/
int	ParseValue( void ) {
	Parse();
	return atoi( token );
}


/*
==============
ParseExpression
==============
*/
int	ParseExpression(void) {
	int		i, j;
	char	sym[MAX_LINE_LENGTH];
	int		v;

	if ( token[0] == '-' ) {
		i = 1;
	} else {
		i = 0;
	}

	for ( ; i < MAX_LINE_LENGTH ; i++ ) {
		if ( token[i] == '+' || token[i] == '-' || token[i] == 0 ) {
			break;
		}
	}

	memcpy( sym, token, i );
	sym[i] = 0;

	if ( ( sym[0] >= '0' && sym[0] <= '9' ) || sym[0] == '-' ) {
		v = atoi( sym );
	} else {
		v = LookupSymbol( sym );
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

		if ( token[i] == '+' ) {
			v += atoi( sym );
		}
		if ( token[i] == '-' ) {
			v -= atoi( sym );
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
void HackToSegment( segmentName_t seg ) {
	if ( currentSegment == &segment[seg] ) {
		return;
	}

	currentSegment = &segment[seg];
	if ( passNumber == 0 ) {
		lastSymbol->segment = currentSegment;
		lastSymbol->value = currentSegment->imageUsed;
	}
}

/*
==============
AssembleLine

==============
*/
void AssembleLine( void ) {
	int		v, v2;
	int		i;
	int		hash;

	Parse();
	if ( !token[0] ) {
		return;
	}

	hash = HashString( token );

	for ( i = 0 ; i < NUM_SOURCE_OPS ; i++ ) {
		if ( hash == opcodesHash[i] && !strcmp( token, sourceOps[i].name ) ) {
			int		opcode;
			int		expression;

			if ( sourceOps[i].opcode == OP_UNDEF ) {
				CodeError( "Undefined opcode: %s\n", token );
			}
			if ( sourceOps[i].opcode == OP_IGNORE ) {
				return;		// we ignore most conversions
			}

			// sign extensions need to check next parm
			opcode = sourceOps[i].opcode;
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
			if ( token[0] && sourceOps[i].opcode != OP_CVIF
					&& sourceOps[i].opcode != OP_CVFI ) {
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

	// call instructions reset currentArgOffset
	if ( !strncmp( token, "CALL", 4 ) ) {
		EmitByte( &segment[CODESEG], OP_CALL );
		instructionCount++;
		currentArgOffset = 0;
		return;
	}

	// arg is converted to a reversed store
	if ( !strncmp( token, "ARG", 3 ) ) {
		EmitByte( &segment[CODESEG], OP_ARG );
		instructionCount++;
		if ( 8 + currentArgOffset >= 256 ) {
			CodeError( "currentArgOffset >= 256" );
			return;
		}
		EmitByte( &segment[CODESEG], 8 + currentArgOffset );
		currentArgOffset += 4;
		return;
	}

	// ret just leaves something on the op stack
	if ( !strncmp( token, "RET", 3 ) ) {
		EmitByte( &segment[CODESEG], OP_LEAVE );
		instructionCount++;
		EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );
		return;
	}

	// pop is needed to discard the return value of 
	// a function
	if ( !strncmp( token, "pop", 3 ) ) {
		EmitByte( &segment[CODESEG], OP_POP );
		instructionCount++;
		return;
	}

	// address of a parameter is converted to OP_LOCAL
	if ( !strncmp( token, "ADDRF", 5 ) ) {
		instructionCount++;
		Parse();
		v = ParseExpression();
		v = 16 + currentArgs + currentLocals + v;
		EmitByte( &segment[CODESEG], OP_LOCAL );
		EmitInt( &segment[CODESEG], v );
		return;
	}

	// address of a local is converted to OP_LOCAL
	if ( !strncmp( token, "ADDRL", 5 ) ) {
		instructionCount++;
		Parse();
		v = ParseExpression();
		v = 8 + currentArgs + v;
		EmitByte( &segment[CODESEG], OP_LOCAL );
		EmitInt( &segment[CODESEG], v );
		return;
	}

	if ( !strcmp( token, "proc" ) ) {
		char	name[1024];

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
		return;
	}
	if ( !strcmp( token, "endproc" ) ) {
		Parse();				// skip the function name
		v = ParseValue();		// locals
		v2 = ParseValue();		// arg marshalling

		// all functions must leave something on the opstack
		instructionCount++;
		EmitByte( &segment[CODESEG], OP_PUSH );

		instructionCount++;
		EmitByte( &segment[CODESEG], OP_LEAVE );
		EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );

		return;
	}


	if ( !strcmp( token, "address" ) ) {
		Parse();
		v = ParseExpression();

		HackToSegment( DATASEG );
		EmitInt( currentSegment, v );
		return;
	}
	if ( !strcmp( token, "export" ) ) {
		return;
	}
	if ( !strcmp( token, "import" ) ) {
		return;
	}
	if ( !strcmp( token, "code" ) ) {
		currentSegment = &segment[CODESEG];
		return;
	}
	if ( !strcmp( token, "bss" ) ) {
		currentSegment = &segment[BSSSEG];
		return;
	}
	if ( !strcmp( token, "data" ) ) {
		currentSegment = &segment[DATASEG];
		return;
	}
	if ( !strcmp( token, "lit" ) ) {
		currentSegment = &segment[LITSEG];
		return;
	}
	if ( !strcmp( token, "line" ) ) {
		return;
	}
	if ( !strcmp( token, "file" ) ) {
		return;
	}

	if ( !strcmp( token, "equ" ) ) {
		char	name[1024];

		Parse();
		strcpy( name, token );
		Parse();
		DefineSymbol( name, atoi(token) );
		return;
	}

	if ( !strcmp( token, "align" ) ) {
		v = ParseValue();
		currentSegment->imageUsed = (currentSegment->imageUsed + v - 1 ) & ~( v - 1 );
		return;
	}

	if ( !strcmp( token, "skip" ) ) {
		v = ParseValue();
		currentSegment->imageUsed += v;
		return;
	}

	if ( !strcmp( token, "byte" ) ) {
		v = ParseValue();
		v2 = ParseValue();

		if ( v == 1 ) {
			HackToSegment( LITSEG );
		} else if ( v == 4 ) {
			HackToSegment( DATASEG );
		} else if ( v == 2 ) {
			CodeError( "16 bit initialized data not supported" );
		}

		// emit little endien
		for ( i = 0 ; i < v ; i++ ) {
			EmitByte( currentSegment, v2 );
			v2 >>= 8;
		}
		return;
	}

	// code labels are emited as instruction counts, not byte offsets,
	// because the physical size of the code will change with
	// different run time compilers and we want to minimize the
	// size of the required translation table
	if ( !strncmp( token, "LABEL", 5 ) ) {
		Parse();
		if ( currentSegment == &segment[CODESEG] ) {
			DefineSymbol( token, instructionCount );
		} else {
			DefineSymbol( token, currentSegment->imageUsed );
		}
		return;
	}

	CodeError( "Unknown token: %s\n", token );
}

/*
==============
InitTables
==============
*/
void InitTables( void ) {
	int		i;

	for ( i = 0 ; i < NUM_SOURCE_OPS ; i++ ) {
		opcodesHash[i] = HashString( sourceOps[i].name );
	}
}


/*
==============
WriteMapFile
==============
*/
void WriteMapFile( void ) {
	FILE		*f;
	symbol_t	*s;
	char		imageName[MAX_OS_PATH];
	int			seg;

	strcpy( imageName, outputFilename );
	StripExtension( imageName );
	strcat( imageName, ".map" );

	printf( "Writing %s...\n", imageName );
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
void WriteVmFile( void ) {
	char	imageName[MAX_OS_PATH];
	vmHeader_t	header;
	FILE	*f;

	printf( "%i total errors\n", errorCount );
	strcpy( imageName, outputFilename );
	StripExtension( imageName );
	strcat( imageName, ".qvm" );

	remove( imageName );

	printf( "code segment: %7i\n", segment[CODESEG].imageUsed );
	printf( "data segment: %7i\n", segment[DATASEG].imageUsed );
	printf( "lit  segment: %7i\n", segment[LITSEG].imageUsed );
	printf( "bss  segment: %7i\n", segment[BSSSEG].imageUsed );
	printf( "instruction count: %i\n", instructionCount );
	if ( errorCount != 0 ) {
		printf( "Not writing a file due to errors\n" );
		return;
	}

	header.vmMagic = VM_MAGIC;
	header.instructionCount = instructionCount;
	header.codeOffset = sizeof( header );
	header.codeLength = segment[CODESEG].imageUsed;
	header.dataOffset = header.codeOffset + segment[CODESEG].imageUsed;
	header.dataLength = segment[DATASEG].imageUsed;
	header.litLength = segment[LITSEG].imageUsed;
	header.bssLength = segment[BSSSEG].imageUsed;

	printf( "Writing to %s\n", imageName );

	CreatePath( imageName );
	f = SafeOpenWrite( imageName );
	SafeWrite( f, &header, sizeof( header ) );
	SafeWrite( f, &segment[CODESEG].image, segment[CODESEG].imageUsed );
	SafeWrite( f, &segment[DATASEG].image, segment[DATASEG].imageUsed );
	SafeWrite( f, &segment[LITSEG].image, segment[LITSEG].imageUsed );
	fclose( f );
}

/*
===============
Assemble
===============
*/
void Assemble( void ) {
	int		i;
	char	filename[MAX_OS_PATH];
	char		*ptr;

	printf( "outputFilename: %s\n", outputFilename );

	for ( i = 0 ; i < numAsmFiles ; i++ ) {
		strcpy( filename, asmFileNames[ i ] );
		DefaultExtension( filename, ".asm" );
		LoadFile( filename, (void **)&asmFiles[i] );
	}

	// assemble
	for ( passNumber = 0 ; passNumber < 2 ; passNumber++ ) {
		segment[LITSEG].segmentBase = segment[DATASEG].imageUsed;
		segment[BSSSEG].segmentBase = segment[LITSEG].segmentBase + segment[LITSEG].imageUsed;
		for ( i = 0 ; i < NUM_SEGMENTS ; i++ ) {
			segment[i].imageUsed = 0;
		}
		segment[DATASEG].imageUsed = 4;		// skip the 0 byte, so NULL pointers are fixed up properly
		instructionCount = 0;

		for ( i = 0 ; i < numAsmFiles ; i++ ) {
			currentFileIndex = i;
			currentFileName = asmFileNames[ i ];
			currentFileLine = 0;
			printf("pass %i: %s\n", passNumber, currentFileName );
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
	}

	// reserve the stack in bss
	DefineSymbol( "_stackStart", segment[BSSSEG].imageUsed );
	segment[BSSSEG].imageUsed += stackSize;
	DefineSymbol( "_stackEnd", segment[BSSSEG].imageUsed );

	// write the image
	WriteVmFile();

	// write the map file even if there were errors
	WriteMapFile();
}


/*
=============
ParseOptionFile

=============
*/
void ParseOptionFile( const char *filename ) {
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
		Error( "usage: q3asm [-o output] <files> or q3asm -f <listfile>\n" );
	}

	start = I_FloatTime ();
	InitTables();

	// default filename is "q3asm"
	strcpy( outputFilename, "q3asm" );
	numAsmFiles = 0;	

	for ( i = 1 ; i < argc ; i++ ) {
		if ( argv[i][0] != '-' ) {
			break;
		}
		if ( !strcmp( argv[i], "-o" ) ) {
			if ( i == argc - 1 ) {
				Error( "-o must preceed a filename" );
			}
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
		Error( "Unknown option: %s", argv[i] );
	}

	// the rest of the command line args are asm files
	for ( ; i < argc ; i++ ) {
		asmFileNames[ numAsmFiles ] = copystring( argv[ i ] );
		numAsmFiles++;
	}

	Assemble();

	end = I_FloatTime ();
	printf ("%5.0f seconds elapsed\n", end-start);

	return 0;
}

