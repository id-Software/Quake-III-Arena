   struct defs {
     int cbits;          /* No. of bits per char           */
     int ibits;          /*                 int            */
     int sbits;          /*                 short          */
     int lbits;          /*                 long           */
     int ubits;          /*                 unsigned       */
     int fbits;          /*                 float          */
     int dbits;          /*                 double         */
     float fprec;        /* Smallest number that can be    */
     float dprec;        /* significantly added to 1.      */
     int flgs;           /* Print return codes, by section */
     int flgm;           /* Announce machine dependencies  */
     int flgd;           /* give explicit diagnostics      */
     int flgl;           /* Report local return codes.     */
     int rrc;            /* recent return code             */
     int crc;            /* Cumulative return code         */
     char rfs[8];        /* Return from section            */
   };
main(n,args)               /* C REFERENCE MANUAL         */
int n;
char **args;
{

/*   This program performs a series of tests on a C compiler,
based on information in the

             C REFERENCE MANUAL

which appears as Appendix A to the book "The C Programming
Language" by Brian W. Kernighan and Dennis M. Ritchie
(Prentice-Hall, 1978, $10.95). This Appendix is hereafter
referred to as "the Manual".

     The rules followed in writing this program are:

     1. The entire program is written in legal C, according
     to the Manual. It should compile with no error messages,
     although some warning messages may be produced by some
     compilers. Failure to compile should be interpreted as
     a compiler error.

     2. The program is clean, in that it does not make use
     of any features of the operating system on which it runs,
     with the sole exceptions of the printf() function, and an
     internal "options" routine, which is easily excised.

     3. No global variables are used, except for the spec-
     ific purpose of testing the global variable facility.

     The program is divided into modules having names of the
form snnn... These modules correspond to those sections of the
Manual, as identified by boldface type headings, in which
there is something to test. For example, s241() corresponds
to section 2.4.1 of the Manual (Integer constants) and tests
the facilities described therein. The module numbering
scheme is ambiguous, especially when it names modules
referring to more than one section; module s7813, for ex-
ample, deals with sections 7.8 through 7.13. Nonetheless,
it is surprisingly easy to find a section in the Manual
corresponding to a section of code, and vice versa.

     Note also that there seem to be "holes" in the program,
at least from the point of view that there exist sections in the
Manual for which there is no corresponding code. Such holes
arise from three causes: (a) there is nothing in that partic-
ular section to test, (b) everything in that section is tested
elsewhere, and (c) it was deemed advisable not to check cer-
tain features like preprocessor or listing control features.

     Modules are called by a main program main(). The mod-
ules that are called, and the sequence in which they are 
called, are determined by two lists in main(), in which the
module names appear. The first list (an extern statement)
declares the module names to be external. The second (a stat-
ic int statement) names the modules and defines the sequence
in which they are called. There is no need for these lists
to be in the same order, but it is probably a good idea to keep
them that way in the interest of clarity. Since there are no
cross-linkages between modules, new modules may be added,
or old ones deleted, simply by editing the lists, with one
exception: section s26, which pokes around at the hardware
trying to figure out the characteristics of the machine that
it is running on, saves information that is subsequently
used by sections s626, s72, and s757. If this program is
to be broken up into smallish pieces, say for running on
a microcomputer, take care to see that s26 is called before
calling any of the latter three sections.  The size
of the lists, i.e., the number of modules to be called, is
not explicitly specified as a program parameter, but is
determined dynamically using the sizeof operator.

     Communication between the main program and the modules
takes place in two ways. In all cases, a pointer to a structure
is passed to the called module. The structure contains flags
that will determine the type of information to be published
by the module, and fields that may be written in by the
module. The former include "flgm" and "flgd", which, if set
to a nonzero value, specify that machine dependencies are to
be announced or that error messages are to be printed, re-
spectively. The called module's name, and the hardware char-
acteristics probed in s26() comprise the latter.


     Also, in all cases, a return code is returned by the called
module. A return code of zero indicates that all has gone well;
nonzero indicates otherwise. Since more than one type of error
may be detected by a module, the return code is a composite
of error indicators, which, individually, are given as numbers
that are powers of two. Thus, a return code of 10 indicates
that two specific errors, 8 and 2, were detected. Whether or
not the codes returned by the modules are printed by the main
program is determined by setting "flgs" to 1 (resp. 0).

     The entire logic of the main program is contained in the
half-dozen or so lines at the end. The somewhat cryptic 
statement:

           d0.rrc = (*sec[j])(pd0);

in the for loop calls the modules. The rest of the code is
reasonably straightforward.

     Finally, in each of the modules, there is the following
prologue:

           snnn(pd0)
           struct defs *pd0;
           {
              static char snnner[] = "snnn,er%d\n";
              static char qsnnn[8] = "snnn   ";
              char *ps, *pt;
              int rc;

              rc = 0;
              ps = qsnnn;
              pt = pd0->rfs;
              while(*pt++ = *ps++);

used for housekeeping, handshaking and module initialization.

                                                           */
   extern
     s22(),
     s241(),
     s243(),
     s244(),
     s25(),
     s26(),
     s4(),
     s61(),
     s626(),
     s71(),
     s72(),
     s757(),
     s7813(),
     s714(),
     s715(),
     s81(),
     s84(),
     s85(),
     s86(),
     s88(),
     s9()
   ;

   int j;
   static int (*sec[])() = {
     s22,
     s241,
     s243,
     s244,
     s25,
     s26,
     s4,
     s61,
     s626,
     s71,
     s72,
     s757,
     s7813,
     s714,
     s715,
     s81,
     s84,
     s85,
     s86,
     s88,
     s9
   };

   static struct defs d0, *pd0;
    
     d0.flgs = 1;          /* These flags dictate            */
     d0.flgm = 1;          /*     the verbosity of           */
     d0.flgd = 1;          /*         the program.           */
     d0.flgl = 1;

   pd0 = &d0;

   for (j=0; j<sizeof(sec) / sizeof(sec[0]); j++) {
     d0.rrc = (*sec[j])(pd0);
     d0.crc = d0.crc+d0.rrc;
     if(d0.flgs != 0) printf("Section %s returned %d.\n",d0.rfs,d0.rrc);
   }
  
   if(d0.crc == 0) printf("\nNo errors detected.\n");
   else printf("\nFailed.\n");
   return 0;
}
s22(pd0)                 /* 2.2 Identifiers (Names)      */
struct defs *pd0;
{
   int a234, a;
   int _, _234, A, rc;

   static char s22er[] = "s22,er%d\n";
   static char qs22[8] = "s22    ";

   char *ps, *pt;
                         /* Initialize                      */

   rc = 0;
   ps = qs22;
   pt = pd0 -> rfs;
   while (*pt++ = *ps++);

     /* An identifier is a sequence of letters and digits;
        the first character must be a letter. The under-
        score _ counts as a letter.                        */

   a=1;
   _=2;
   _234=3;
   a234=4;
   if(a+_+_234+a234 != 10) {
     rc = rc+1;
     if(pd0->flgd != 0) printf(s22er,1);
   }

   /* Upper and lower case letters are different.     */

   A = 2;
   if (A == a) {
     rc = rc+4;
     if (pd0->flgd != 0) printf(s22er,4);
   }

   return(rc);
}
s241(pd0)                   /* 2.4.1 Integer constants
                               2.4.2 Explicit long constants  */
struct defs *pd0;
{
   long pow2();
   static char s241er[] = "s241,er%d\n";
   static char qs241[8] = "s241   ";
   char *ps, *pt;
   int rc, j, lrc;
   static long g[39] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                        0,6,0,8,0,12,0,16,0,18,0,20,0,24,
                        0,28,0,30,0,32,0,36};
   long d[39], o[39], x[39];

   rc = 0;
   lrc = 0;
   ps = qs241;
   pt = pd0 -> rfs;
   while (*pt++ = *ps++);

     /* An integer constant consisting of a sequence of digits is
        taken to be octal if it begins with 0 (digit zero), decimal
        otherwise.                                            */

   if (   8 !=  010
     ||  16 !=  020
     ||  24 !=  030
     ||  32 !=  040
     ||  40 !=  050
     ||  48 !=  060
     ||  56 !=  070
     ||  64 != 0100
     ||  72 != 0110
     ||  80 != 0120
     ||   9 != 0011
     ||  17 != 0021
     ||  25 != 0031
     ||  33 != 0041
     ||  41 != 0051
     ||  49 != 0061
     ||  57 != 0071
     ||  65 != 0101
     ||  73 != 0111
     ||  81 != 0121 ){

     rc = rc+1;
     if( pd0->flgd != 0 ) printf(s241er,1);
   }

     /* A sequence of digits preceded by 0x or 0X (digit zero)
        is taken to be a hexadecimal integer. The hexadecimal
        digits include a or A through f or F with values 10
        through 15.     */

   if ( 0x00abcdef != 0xabcdef
     || 0xabcdef != 0Xabcdef || 0Xabcdef != 0XAbcdef
     || 0XAbcdef != 0XABcdef || 0XABcdef != 0XABCdef
     || 0XABCdef != 0XABCDef || 0XABCDef != 0XABCDEf
     || 0XABCDEf != 0XABCDEF || 0xABCDEF != 11259375 ){

     rc = rc+2;
     if( pd0->flgd != 0 ) printf(s241er,2);
   }

     /* A decimal constant whose value exceeds the largest signed
        machine integer is taken to be long; an octal or hex con-
        stant which exceeds the largest unsigned machine integer
        is likewise taken to be long.     */

   if ( sizeof 010000000000 != sizeof(long)      /* 2**30 */
     || sizeof 1073741824   != sizeof(long)      /* ditto */
     || sizeof 0x40000000   != sizeof(long) ){   /*   "   */

     rc = rc+4;
     if( pd0->flgd != 0 ) printf(s241er,4);
   }

     /* A decimal, octal, or hexadecimal constant immediately followed
        by l (letter ell) or L is a long constant.    */

   if ( sizeof   67l != sizeof(long)
     || sizeof   67L != sizeof(long)
     || sizeof  067l != sizeof(long)
     || sizeof  067L != sizeof(long)
     || sizeof 0X67l != sizeof(long)
     || sizeof 0x67L != sizeof(long) ){

     rc = rc+8;
     if( pd0 -> flgd != 0 ) printf(s241er,8);
   }

     /* Finally, we test to see that decimal (d), octal (o),
        and hexadecimal (x) constants representing the same values
        agree among themselves, and with computed values, at spec-
        ified points over an appropriate range. The points select-
        ed here are those with the greatest potential for caus-
        ing trouble, i.e., zero, 1-16, and values of 2**n and
        2**n - 1 where n is some multiple of 4 or 6. Unfortunately,
        just what happens when a value is too big to fit in a
        long is undefined; however, it would be nice if what
        happened were at least consistent...      */

   for ( j=0; j<17; j++ ) g[j] = j;
   for ( j=18; j<39; ) {
     g[j] = pow2(g[j]);
     g[j-1] = g[j] - 1;
     j = j+2;
   }

   d[0] = 0;                o[0] = 00;               x[0] = 0x0;
   d[1] = 1;                o[1] = 01;               x[1] = 0x1;
   d[2] = 2;                o[2] = 02;               x[2] = 0x2;
   d[3] = 3;                o[3] = 03;               x[3] = 0x3;
   d[4] = 4;                o[4] = 04;               x[4] = 0x4;
   d[5] = 5;                o[5] = 05;               x[5] = 0x5;
   d[6] = 6;                o[6] = 06;               x[6] = 0x6;
   d[7] = 7;                o[7] = 07;               x[7] = 0x7;
   d[8] = 8;                o[8] = 010;              x[8] = 0x8;
   d[9] = 9;                o[9] = 011;              x[9] = 0x9;
   d[10] = 10;              o[10] = 012;             x[10] = 0xa;
   d[11] = 11;              o[11] = 013;             x[11] = 0xb;
   d[12] = 12;              o[12] = 014;             x[12] = 0xc;
   d[13] = 13;              o[13] = 015;             x[13] = 0xd;
   d[14] = 14;              o[14] = 016;             x[14] = 0xe;
   d[15] = 15;              o[15] = 017;             x[15] = 0xf;
   d[16] = 16;              o[16] = 020;             x[16] = 0x10;
   d[17] = 63;              o[17] = 077;             x[17] = 0x3f;
   d[18] = 64;              o[18] = 0100;            x[18] = 0x40;
   d[19] = 255;             o[19] = 0377;            x[19] = 0xff;
   d[20] = 256;             o[20] = 0400;            x[20] = 0x100;
   d[21] = 4095;            o[21] = 07777;           x[21] = 0xfff;
   d[22] = 4096;            o[22] = 010000;          x[22] = 0x1000;
   d[23] = 65535;           o[23] = 0177777;         x[23] = 0xffff;
   d[24] = 65536;           o[24] = 0200000;         x[24] = 0x10000;
   d[25] = 262143;          o[25] = 0777777;         x[25] = 0x3ffff;
   d[26] = 262144;          o[26] = 01000000;        x[26] = 0x40000;
   d[27] = 1048575;         o[27] = 03777777;        x[27] = 0xfffff;
   d[28] = 1048576;         o[28] = 04000000;        x[28] = 0x100000;
   d[29] = 16777215;        o[29] = 077777777;       x[29] = 0xffffff;
   d[30] = 16777216;        o[30] = 0100000000;      x[30] = 0x1000000;
   d[31] = 268435455;       o[31] = 01777777777;     x[31] = 0xfffffff;
   d[32] = 268435456;       o[32] = 02000000000;     x[32] = 0x10000000;
   d[33] = 1073741823;      o[33] = 07777777777;     x[33] = 0x3fffffff;
   d[34] = 1073741824;      o[34] = 010000000000;    x[34] = 0x40000000;
   d[35] = 4294967295;      o[35] = 037777777777;    x[35] = 0xffffffff;
   d[36] = 4294967296;      o[36] = 040000000000;    x[36] = 0x100000000;
   d[37] = 68719476735;     o[37] = 0777777777777;   x[37] = 0xfffffffff;
   d[38] = 68719476736;     o[38] = 01000000000000;  x[38] = 0x1000000000;

   /* WHEW! */

   for (j=0; j<39; j++){
     if ( g[j] != d[j]
       || d[j] != o[j]
       || o[j] != x[j]) {

       if( pd0 -> flgm != 0 ) {
/*       printf(s241er,16);          save in case opinions change...     */
         printf("Decimal and octal/hex constants sometimes give\n");
         printf("   different results when assigned to longs.\n");
       }
/*     lrc = 1;   save...   */
     }
   }

   if (lrc != 0) rc =16;

   return rc;
}

long pow2(n)        /* Calculate 2**n by multiplying, not shifting  */
long n;
{
   long s;
   s = 1;
   while(n--) s = s*2;
   return s;
}
s243(pd0)                   /*  2.4.3 Character constants  */
struct defs *pd0;
{
   static char s243er[] = "s243,er%d\n";
   static char qs243[8] = "s243   ";
   char *ps, *pt;
   int rc;
   char chars[256];

   rc = 0;
   ps = qs243;
   pt = pd0->rfs;
   while(*pt++ = *ps++);

     /* One of the problems that arises when testing character constants
        is that of definition: What, exactly, is the character set?
        In order to guarantee a certain amount of machine independence,
        the character set we will use here is the set of characters writ-
        able as escape sequences in C, plus those characters used in writ-
        ing C programs, i.e.,

        letters:
                   ABCDEFGHIJKLMNOPQRSTUVWXYZ      26
                   abcdefghijklmnopqrstuvwxyz      26
        numbers:
                   0123456789                      10
        special characters:
                   ~!"#%&()_=-^|{}[]+;*:<>,.?/     27
        extra special characters:
                   newline           \n       
                   horizontal tab    \t
                   backspace         \b
                   carriage return   \r
                   form feed         \f
                   backslash         \\
                   single quote      \'             7
        blank & NUL                                 2
                                                  ---
                                                   98

        Any specific implementation of C may of course support additional
        characters.                                       */

        /* Since the value of a character constant is the numerical value
           of the character in the machine's character set, there should
           be a one-to-one correspondence between characters and values. */

   zerofill(chars);

   chars['a'] = 1;   chars['A'] = 1;   chars['~'] = 1;   chars['0'] = 1;
   chars['b'] = 1;   chars['B'] = 1;   chars['!'] = 1;   chars['1'] = 1;
   chars['c'] = 1;   chars['C'] = 1;   chars['"'] = 1;   chars['2'] = 1;
   chars['d'] = 1;   chars['D'] = 1;   chars['#'] = 1;   chars['3'] = 1;
   chars['e'] = 1;   chars['E'] = 1;   chars['%'] = 1;   chars['4'] = 1;
   chars['f'] = 1;   chars['F'] = 1;   chars['&'] = 1;   chars['5'] = 1;
   chars['g'] = 1;   chars['G'] = 1;   chars['('] = 1;   chars['6'] = 1;
   chars['h'] = 1;   chars['H'] = 1;   chars[')'] = 1;   chars['7'] = 1;
   chars['i'] = 1;   chars['I'] = 1;   chars['_'] = 1;   chars['8'] = 1;
   chars['j'] = 1;   chars['J'] = 1;   chars['='] = 1;   chars['9'] = 1;
   chars['k'] = 1;   chars['K'] = 1;   chars['-'] = 1;
   chars['l'] = 1;   chars['L'] = 1;   chars['^'] = 1;
   chars['m'] = 1;   chars['M'] = 1;   chars['|'] = 1;   chars['\n'] = 1;
   chars['n'] = 1;   chars['N'] = 1;                     chars['\t'] = 1;
   chars['o'] = 1;   chars['O'] = 1;   chars['{'] = 1;   chars['\b'] = 1;
   chars['p'] = 1;   chars['P'] = 1;   chars['}'] = 1;   chars['\r'] = 1;
   chars['q'] = 1;   chars['Q'] = 1;   chars['['] = 1;   chars['\f'] = 1;
   chars['r'] = 1;   chars['R'] = 1;   chars[']'] = 1;
   chars['s'] = 1;   chars['S'] = 1;   chars['+'] = 1;   chars['\\'] = 1;
   chars['t'] = 1;   chars['T'] = 1;   chars[';'] = 1;   chars['\''] = 1;
   chars['u'] = 1;   chars['U'] = 1;   chars['*'] = 1;  
   chars['v'] = 1;   chars['V'] = 1;   chars[':'] = 1;   chars['\0'] = 1;
   chars['w'] = 1;   chars['W'] = 1;   chars['<'] = 1;   chars[' '] = 1;
   chars['x'] = 1;   chars['X'] = 1;   chars['>'] = 1;
   chars['y'] = 1;   chars['Y'] = 1;   chars[','] = 1;
   chars['z'] = 1;   chars['Z'] = 1;   chars['.'] = 1;
                                       chars['?'] = 1;
                                       chars['/'] = 1;

   if(sumof(chars) != 98){
     rc = rc+1;
     if(pd0->flgd != 0) printf(s243er,1);
   }

   /* Finally, the escape \ddd consists of the backslash followed
      by 1, 2, or 3 octal digits which are taken to specify  the
      desired character.                           */

   if( '\0'    !=   0 || '\01'   !=   1 || '\02'   !=   2
    || '\03'   !=   3 || '\04'   !=   4 || '\05'   !=   5
    || '\06'   !=   6 || '\07'   !=   7 || '\10'   !=   8
    || '\17'   !=  15 || '\20'   !=  16 || '\77'   !=  63
    || '\100'  !=  64 || '\177'  != 127                 ){
   
     rc = rc+8;
     if(pd0->flgd != 0) printf(s243er,8);
   }

   return rc;
}
zerofill(x)
char *x;
{
   int j;

   for (j=0; j<256; j++) *x++ = 0;
}
sumof(x)
char *x;
{
   char *p;
   int total, j;

   p = x;
   total = 0;

   for(j=0; j<256; j++) total = total+ *p++;
   return total;
}
s244(pd0)
struct defs *pd0;
{
   double a[8];
   int rc, lrc, j;
   static char s244er[] = "s244,er%d\n";
   static char qs244[8] = "s244   ";
   char *ps, *pt;

   ps = qs244;
   pt = pd0->rfs;
   while(*pt++ = *ps++);
   rc = 0;
   lrc = 0;

   /* Unfortunately, there's not a lot we can do with floating constants.
      We can check to see that the various representations can be com-
      piled, that the conversion is such that they yield the same hard-
      ware representations in all cases, and that all representations
      thus checked are double precision.              */

   a[0] = .1250E+04;
   a[1] = 1.250E3;
   a[2] = 12.50E02;
   a[3] = 125.0e+1;
   a[4] = 1250e00;
   a[5] = 12500.e-01;
   a[6] = 125000e-2;
   a[7] = 1250.;

   lrc = 0;
   for (j=0; j<7; j++) if(a[j] != a[j+1]) lrc = 1;

   if(lrc != 0) {
     if(pd0->flgd != 0) printf(s244er,1);
     rc = rc+1;
   }

   if ( (sizeof .1250E+04 ) != sizeof(double)
     || (sizeof 1.250E3   ) != sizeof(double)
     || (sizeof 12.50E02  ) != sizeof(double)
     || (sizeof 1.250e+1  ) != sizeof(double)
     || (sizeof 1250e00   ) != sizeof(double)
     || (sizeof 12500.e-01) != sizeof(double)
     || (sizeof 125000e-2 ) != sizeof(double)
     || (sizeof 1250.     ) != sizeof(double)){
     
     if(pd0->flgd != 0) printf(s244er,2);
     rc = rc+2;
   }

   return rc;
}
s25(pd0)
struct defs *pd0;
{
   char *s, *s2;
   int rc, lrc, j;
   static char s25er[] = "s25,er%d\n";
   static char qs25[8] = "s25    ";
   char *ps, *pt;

   ps = qs25;
   pt = pd0->rfs;
   while(*pt++ = *ps++);
   rc = 0;

   /* A string is a sequence of characters surrounded by double
      quotes, as in "...".                         */

   s = "...";

   /* A string has type "array of characters" and storage class
      static and is initialized with the given characters.  */

   if ( s[0] != s[1] || s[1] != s[2]
     || s[2] != '.' ) {

    rc = rc+1;
     if(pd0->flgd != 0) printf(s25er,1);
   }

   /* The compiler places a null byte \0 at the end of each string
      so the program which scans the string can find its end.   */

   if( s[3] != '\0' ){
     rc = rc+4;
     if(pd0->flgd != 0) printf(s25er,4);
   }

   /* In a string, the double quote character " must be preceded
      by a \.                                               */

    if( ".\"."[1] != '"' ){
    rc = rc+8;
     if(pd0->flgd != 0) printf(s25er,8);
   }

   /* In addition, the same escapes described for character constants
      may be used.                                            */

   s = "\n\t\b\r\f\\\'";

   if( s[0] != '\n'
    || s[1] != '\t'
    || s[2] != '\b'
    || s[3] != '\r'
    || s[4] != '\f'
    || s[5] != '\\'
    || s[6] != '\'' ){

     rc = rc+16;
     if( pd0->flgd != 0) printf(s25er,16);
   }

   /* Finally, a \ and an immediately following newline are ignored */

   s2 = "queep!";
   s = "queep!";

   lrc = 0;
   for (j=0; j<sizeof "queep!"; j++) if(s[j] != s2[j]) lrc = 1;
   if (lrc != 0){
     rc = rc+32;
     if(pd0->flgd != 0) printf(s25er,32);
   }
   return rc;
}
s26(pd0)                  /*  2.6  Hardware Characteristics     */
struct defs *pd0;
{
   static char qs26[8] = "s26    ";
   char *ps, *pt;
   char c0, c1;
   float temp, one, delta;
   double tempd, oned;
   static char s[] = "%3d bits in %ss.\n";
   static char s2[] = "%e is the least number that can be added to 1. (%s).\n";

   ps = qs26;
   pt = pd0->rfs;

   while(*pt++ = *ps++);

          /* Here, we shake the machinery a little to see what falls
             out.  First, we find out how many bits are in a char.  */

   pd0->cbits = 0;
   c0 = 0;
   c1 = 1;

   while(c0 != c1) {
     c1 = c1<<1;
     pd0->cbits = pd0->cbits+1;
   }
          /* That information lets us determine the size of everything else. */

   pd0->ibits = pd0->cbits * sizeof(int);
   pd0->sbits = pd0->cbits * sizeof(short);
   pd0->lbits = pd0->cbits * sizeof(long);
   pd0->ubits = pd0->cbits * sizeof(unsigned);
   pd0->fbits = pd0->cbits * sizeof(float);
   pd0->dbits = pd0->cbits * sizeof(double);

          /* We have now almost reconstructed the table in section 2.6, the
             exception being the range of the floating point hardware.
             Now there are just so many ways to conjure up a floating point
             representation system that it's damned near impossible to guess
             what's going on by writing a program to interpret bit patterns.
             Further, the information isn't all that useful, if we consider
             the fact that machines that won't handle numbers between 10**30
             and 10**-30 are very hard to find, and that people playing with
             numbers outside that range have a lot more to worry about than
             just the capacity of the characteristic.

             A much more useful measure is the precision, which can be ex-
             pressed in terms of the smallest number that can be added to
             1. without loss of significance. We calculate that here, for
             float and double.                       */

   one = 1.;
   delta = 1.;
   temp = 0.;
   while(temp != one) {
     temp = one+delta;
     delta = delta/2.;
   }
   pd0->fprec = delta * 4.;
   oned = 1.;
   delta = 1.;
   tempd = 0.;
   while(tempd != oned) {
     tempd = oned+delta;
     delta = delta/2.;
   }
   pd0->dprec = delta * 4.;

          /* Now, if anyone's interested, we publish the results.       */

   if(pd0->flgm != 0) {
     printf(s,pd0->cbits,"char");
     printf(s,pd0->ibits,"int");
     printf(s,pd0->sbits,"short");
     printf(s,pd0->lbits,"long");
     printf(s,pd0->ubits,"unsigned");
     printf(s,pd0->fbits,"float");
     printf(s,pd0->dbits,"double");
     printf(s2,pd0->fprec,"float");
     printf(s2,pd0->dprec,"double");
   }
          /* Since we are only exploring and perhaps reporting, but not 
             testing any features, we cannot return an error code.  */

   return 0;
}
int extvar;
s4(pd0)                    /* 4. What's in a name?             */
struct defs *pd0;
{
   static char s4er[] = "s4,er%d\n";
   static char qs4[8] = "s4     ";
   char *ps, *pt;
   int j, rc;

   short sint;             /* short integer, for size test      */
   int pint;               /* plain                             */
   long lint;              /* long                              */
   unsigned target;
   unsigned int mask;

   rc = 0;
   ps = qs4;
   pt = pd0->rfs;

   while(*pt++ = *ps++);

/*   There are four declarable storage classes: automatic,
static, external, and register. Automatic variables have
been dealt with extensively thus far, and will not be specif-
ically treated in this section. Register variables are treated
in section s81.

     Static variables are local to a block, but retain their
values upon reentry to a block, even after control has left
the block.                                                     */

   for (j=0; j<3; j++)
     if(svtest(j) != zero()){
       rc = 1;
       if(pd0->flgd != 0) printf(s4er,1);
     }
   ;

/*   External variables exist and retain their values throughout
the execution of the entire program, and may be used for comm-
unication between functions, even separately compiled functions.
                                                                */

   setev();
   if(testev() != 0){
     rc=rc+2;
     if(pd0->flgd != 0) printf(s4er,2);
   }
/*   
     Characters have been tested elsewhere (in s243).

     Up to three sizes of integer, declared short int, int, and
long int, are available. Longer integers provide no less storage
than shorter ones, but implementation may make either short
integers, or long integers, or both, equivalent to plain
integers.
                                                                */

   if(sizeof lint < sizeof pint || sizeof pint < sizeof sint){

     rc = rc+4;
     if(pd0->flgd != 0) printf(s4er,4);
   }

/*   Unsigned integers, declared unsigned, obey the laws of
arithmetic modulo 2**n, where n is the number of bits in the
implementation                                                  */

   target = ~0U;
   mask = 1;
 
   for(j=0; j<(sizeof target)*pd0->cbits; j++){
   
     mask = mask&target;
     target = target>>1;
   }

   if(mask != 1 || target != 0){

     rc = rc+8;
     if(pd0->flgd != 0) printf(s4er,8);
   }

   return rc;
}
svtest(n)
int n;
{
   static k;
   int rc;
   switch (n) {

     case 0: k = 1978;
             rc = 0;
             break;

     case 1: if(k != 1978) rc = 1;
             else{
              k = 1929;
              rc = 0;
             }
             break;

     case 2: if(k != 1929) rc = 1;
             else rc = 0;
             break;
   }
   return rc;
}
zero(){                 /* Returns a value of zero, possibly */
   static k;            /* with side effects, as it's called */
   int rc;              /* alternately with svtest, above,   */
   k = 2;               /* and has the same internal storage */
   rc = 0;              /* requirements.                     */
   return rc;
}
testev(){
   if(extvar != 1066) return 1;
   else return 0;
}
s61(pd0)          /* Characters and integers */
struct defs *pd0;
{
   static char s61er[] = "s61,er%d\n";
   static char qs61[8] = "s61    ";
   short from, shortint;
   long int to, longint;
   int rc, lrc;
   int j;
   char fromc, charint;
   char *wd, *pc[6];
   
   static char upper_alpha[]             = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   static char lower_alpha[]             = "abcdefghijklmnopqrstuvwxyz";
   static char numbers[]               = "0123456789";
   static char special_characters[]    = "~!\"#%&()_=-^|{}[]+;*:<>,.?/";
   static char extra_special_characters[] = "\n\t\b\r\f\\\'";
   static char blank_and_NUL[]            = " \0";

   char *ps, *pt;
   ps = qs61;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

/*      A character or a short integer may be used wherever
an integer may be used. In all cases, the value is converted
to integer. This principle is extensively used throughout this
program, and will not be explicitly tested here.        */

/*      Conversion of a shorter integer to a longer always
involves sign extension.                                */

   from = -19;
   to = from;

   if(to != -19){
     rc = rc+1;
     if(pd0->flgd != 0) printf(s61er,1);
   }

/*      It is guaranteed that a member of the standard char-
acter set is nonnegative.                               */

   pc[0] = upper_alpha;
   pc[1] = lower_alpha;
   pc[2] = numbers;
   pc[3] = special_characters;
   pc[4] = extra_special_characters;
   pc[5] = blank_and_NUL;

   lrc = 0;
   for (j=0; j<6; j++)
     while(*pc[j]) if(*pc[j]++ < 0) lrc =1;

   if(lrc != 0){
     rc=rc+2;
     if(pd0->flgd != 0) printf(s61er,2);
   }

/*      When a longer integer is converted to a shorter or
to  a char, it is truncated on the left; excess bits are 
simply discarded.                                       */

   longint = 1048579;           /* =2**20+3 */
   shortint = longint;
   charint = longint;

   if((shortint != longint && shortint != 3) ||
      (charint  != longint && charint  != 3)) {
     rc = rc+8;
     if(pd0->flgd != 0) printf(s61er,8);
   }

   return rc;
}
s626(pd0)          /* 6.2 Float and double                  */
                   /* 6.3 Floating and integral                 */
                   /* 6.4 Pointers and integers                 */
                   /* 6.5 Unsigned                              */
                   /* 6.6 Arithmetic conversions                */
struct defs *pd0;
{
   static char s626er[] = "s626,er%d\n";
   static char qs626[8] = "s626   ";
   int rc;
   char *ps, *pt;
   float eps, f1, f2, f3, f4, f;
   long lint1, lint2, l, ls;
   char c, t[28], t0;
   short s;
   int is, i, j;
   unsigned u, us;
   double d, ds;
   ps = qs626;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* Conversions of integral values to floating type are
        well-behaved.                                           */

   f1 = 1.;
   lint1 = 1.;
   lint2 = 1.;

   for(j=0;j<pd0->lbits-2;j++){
     f1 = f1*2;
     lint2 = (lint2<<1)|lint1;
   }
   f2 = lint2;
   f1 = (f1-f2)/f1;
   if(f1>2.*pd0->fprec){

     rc = rc+2;
     if(pd0->flgd != 0) printf(s626er,2);
   }

        /* Pointer-integer combinations are discussed in s74,
        "Additive operators". The unsigned-int combination
        appears below.                                          */

   c = 125;
   s = 125;
   i = 125;     is = 15625;
   u = 125;     us = 15625;
   l = 125;     ls = 15625;
   f = 125.;
   d = 125.;    ds = 15625.;

   for(j=0;j<28;j++) t[j] = 0;

   if(c*c != is) t[ 0] = 1;
   if(s*c != is) t[ 1] = 1;
   if(s*s != is) t[ 2] = 1;
   if(i*c != is) t[ 3] = 1;
   if(i*s != is) t[ 4] = 1;
   if(i*i != is) t[ 5] = 1;
   if(u*c != us) t[ 6] = 1;
   if(u*s != us) t[ 7] = 1;
   if(u*i != us) t[ 8] = 1;
   if(u*u != us) t[ 9] = 1;
   if(l*c != ls) t[10] = 1;
   if(l*s != ls) t[11] = 1;
   if(l*i != ls) t[12] = 1;
   if(l*u != us) t[13] = 1;
   if(l*l != ls) t[14] = 1;
   if(f*c != ds) t[15] = 1;
   if(f*s != ds) t[16] = 1;
   if(f*i != ds) t[17] = 1;
   if(f*u != ds) t[18] = 1;
   if(f*l != ds) t[19] = 1;
   if(f*f != ds) t[20] = 1;
   if(d*c != ds) t[21] = 1;
   if(d*s != ds) t[22] = 1;
   if(d*i != ds) t[23] = 1;
   if(d*u != ds) t[24] = 1;
   if(d*l != ds) t[25] = 1;
   if(d*f != ds) t[26] = 1;
   if(d*d != ds) t[27] = 1;

   t0 = 0;
   for(j=0; j<28; j++) t0 = t0+t[j];

   if(t0 != 0){

     rc = rc+4;
     if(pd0->flgd != 0){

       printf(s626er,4);
       printf("   key=");
       for(j=0;j<28;j++) printf("%d",t[j]);
       printf("\n");
     }
   }

        /* When an unsigned integer is converted to long,
           the value of the result is the same numerically
           as that of the unsigned integer.               */

   l = (unsigned)0100000;
   if((long)l > (unsigned)0100000){

      rc = rc+8;
      if(pd0->flgd != 0) printf(s626er,8);
   }

   return rc;
}
s71(pd0)          /*         7.1  Primary expressions   */
struct defs *pd0;
{
   static char s71er[] = "s71,er%d\n";
   static char qs71[8] = "s71    ";
   int rc;
   char *ps, *pt;
   static char q = 'q';
   int x[10], McCarthy(), clobber(), a, b, *p;
   ps = qs71;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

/*   Testing of expressions and operators is quite complicated,
     because (a) problems are apt to surface in queer combinations
     of operators and operands, rather than in isolation,
     and (b) the number of expressions needed to provoke a case
     of improper behaviour may be quite large. Hence, we take the
     following approach: for this section, and for subsequent
     sections through 7.15, we will check the primitive operations
     in isolation, thus verifying that the primitives work,
     after a fashion. The job of testing combinations, we will
     leave to a separate, machine-generated program, to be included
     in the C test package at some later date.
                                                                */

/*   A string is a primary expression. The identifier points to
     the first character of a string.
                                                                  */

   if(*"queep" != q){
     rc = rc+1;
     if(pd0->flgd  != 0) printf(s71er,1);
   }
/*   A parenthesized expression is a primary expression whose
     type and value are the same as those of the unadorned
     expression.
                                                                */
   if((2+3) != 2+3) {
     rc = rc+2;
     if(pd0->flgd != 0) printf(s71er,2);
   }

/*   A primary expression followed by an expression in square 
     brackets is a primary expression. The intuitive meaning is
     that of a subscript. The expression E1[E2] is identical
     (by definition) to *((E1)+(E2)).
                                                                */

   x[5] = 1942;
   if(x[5] != 1942 || x[5] != *((x)+(5))){
     rc = rc+4;
     if(pd0->flgd != 0) printf(s71er,4);
   }

/*   If the various flavors of function calls didn't work, we
     would never have gotten this far; however, we do need to 
     show that functions can be recursive...
                                                               */

   if ( McCarthy(-5) != 91){
     rc = rc+8;
     if(pd0->flgd != 0) printf(s71er,8);
   }

/*   and that argument passing is strictly by value.           */

   a = 2;
   b = 3;
   p = &b;

   clobber(a,p);

   if(a != 2 || b != 2){
     rc = rc+16;
     if(pd0->flgd != 0) printf(s71er,16);
   }

/*   Finally, structures and unions are addressed thusly:      */

   if(pd0->dprec != (*pd0).dprec){
     rc = rc+32;
     if(pd0->flgd != 0) printf(s71er,32);
   }

   return rc;
}
McCarthy(x)
int x;
{
   if(x>100) return x-10;
   else return McCarthy( McCarthy(x+11));
}
clobber(x,y)
int x, *y;
{
   x = 3;
   *y = 2;
}
s714(pd0)          /*  7.14  Assignment operators       */
struct defs *pd0;
{
   static char f[] = "Local error %d.\n";
   static char s714er[] = "s714,er%d\n";
   static char qs714[8] = "s714   ";
   register int prlc, lrc;
   int rc;
   char cl, cr;
   short sl, sr;
   int il, ir;
   long ll, lr;
   unsigned ul, ur;
   float fl, fr;
   double dl, dr;
   char *ps, *pt;
   ps = qs714;
   pt = pd0->rfs;
   rc = 0;
   lrc = 0;
   prlc = pd0->flgl;
   while (*pt++ = *ps++);

        /* This section tests the assignment operators.

        It is an exhaustive test of all assignment statements
        of the form:

                vl op vr

        where vl and vr are variables from the set
        {char,short,int,long,unsigned,float,double} and op is
        one of the assignment operators. There are 395 such
        statements.

        The initial values for the variables have been chosen
        so that both the initial values and the results will
        "fit" in just about any implementation, and that the re-
        sults will be such that they test for the proper form-
        ation of composite operators, rather than checking for
        the valid operation of those operators' components.
        For example, in checking >>=, we want to verify that
        a right shift and a move take place, rather than
        whether or not there may be some peculiarities about
        the right shift. Such tests have been made previously,
        and to repeat them here would be to throw out a red
        herring.

        The table below lists the operators, assignment targets,
        initial values for left and right operands, and the
        expected values of the results.


          =  +=  -=  *=  /=  %=  >>=  <<=  &=  ^=  |=	
char      2   7   3  10   2   1   1    20   8   6  14
short     2   7   3  10   2   1   1    20   8   6  14
int       2   7   3  10   2   1   1    20   8   6  14
long      2   7   3  10   2   1   1    20   8   6  14
unsigned  2   7   3  10   2   1   1    20   8   6  14
float     2   7   3  10 2.5 |             |
double    2   7   3  10 2.5 |             |
                            |             |
initial         (5,2)       |    (5,2)    |  (12,10)

        The following machine-generated program reflects the
        tests described in the table.
                                                                */

   cl = 5; cr = 2;
   cl = cr;
   if(cl != 2){
     lrc = 1;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl = sr;
   if(cl != 2){
     lrc = 2;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl = ir;
   if(cl != 2){
     lrc = 3;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl = lr;
   if(cl != 2){
     lrc = 4;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl = ur;
   if(cl != 2){
     lrc = 5;
     if(prlc) printf(f,lrc);
   }
   cl = 5; fr = 2;
   cl = fr;
   if(cl != 2){
     lrc = 6;
     if(prlc) printf(f,lrc);
   }
   cl = 5; dr = 2;
   cl = dr;
   if(cl != 2){
     lrc = 7;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl = cr;
   if(sl != 2){
     lrc = 8;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl = sr;
   if(sl != 2){
     lrc = 9;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl = ir;
   if(sl != 2){
     lrc = 10;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl = lr;
   if(sl != 2){
     lrc = 11;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl = ur;
   if(sl != 2){
     lrc = 12;
     if(prlc) printf(f,lrc);
   }
   sl = 5; fr = 2;
   sl = fr;
   if(sl != 2){
     lrc = 13;
     if(prlc) printf(f,lrc);
   }
   sl = 5; dr = 2;
   sl = dr;
   if(sl != 2){
     lrc = 14;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il = cr;
   if(il != 2){
     lrc = 15;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il = sr;
   if(il != 2){
     lrc = 16;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il = ir;
   if(il != 2){
     lrc = 17;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il = lr;
   if(il != 2){
     lrc = 18;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il = ur;
   if(il != 2){
     lrc = 19;
     if(prlc) printf(f,lrc);
   }
   il = 5; fr = 2;
   il = fr;
   if(il != 2){
     lrc = 20;
     if(prlc) printf(f,lrc);
   }
   il = 5; dr = 2;
   il = dr;
   if(il != 2){
     lrc = 21;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll = cr;
   if(ll != 2){
     lrc = 22;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll = sr;
   if(ll != 2){
     lrc = 23;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll = ir;
   if(ll != 2){
     lrc = 24;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll = lr;
   if(ll != 2){
     lrc = 25;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll = ur;
   if(ll != 2){
     lrc = 26;
     if(prlc) printf(f,lrc);
   }
   ll = 5; fr = 2;
   ll = fr;
   if(ll != 2){
     lrc = 27;
     if(prlc) printf(f,lrc);
   }
   ll = 5; dr = 2;
   ll = dr;
   if(ll != 2){
     lrc = 28;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul = cr;
   if(ul != 2){
     lrc = 29;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul = sr;
   if(ul != 2){
     lrc = 30;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul = ir;
   if(ul != 2){
     lrc = 31;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul = lr;
   if(ul != 2){
     lrc = 32;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul = ur;
   if(ul != 2){
     lrc = 33;
     if(prlc) printf(f,lrc);
   }
   ul = 5; fr = 2;
   ul = fr;
   if(ul != 2){
     lrc = 34;
     if(prlc) printf(f,lrc);
   }
   ul = 5; dr = 2;
   ul = dr;
   if(ul != 2){
     lrc = 35;
     if(prlc) printf(f,lrc);
   }
   fl = 5; cr = 2;
   fl = cr;
   if(fl != 2){
     lrc = 36;
     if(prlc) printf(f,lrc);
   }
   fl = 5; sr = 2;
   fl = sr;
   if(fl != 2){
     lrc = 37;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ir = 2;
   fl = ir;
   if(fl != 2){
     lrc = 38;
     if(prlc) printf(f,lrc);
   }
   fl = 5; lr = 2;
   fl = lr;
   if(fl != 2){
     lrc = 39;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ur = 2;
   fl = ur;
   if(fl != 2){
     lrc = 40;
     if(prlc) printf(f,lrc);
   }
   fl = 5; fr = 2;
   fl = fr;
   if(fl != 2){
     lrc = 41;
     if(prlc) printf(f,lrc);
   }
   fl = 5; dr = 2;
   fl = dr;
   if(fl != 2){
     lrc = 42;
     if(prlc) printf(f,lrc);
   }
   dl = 5; cr = 2;
   dl = cr;
   if(dl != 2){
     lrc = 43;
     if(prlc) printf(f,lrc);
   }
   dl = 5; sr = 2;
   dl = sr;
   if(dl != 2){
     lrc = 44;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ir = 2;
   dl = ir;
   if(dl != 2){
     lrc = 45;
     if(prlc) printf(f,lrc);
   }
   dl = 5; lr = 2;
   dl = lr;
   if(dl != 2){
     lrc = 46;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ur = 2;
   dl = ur;
   if(dl != 2){
     lrc = 47;
     if(prlc) printf(f,lrc);
   }
   dl = 5; fr = 2;
   dl = fr;
   if(dl != 2){
     lrc = 48;
     if(prlc) printf(f,lrc);
   }
   dl = 5; dr = 2;
   dl = dr;
   if(dl != 2){
     lrc = 49;
     if(prlc) printf(f,lrc);
   }
   cl = 5; cr = 2;
   cl += cr;
   if(cl != 7){
     lrc = 50;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl += sr;
   if(cl != 7){
     lrc = 51;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl += ir;
   if(cl != 7){
     lrc = 52;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl += lr;
   if(cl != 7){
     lrc = 53;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl += ur;
   if(cl != 7){
     lrc = 54;
     if(prlc) printf(f,lrc);
   }
   cl = 5; fr = 2;
   cl += fr;
   if(cl != 7){
     lrc = 55;
     if(prlc) printf(f,lrc);
   }
   cl = 5; dr = 2;
   cl += dr;
   if(cl != 7){
     lrc = 56;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl += cr;
   if(sl != 7){
     lrc = 57;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl += sr;
   if(sl != 7){
     lrc = 58;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl += ir;
   if(sl != 7){
     lrc = 59;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl += lr;
   if(sl != 7){
     lrc = 60;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl += ur;
   if(sl != 7){
     lrc = 61;
     if(prlc) printf(f,lrc);
   }
   sl = 5; fr = 2;
   sl += fr;
   if(sl != 7){
     lrc = 62;
     if(prlc) printf(f,lrc);
   }
   sl = 5; dr = 2;
   sl += dr;
   if(sl != 7){
     lrc = 63;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il += cr;
   if(il != 7){
     lrc = 64;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il += sr;
   if(il != 7){
     lrc = 65;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il += ir;
   if(il != 7){
     lrc = 66;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il += lr;
   if(il != 7){
     lrc = 67;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il += ur;
   if(il != 7){
     lrc = 68;
     if(prlc) printf(f,lrc);
   }
   il = 5; fr = 2;
   il += fr;
   if(il != 7){
     lrc = 69;
     if(prlc) printf(f,lrc);
   }
   il = 5; dr = 2;
   il += dr;
   if(il != 7){
     lrc = 70;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll += cr;
   if(ll != 7){
     lrc = 71;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll += sr;
   if(ll != 7){
     lrc = 72;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll += ir;
   if(ll != 7){
     lrc = 73;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll += lr;
   if(ll != 7){
     lrc = 74;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll += ur;
   if(ll != 7){
     lrc = 75;
     if(prlc) printf(f,lrc);
   }
   ll = 5; fr = 2;
   ll += fr;
   if(ll != 7){
     lrc = 76;
     if(prlc) printf(f,lrc);
   }
   ll = 5; dr = 2;
   ll += dr;
   if(ll != 7){
     lrc = 77;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul += cr;
   if(ul != 7){
     lrc = 78;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul += sr;
   if(ul != 7){
     lrc = 79;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul += ir;
   if(ul != 7){
     lrc = 80;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul += lr;
   if(ul != 7){
     lrc = 81;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul += ur;
   if(ul != 7){
     lrc = 82;
     if(prlc) printf(f,lrc);
   }
   ul = 5; fr = 2;
   ul += fr;
   if(ul != 7){
     lrc = 83;
     if(prlc) printf(f,lrc);
   }
   ul = 5; dr = 2;
   ul += dr;
   if(ul != 7){
     lrc = 84;
     if(prlc) printf(f,lrc);
   }
   fl = 5; cr = 2;
   fl += cr;
   if(fl != 7){
     lrc = 85;
     if(prlc) printf(f,lrc);
   }
   fl = 5; sr = 2;
   fl += sr;
   if(fl != 7){
     lrc = 86;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ir = 2;
   fl += ir;
   if(fl != 7){
     lrc = 87;
     if(prlc) printf(f,lrc);
   }
   fl = 5; lr = 2;
   fl += lr;
   if(fl != 7){
     lrc = 88;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ur = 2;
   fl += ur;
   if(fl != 7){
     lrc = 89;
     if(prlc) printf(f,lrc);
   }
   fl = 5; fr = 2;
   fl += fr;
   if(fl != 7){
     lrc = 90;
     if(prlc) printf(f,lrc);
   }
   fl = 5; dr = 2;
   fl += dr;
   if(fl != 7){
     lrc = 91;
     if(prlc) printf(f,lrc);
   }
   dl = 5; cr = 2;
   dl += cr;
   if(dl != 7){
     lrc = 92;
     if(prlc) printf(f,lrc);
   }
   dl = 5; sr = 2;
   dl += sr;
   if(dl != 7){
     lrc = 93;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ir = 2;
   dl += ir;
   if(dl != 7){
     lrc = 94;
     if(prlc) printf(f,lrc);
   }
   dl = 5; lr = 2;
   dl += lr;
   if(dl != 7){
     lrc = 95;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ur = 2;
   dl += ur;
   if(dl != 7){
     lrc = 96;
     if(prlc) printf(f,lrc);
   }
   dl = 5; fr = 2;
   dl += fr;
   if(dl != 7){
     lrc = 97;
     if(prlc) printf(f,lrc);
   }
   dl = 5; dr = 2;
   dl += dr;
   if(dl != 7){
     lrc = 98;
     if(prlc) printf(f,lrc);
   }
   cl = 5; cr = 2;
   cl -= cr;
   if(cl != 3){
     lrc = 99;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl -= sr;
   if(cl != 3){
     lrc = 100;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl -= ir;
   if(cl != 3){
     lrc = 101;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl -= lr;
   if(cl != 3){
     lrc = 102;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl -= ur;
   if(cl != 3){
     lrc = 103;
     if(prlc) printf(f,lrc);
   }
   cl = 5; fr = 2;
   cl -= fr;
   if(cl != 3){
     lrc = 104;
     if(prlc) printf(f,lrc);
   }
   cl = 5; dr = 2;
   cl -= dr;
   if(cl != 3){
     lrc = 105;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl -= cr;
   if(sl != 3){
     lrc = 106;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl -= sr;
   if(sl != 3){
     lrc = 107;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl -= ir;
   if(sl != 3){
     lrc = 108;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl -= lr;
   if(sl != 3){
     lrc = 109;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl -= ur;
   if(sl != 3){
     lrc = 110;
     if(prlc) printf(f,lrc);
   }
   sl = 5; fr = 2;
   sl -= fr;
   if(sl != 3){
     lrc = 111;
     if(prlc) printf(f,lrc);
   }
   sl = 5; dr = 2;
   sl -= dr;
   if(sl != 3){
     lrc = 112;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il -= cr;
   if(il != 3){
     lrc = 113;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il -= sr;
   if(il != 3){
     lrc = 114;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il -= ir;
   if(il != 3){
     lrc = 115;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il -= lr;
   if(il != 3){
     lrc = 116;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il -= ur;
   if(il != 3){
     lrc = 117;
     if(prlc) printf(f,lrc);
   }
   il = 5; fr = 2;
   il -= fr;
   if(il != 3){
     lrc = 118;
     if(prlc) printf(f,lrc);
   }
   il = 5; dr = 2;
   il -= dr;
   if(il != 3){
     lrc = 119;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll -= cr;
   if(ll != 3){
     lrc = 120;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll -= sr;
   if(ll != 3){
     lrc = 121;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll -= ir;
   if(ll != 3){
     lrc = 122;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll -= lr;
   if(ll != 3){
     lrc = 123;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll -= ur;
   if(ll != 3){
     lrc = 124;
     if(prlc) printf(f,lrc);
   }
   ll = 5; fr = 2;
   ll -= fr;
   if(ll != 3){
     lrc = 125;
     if(prlc) printf(f,lrc);
   }
   ll = 5; dr = 2;
   ll -= dr;
   if(ll != 3){
     lrc = 126;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul -= cr;
   if(ul != 3){
     lrc = 127;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul -= sr;
   if(ul != 3){
     lrc = 128;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul -= ir;
   if(ul != 3){
     lrc = 129;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul -= lr;
   if(ul != 3){
     lrc = 130;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul -= ur;
   if(ul != 3){
     lrc = 131;
     if(prlc) printf(f,lrc);
   }
   ul = 5; fr = 2;
   ul -= fr;
   if(ul != 3){
     lrc = 132;
     if(prlc) printf(f,lrc);
   }
   ul = 5; dr = 2;
   ul -= dr;
   if(ul != 3){
     lrc = 133;
     if(prlc) printf(f,lrc);
   }
   fl = 5; cr = 2;
   fl -= cr;
   if(fl != 3){
     lrc = 134;
     if(prlc) printf(f,lrc);
   }
   fl = 5; sr = 2;
   fl -= sr;
   if(fl != 3){
     lrc = 135;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ir = 2;
   fl -= ir;
   if(fl != 3){
     lrc = 136;
     if(prlc) printf(f,lrc);
   }
   fl = 5; lr = 2;
   fl -= lr;
   if(fl != 3){
     lrc = 137;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ur = 2;
   fl -= ur;
   if(fl != 3){
     lrc = 138;
     if(prlc) printf(f,lrc);
   }
   fl = 5; fr = 2;
   fl -= fr;
   if(fl != 3){
     lrc = 139;
     if(prlc) printf(f,lrc);
   }
   fl = 5; dr = 2;
   fl -= dr;
   if(fl != 3){
     lrc = 140;
     if(prlc) printf(f,lrc);
   }
   dl = 5; cr = 2;
   dl -= cr;
   if(dl != 3){
     lrc = 141;
     if(prlc) printf(f,lrc);
   }
   dl = 5; sr = 2;
   dl -= sr;
   if(dl != 3){
     lrc = 142;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ir = 2;
   dl -= ir;
   if(dl != 3){
     lrc = 143;
     if(prlc) printf(f,lrc);
   }
   dl = 5; lr = 2;
   dl -= lr;
   if(dl != 3){
     lrc = 144;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ur = 2;
   dl -= ur;
   if(dl != 3){
     lrc = 145;
     if(prlc) printf(f,lrc);
   }
   dl = 5; fr = 2;
   dl -= fr;
   if(dl != 3){
     lrc = 146;
     if(prlc) printf(f,lrc);
   }
   dl = 5; dr = 2;
   dl -= dr;
   if(dl != 3){
     lrc = 147;
     if(prlc) printf(f,lrc);
   }
   cl = 5; cr = 2;
   cl *= cr;
   if(cl != 10){
     lrc = 148;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl *= sr;
   if(cl != 10){
     lrc = 149;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl *= ir;
   if(cl != 10){
     lrc = 150;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl *= lr;
   if(cl != 10){
     lrc = 151;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl *= ur;
   if(cl != 10){
     lrc = 152;
     if(prlc) printf(f,lrc);
   }
   cl = 5; fr = 2;
   cl *= fr;
   if(cl != 10){
     lrc = 153;
     if(prlc) printf(f,lrc);
   }
   cl = 5; dr = 2;
   cl *= dr;
   if(cl != 10){
     lrc = 154;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl *= cr;
   if(sl != 10){
     lrc = 155;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl *= sr;
   if(sl != 10){
     lrc = 156;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl *= ir;
   if(sl != 10){
     lrc = 157;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl *= lr;
   if(sl != 10){
     lrc = 158;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl *= ur;
   if(sl != 10){
     lrc = 159;
     if(prlc) printf(f,lrc);
   }
   sl = 5; fr = 2;
   sl *= fr;
   if(sl != 10){
     lrc = 160;
     if(prlc) printf(f,lrc);
   }
   sl = 5; dr = 2;
   sl *= dr;
   if(sl != 10){
     lrc = 161;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il *= cr;
   if(il != 10){
     lrc = 162;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il *= sr;
   if(il != 10){
     lrc = 163;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il *= ir;
   if(il != 10){
     lrc = 164;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il *= lr;
   if(il != 10){
     lrc = 165;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il *= ur;
   if(il != 10){
     lrc = 166;
     if(prlc) printf(f,lrc);
   }
   il = 5; fr = 2;
   il *= fr;
   if(il != 10){
     lrc = 167;
     if(prlc) printf(f,lrc);
   }
   il = 5; dr = 2;
   il *= dr;
   if(il != 10){
     lrc = 168;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll *= cr;
   if(ll != 10){
     lrc = 169;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll *= sr;
   if(ll != 10){
     lrc = 170;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll *= ir;
   if(ll != 10){
     lrc = 171;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll *= lr;
   if(ll != 10){
     lrc = 172;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll *= ur;
   if(ll != 10){
     lrc = 173;
     if(prlc) printf(f,lrc);
   }
   ll = 5; fr = 2;
   ll *= fr;
   if(ll != 10){
     lrc = 174;
     if(prlc) printf(f,lrc);
   }
   ll = 5; dr = 2;
   ll *= dr;
   if(ll != 10){
     lrc = 175;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul *= cr;
   if(ul != 10){
     lrc = 176;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul *= sr;
   if(ul != 10){
     lrc = 177;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul *= ir;
   if(ul != 10){
     lrc = 178;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul *= lr;
   if(ul != 10){
     lrc = 179;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul *= ur;
   if(ul != 10){
     lrc = 180;
     if(prlc) printf(f,lrc);
   }
   ul = 5; fr = 2;
   ul *= fr;
   if(ul != 10){
     lrc = 181;
     if(prlc) printf(f,lrc);
   }
   ul = 5; dr = 2;
   ul *= dr;
   if(ul != 10){
     lrc = 182;
     if(prlc) printf(f,lrc);
   }
   fl = 5; cr = 2;
   fl *= cr;
   if(fl != 10){
     lrc = 183;
     if(prlc) printf(f,lrc);
   }
   fl = 5; sr = 2;
   fl *= sr;
   if(fl != 10){
     lrc = 184;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ir = 2;
   fl *= ir;
   if(fl != 10){
     lrc = 185;
     if(prlc) printf(f,lrc);
   }
   fl = 5; lr = 2;
   fl *= lr;
   if(fl != 10){
     lrc = 186;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ur = 2;
   fl *= ur;
   if(fl != 10){
     lrc = 187;
     if(prlc) printf(f,lrc);
   }
   fl = 5; fr = 2;
   fl *= fr;
   if(fl != 10){
     lrc = 188;
     if(prlc) printf(f,lrc);
   }
   fl = 5; dr = 2;
   fl *= dr;
   if(fl != 10){
     lrc = 189;
     if(prlc) printf(f,lrc);
   }
   dl = 5; cr = 2;
   dl *= cr;
   if(dl != 10){
     lrc = 190;
     if(prlc) printf(f,lrc);
   }
   dl = 5; sr = 2;
   dl *= sr;
   if(dl != 10){
     lrc = 191;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ir = 2;
   dl *= ir;
   if(dl != 10){
     lrc = 192;
     if(prlc) printf(f,lrc);
   }
   dl = 5; lr = 2;
   dl *= lr;
   if(dl != 10){
     lrc = 193;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ur = 2;
   dl *= ur;
   if(dl != 10){
     lrc = 194;
     if(prlc) printf(f,lrc);
   }
   dl = 5; fr = 2;
   dl *= fr;
   if(dl != 10){
     lrc = 195;
     if(prlc) printf(f,lrc);
   }
   dl = 5; dr = 2;
   dl *= dr;
   if(dl != 10){
     lrc = 196;
     if(prlc) printf(f,lrc);
   }
   cl = 5; cr = 2;
   cl /= cr;
   if(cl != 2){
     lrc = 197;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl /= sr;
   if(cl != 2){
     lrc = 198;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl /= ir;
   if(cl != 2){
     lrc = 199;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl /= lr;
   if(cl != 2){
     lrc = 200;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl /= ur;
   if(cl != 2){
     lrc = 201;
     if(prlc) printf(f,lrc);
   }
   cl = 5; fr = 2;
   cl /= fr;
   if(cl != 2){
     lrc = 202;
     if(prlc) printf(f,lrc);
   }
   cl = 5; dr = 2;
   cl /= dr;
   if(cl != 2){
     lrc = 203;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl /= cr;
   if(sl != 2){
     lrc = 204;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl /= sr;
   if(sl != 2){
     lrc = 205;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl /= ir;
   if(sl != 2){
     lrc = 206;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl /= lr;
   if(sl != 2){
     lrc = 207;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl /= ur;
   if(sl != 2){
     lrc = 208;
     if(prlc) printf(f,lrc);
   }
   sl = 5; fr = 2;
   sl /= fr;
   if(sl != 2){
     lrc = 209;
     if(prlc) printf(f,lrc);
   }
   sl = 5; dr = 2;
   sl /= dr;
   if(sl != 2){
     lrc = 210;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il /= cr;
   if(il != 2){
     lrc = 211;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il /= sr;
   if(il != 2){
     lrc = 212;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il /= ir;
   if(il != 2){
     lrc = 213;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il /= lr;
   if(il != 2){
     lrc = 214;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il /= ur;
   if(il != 2){
     lrc = 215;
     if(prlc) printf(f,lrc);
   }
   il = 5; fr = 2;
   il /= fr;
   if(il != 2){
     lrc = 216;
     if(prlc) printf(f,lrc);
   }
   il = 5; dr = 2;
   il /= dr;
   if(il != 2){
     lrc = 217;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll /= cr;
   if(ll != 2){
     lrc = 218;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll /= sr;
   if(ll != 2){
     lrc = 219;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll /= ir;
   if(ll != 2){
     lrc = 220;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll /= lr;
   if(ll != 2){
     lrc = 221;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll /= ur;
   if(ll != 2){
     lrc = 222;
     if(prlc) printf(f,lrc);
   }
   ll = 5; fr = 2;
   ll /= fr;
   if(ll != 2){
     lrc = 223;
     if(prlc) printf(f,lrc);
   }
   ll = 5; dr = 2;
   ll /= dr;
   if(ll != 2){
     lrc = 224;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul /= cr;
   if(ul != 2){
     lrc = 225;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul /= sr;
   if(ul != 2){
     lrc = 226;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul /= ir;
   if(ul != 2){
     lrc = 227;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul /= lr;
   if(ul != 2){
     lrc = 228;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul /= ur;
   if(ul != 2){
     lrc = 229;
     if(prlc) printf(f,lrc);
   }
   ul = 5; fr = 2;
   ul /= fr;
   if(ul != 2){
     lrc = 230;
     if(prlc) printf(f,lrc);
   }
   ul = 5; dr = 2;
   ul /= dr;
   if(ul != 2){
     lrc = 231;
     if(prlc) printf(f,lrc);
   }
   fl = 5; cr = 2;
   fl /= cr;
   if(fl != 2.5){
     lrc = 232;
     if(prlc) printf(f,lrc);
   }
   fl = 5; sr = 2;
   fl /= sr;
   if(fl != 2.5){
     lrc = 233;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ir = 2;
   fl /= ir;
   if(fl != 2.5){
     lrc = 234;
     if(prlc) printf(f,lrc);
   }
   fl = 5; lr = 2;
   fl /= lr;
   if(fl != 2.5){
     lrc = 235;
     if(prlc) printf(f,lrc);
   }
   fl = 5; ur = 2;
   fl /= ur;
   if(fl != 2.5){
     lrc = 236;
     if(prlc) printf(f,lrc);
   }
   fl = 5; fr = 2;
   fl /= fr;
   if(fl != 2.5){
     lrc = 237;
     if(prlc) printf(f,lrc);
   }
   fl = 5; dr = 2;
   fl /= dr;
   if(fl != 2.5){
     lrc = 238;
     if(prlc) printf(f,lrc);
   }
   dl = 5; cr = 2;
   dl /= cr;
   if(dl != 2.5){
     lrc = 239;
     if(prlc) printf(f,lrc);
   }
   dl = 5; sr = 2;
   dl /= sr;
   if(dl != 2.5){
     lrc = 240;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ir = 2;
   dl /= ir;
   if(dl != 2.5){
     lrc = 241;
     if(prlc) printf(f,lrc);
   }
   dl = 5; lr = 2;
   dl /= lr;
   if(dl != 2.5){
     lrc = 242;
     if(prlc) printf(f,lrc);
   }
   dl = 5; ur = 2;
   dl /= ur;
   if(dl != 2.5){
     lrc = 243;
     if(prlc) printf(f,lrc);
   }
   dl = 5; fr = 2;
   dl /= fr;
   if(dl != 2.5){
     lrc = 244;
     if(prlc) printf(f,lrc);
   }
   dl = 5; dr = 2;
   dl /= dr;
   if(dl != 2.5){
     lrc = 245;
     if(prlc) printf(f,lrc);
   }
   cl = 5; cr = 2;
   cl %= cr;
   if(cl != 1){
     lrc = 246;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl %= sr;
   if(cl != 1){
     lrc = 247;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl %= ir;
   if(cl != 1){
     lrc = 248;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl %= lr;
   if(cl != 1){
     lrc = 249;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl %= ur;
   if(cl != 1){
     lrc = 250;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl %= cr;
   if(sl != 1){
     lrc = 251;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl %= sr;
   if(sl != 1){
     lrc = 252;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl %= ir;
   if(sl != 1){
     lrc = 253;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl %= lr;
   if(sl != 1){
     lrc = 254;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl %= ur;
   if(sl != 1){
     lrc = 255;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il %= cr;
   if(il != 1){
     lrc = 256;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il %= sr;
   if(il != 1){
     lrc = 257;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il %= ir;
   if(il != 1){
     lrc = 258;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il %= lr;
   if(il != 1){
     lrc = 259;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il %= ur;
   if(il != 1){
     lrc = 260;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll %= cr;
   if(ll != 1){
     lrc = 261;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll %= sr;
   if(ll != 1){
     lrc = 262;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll %= ir;
   if(ll != 1){
     lrc = 263;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll %= lr;
   if(ll != 1){
     lrc = 264;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll %= ur;
   if(ll != 1){
     lrc = 265;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul %= cr;
   if(ul != 1){
     lrc = 266;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul %= sr;
   if(ul != 1){
     lrc = 267;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul %= ir;
   if(ul != 1){
     lrc = 268;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul %= lr;
   if(ul != 1){
     lrc = 269;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul %= ur;
   if(ul != 1){
     lrc = 270;
     if(prlc) printf(f,lrc);
   }
   cl = 5; cr = 2;
   cl >>= cr;
   if(cl != 1){
     lrc = 271;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl >>= sr;
   if(cl != 1){
     lrc = 272;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl >>= ir;
   if(cl != 1){
     lrc = 273;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl >>= lr;
   if(cl != 1){
     lrc = 274;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl >>= ur;
   if(cl != 1){
     lrc = 275;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl >>= cr;
   if(sl != 1){
     lrc = 276;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl >>= sr;
   if(sl != 1){
     lrc = 277;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl >>= ir;
   if(sl != 1){
     lrc = 278;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl >>= lr;
   if(sl != 1){
     lrc = 279;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl >>= ur;
   if(sl != 1){
     lrc = 280;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il >>= cr;
   if(il != 1){
     lrc = 281;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il >>= sr;
   if(il != 1){
     lrc = 282;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il >>= ir;
   if(il != 1){
     lrc = 283;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il >>= lr;
   if(il != 1){
     lrc = 284;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il >>= ur;
   if(il != 1){
     lrc = 285;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll >>= cr;
   if(ll != 1){
     lrc = 286;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll >>= sr;
   if(ll != 1){
     lrc = 287;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll >>= ir;
   if(ll != 1){
     lrc = 288;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll >>= lr;
   if(ll != 1){
     lrc = 289;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll >>= ur;
   if(ll != 1){
     lrc = 290;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul >>= cr;
   if(ul != 1){
     lrc = 291;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul >>= sr;
   if(ul != 1){
     lrc = 292;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul >>= ir;
   if(ul != 1){
     lrc = 293;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul >>= lr;
   if(ul != 1){
     lrc = 294;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul >>= ur;
   if(ul != 1){
     lrc = 295;
     if(prlc) printf(f,lrc);
   }
   cl = 5; cr = 2;
   cl <<= cr;
   if(cl != 20){
     lrc = 296;
     if(prlc) printf(f,lrc);
   }
   cl = 5; sr = 2;
   cl <<= sr;
   if(cl != 20){
     lrc = 297;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ir = 2;
   cl <<= ir;
   if(cl != 20){
     lrc = 298;
     if(prlc) printf(f,lrc);
   }
   cl = 5; lr = 2;
   cl <<= lr;
   if(cl != 20){
     lrc = 299;
     if(prlc) printf(f,lrc);
   }
   cl = 5; ur = 2;
   cl <<= ur;
   if(cl != 20){
     lrc = 300;
     if(prlc) printf(f,lrc);
   }
   sl = 5; cr = 2;
   sl <<= cr;
   if(sl != 20){
     lrc = 301;
     if(prlc) printf(f,lrc);
   }
   sl = 5; sr = 2;
   sl <<= sr;
   if(sl != 20){
     lrc = 302;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ir = 2;
   sl <<= ir;
   if(sl != 20){
     lrc = 303;
     if(prlc) printf(f,lrc);
   }
   sl = 5; lr = 2;
   sl <<= lr;
   if(sl != 20){
     lrc = 304;
     if(prlc) printf(f,lrc);
   }
   sl = 5; ur = 2;
   sl <<= ur;
   if(sl != 20){
     lrc = 305;
     if(prlc) printf(f,lrc);
   }
   il = 5; cr = 2;
   il <<= cr;
   if(il != 20){
     lrc = 306;
     if(prlc) printf(f,lrc);
   }
   il = 5; sr = 2;
   il <<= sr;
   if(il != 20){
     lrc = 307;
     if(prlc) printf(f,lrc);
   }
   il = 5; ir = 2;
   il <<= ir;
   if(il != 20){
     lrc = 308;
     if(prlc) printf(f,lrc);
   }
   il = 5; lr = 2;
   il <<= lr;
   if(il != 20){
     lrc = 309;
     if(prlc) printf(f,lrc);
   }
   il = 5; ur = 2;
   il <<= ur;
   if(il != 20){
     lrc = 310;
     if(prlc) printf(f,lrc);
   }
   ll = 5; cr = 2;
   ll <<= cr;
   if(ll != 20){
     lrc = 311;
     if(prlc) printf(f,lrc);
   }
   ll = 5; sr = 2;
   ll <<= sr;
   if(ll != 20){
     lrc = 312;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ir = 2;
   ll <<= ir;
   if(ll != 20){
     lrc = 313;
     if(prlc) printf(f,lrc);
   }
   ll = 5; lr = 2;
   ll <<= lr;
   if(ll != 20){
     lrc = 314;
     if(prlc) printf(f,lrc);
   }
   ll = 5; ur = 2;
   ll <<= ur;
   if(ll != 20){
     lrc = 315;
     if(prlc) printf(f,lrc);
   }
   ul = 5; cr = 2;
   ul <<= cr;
   if(ul != 20){
     lrc = 316;
     if(prlc) printf(f,lrc);
   }
   ul = 5; sr = 2;
   ul <<= sr;
   if(ul != 20){
     lrc = 317;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ir = 2;
   ul <<= ir;
   if(ul != 20){
     lrc = 318;
     if(prlc) printf(f,lrc);
   }
   ul = 5; lr = 2;
   ul <<= lr;
   if(ul != 20){
     lrc = 319;
     if(prlc) printf(f,lrc);
   }
   ul = 5; ur = 2;
   ul <<= ur;
   if(ul != 20){
     lrc = 320;
     if(prlc) printf(f,lrc);
   }
   cl = 12; cr = 10;
   cl &= cr;
   if(cl != 8){
     lrc = 321;
     if(prlc) printf(f,lrc);
   }
   cl = 12; sr = 10;
   cl &= sr;
   if(cl != 8){
     lrc = 322;
     if(prlc) printf(f,lrc);
   }
   cl = 12; ir = 10;
   cl &= ir;
   if(cl != 8){
     lrc = 323;
     if(prlc) printf(f,lrc);
   }
   cl = 12; lr = 10;
   cl &= lr;
   if(cl != 8){
     lrc = 324;
     if(prlc) printf(f,lrc);
   }
   cl = 12; ur = 10;
   cl &= ur;
   if(cl != 8){
     lrc = 325;
     if(prlc) printf(f,lrc);
   }
   sl = 12; cr = 10;
   sl &= cr;
   if(sl != 8){
     lrc = 326;
     if(prlc) printf(f,lrc);
   }
   sl = 12; sr = 10;
   sl &= sr;
   if(sl != 8){
     lrc = 327;
     if(prlc) printf(f,lrc);
   }
   sl = 12; ir = 10;
   sl &= ir;
   if(sl != 8){
     lrc = 328;
     if(prlc) printf(f,lrc);
   }
   sl = 12; lr = 10;
   sl &= lr;
   if(sl != 8){
     lrc = 329;
     if(prlc) printf(f,lrc);
   }
   sl = 12; ur = 10;
   sl &= ur;
   if(sl != 8){
     lrc = 330;
     if(prlc) printf(f,lrc);
   }
   il = 12; cr = 10;
   il &= cr;
   if(il != 8){
     lrc = 331;
     if(prlc) printf(f,lrc);
   }
   il = 12; sr = 10;
   il &= sr;
   if(il != 8){
     lrc = 332;
     if(prlc) printf(f,lrc);
   }
   il = 12; ir = 10;
   il &= ir;
   if(il != 8){
     lrc = 333;
     if(prlc) printf(f,lrc);
   }
   il = 12; lr = 10;
   il &= lr;
   if(il != 8){
     lrc = 334;
     if(prlc) printf(f,lrc);
   }
   il = 12; ur = 10;
   il &= ur;
   if(il != 8){
     lrc = 335;
     if(prlc) printf(f,lrc);
   }
   ll = 12; cr = 10;
   ll &= cr;
   if(ll != 8){
     lrc = 336;
     if(prlc) printf(f,lrc);
   }
   ll = 12; sr = 10;
   ll &= sr;
   if(ll != 8){
     lrc = 337;
     if(prlc) printf(f,lrc);
   }
   ll = 12; ir = 10;
   ll &= ir;
   if(ll != 8){
     lrc = 338;
     if(prlc) printf(f,lrc);
   }
   ll = 12; lr = 10;
   ll &= lr;
   if(ll != 8){
     lrc = 339;
     if(prlc) printf(f,lrc);
   }
   ll = 12; ur = 10;
   ll &= ur;
   if(ll != 8){
     lrc = 340;
     if(prlc) printf(f,lrc);
   }
   ul = 12; cr = 10;
   ul &= cr;
   if(ul != 8){
     lrc = 341;
     if(prlc) printf(f,lrc);
   }
   ul = 12; sr = 10;
   ul &= sr;
   if(ul != 8){
     lrc = 342;
     if(prlc) printf(f,lrc);
   }
   ul = 12; ir = 10;
   ul &= ir;
   if(ul != 8){
     lrc = 343;
     if(prlc) printf(f,lrc);
   }
   ul = 12; lr = 10;
   ul &= lr;
   if(ul != 8){
     lrc = 344;
     if(prlc) printf(f,lrc);
   }
   ul = 12; ur = 10;
   ul &= ur;
   if(ul != 8){
     lrc = 345;
     if(prlc) printf(f,lrc);
   }
   cl = 12; cr = 10;
   cl ^= cr;
   if(cl != 6){
     lrc = 346;
     if(prlc) printf(f,lrc);
   }
   cl = 12; sr = 10;
   cl ^= sr;
   if(cl != 6){
     lrc = 347;
     if(prlc) printf(f,lrc);
   }
   cl = 12; ir = 10;
   cl ^= ir;
   if(cl != 6){
     lrc = 348;
     if(prlc) printf(f,lrc);
   }
   cl = 12; lr = 10;
   cl ^= lr;
   if(cl != 6){
     lrc = 349;
     if(prlc) printf(f,lrc);
   }
   cl = 12; ur = 10;
   cl ^= ur;
   if(cl != 6){
     lrc = 350;
     if(prlc) printf(f,lrc);
   }
   sl = 12; cr = 10;
   sl ^= cr;
   if(sl != 6){
     lrc = 351;
     if(prlc) printf(f,lrc);
   }
   sl = 12; sr = 10;
   sl ^= sr;
   if(sl != 6){
     lrc = 352;
     if(prlc) printf(f,lrc);
   }
   sl = 12; ir = 10;
   sl ^= ir;
   if(sl != 6){
     lrc = 353;
     if(prlc) printf(f,lrc);
   }
   sl = 12; lr = 10;
   sl ^= lr;
   if(sl != 6){
     lrc = 354;
     if(prlc) printf(f,lrc);
   }
   sl = 12; ur = 10;
   sl ^= ur;
   if(sl != 6){
     lrc = 355;
     if(prlc) printf(f,lrc);
   }
   il = 12; cr = 10;
   il ^= cr;
   if(il != 6){
     lrc = 356;
     if(prlc) printf(f,lrc);
   }
   il = 12; sr = 10;
   il ^= sr;
   if(il != 6){
     lrc = 357;
     if(prlc) printf(f,lrc);
   }
   il = 12; ir = 10;
   il ^= ir;
   if(il != 6){
     lrc = 358;
     if(prlc) printf(f,lrc);
   }
   il = 12; lr = 10;
   il ^= lr;
   if(il != 6){
     lrc = 359;
     if(prlc) printf(f,lrc);
   }
   il = 12; ur = 10;
   il ^= ur;
   if(il != 6){
     lrc = 360;
     if(prlc) printf(f,lrc);
   }
   ll = 12; cr = 10;
   ll ^= cr;
   if(ll != 6){
     lrc = 361;
     if(prlc) printf(f,lrc);
   }
   ll = 12; sr = 10;
   ll ^= sr;
   if(ll != 6){
     lrc = 362;
     if(prlc) printf(f,lrc);
   }
   ll = 12; ir = 10;
   ll ^= ir;
   if(ll != 6){
     lrc = 363;
     if(prlc) printf(f,lrc);
   }
   ll = 12; lr = 10;
   ll ^= lr;
   if(ll != 6){
     lrc = 364;
     if(prlc) printf(f,lrc);
   }
   ll = 12; ur = 10;
   ll ^= ur;
   if(ll != 6){
     lrc = 365;
     if(prlc) printf(f,lrc);
   }
   ul = 12; cr = 10;
   ul ^= cr;
   if(ul != 6){
     lrc = 366;
     if(prlc) printf(f,lrc);
   }
   ul = 12; sr = 10;
   ul ^= sr;
   if(ul != 6){
     lrc = 367;
     if(prlc) printf(f,lrc);
   }
   ul = 12; ir = 10;
   ul ^= ir;
   if(ul != 6){
     lrc = 368;
     if(prlc) printf(f,lrc);
   }
   ul = 12; lr = 10;
   ul ^= lr;
   if(ul != 6){
     lrc = 369;
     if(prlc) printf(f,lrc);
   }
   ul = 12; ur = 10;
   ul ^= ur;
   if(ul != 6){
     lrc = 370;
     if(prlc) printf(f,lrc);
   }
   cl = 12; cr = 10;
   cl |= cr;
   if(cl != 14){
     lrc = 371;
     if(prlc) printf(f,lrc);
   }
   cl = 12; sr = 10;
   cl |= sr;
   if(cl != 14){
     lrc = 372;
     if(prlc) printf(f,lrc);
   }
   cl = 12; ir = 10;
   cl |= ir;
   if(cl != 14){
     lrc = 373;
     if(prlc) printf(f,lrc);
   }
   cl = 12; lr = 10;
   cl |= lr;
   if(cl != 14){
     lrc = 374;
     if(prlc) printf(f,lrc);
   }
   cl = 12; ur = 10;
   cl |= ur;
   if(cl != 14){
     lrc = 375;
     if(prlc) printf(f,lrc);
   }
   sl = 12; cr = 10;
   sl |= cr;
   if(sl != 14){
     lrc = 376;
     if(prlc) printf(f,lrc);
   }
   sl = 12; sr = 10;
   sl |= sr;
   if(sl != 14){
     lrc = 377;
     if(prlc) printf(f,lrc);
   }
   sl = 12; ir = 10;
   sl |= ir;
   if(sl != 14){
     lrc = 378;
     if(prlc) printf(f,lrc);
   }
   sl = 12; lr = 10;
   sl |= lr;
   if(sl != 14){
     lrc = 379;
     if(prlc) printf(f,lrc);
   }
   sl = 12; ur = 10;
   sl |= ur;
   if(sl != 14){
     lrc = 380;
     if(prlc) printf(f,lrc);
   }
   il = 12; cr = 10;
   il |= cr;
   if(il != 14){
     lrc = 381;
     if(prlc) printf(f,lrc);
   }
   il = 12; sr = 10;
   il |= sr;
   if(il != 14){
     lrc = 382;
     if(prlc) printf(f,lrc);
   }
   il = 12; ir = 10;
   il |= ir;
   if(il != 14){
     lrc = 383;
     if(prlc) printf(f,lrc);
   }
   il = 12; lr = 10;
   il |= lr;
   if(il != 14){
     lrc = 384;
     if(prlc) printf(f,lrc);
   }
   il = 12; ur = 10;
   il |= ur;
   if(il != 14){
     lrc = 385;
     if(prlc) printf(f,lrc);
   }
   ll = 12; cr = 10;
   ll |= cr;
   if(ll != 14){
     lrc = 386;
     if(prlc) printf(f,lrc);
   }
   ll = 12; sr = 10;
   ll |= sr;
   if(ll != 14){
     lrc = 387;
     if(prlc) printf(f,lrc);
   }
   ll = 12; ir = 10;
   ll |= ir;
   if(ll != 14){
     lrc = 388;
     if(prlc) printf(f,lrc);
   }
   ll = 12; lr = 10;
   ll |= lr;
   if(ll != 14){
     lrc = 389;
     if(prlc) printf(f,lrc);
   }
   ll = 12; ur = 10;
   ll |= ur;
   if(ll != 14){
     lrc = 390;
     if(prlc) printf(f,lrc);
   }
   ul = 12; cr = 10;
   ul |= cr;
   if(ul != 14){
     lrc = 391;
     if(prlc) printf(f,lrc);
   }
   ul = 12; sr = 10;
   ul |= sr;
   if(ul != 14){
     lrc = 392;
     if(prlc) printf(f,lrc);
   }
   ul = 12; ir = 10;
   ul |= ir;
   if(ul != 14){
     lrc = 393;
     if(prlc) printf(f,lrc);
   }
   ul = 12; lr = 10;
   ul |= lr;
   if(ul != 14){
     lrc = 394;
     if(prlc) printf(f,lrc);
   }
   ul = 12; ur = 10;
   ul |= ur;
   if(ul != 14){
     lrc = 395;
     if(prlc) printf(f,lrc);
   }
   if(lrc != 0) {
     rc = 1;
     if(pd0->flgd != 0) printf(s714er,1);
   }
   return rc;
}
s715(pd0)          /*  7.15 Comma operator     */
struct defs *pd0;
{
   static char s715er[] = "s715,er%d\n";
   static char qs715[8] = "s715   ";
   int rc;
   char *ps, *pt;
   int a, t, c, i;
   a = c = 0;
   ps = qs715;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* A pair of expressions separated by a comma is
        evaluated left to right and the value of the left
        expression is discarded.
                                                                */
   i = 1;
   if( i++,i++,i++,i++,++i != 6 ){
     if(pd0->flgd != 0) printf(s715er,1);
     rc = rc+1;
   }

        /* In contexts where the comma is given a special mean-
        ing, for example in a list of actual arguments to 
        functions (sic) and lists of initializers, the comma
        operator as described in this section can only appear
        in parentheses; for example

                f( a, (t=3, t+2), c)

        has three arguments, the second of which has the
        value 5.
                                                                */

   if(s715f(a, (t=3, t+2), c) != 5){
     if(pd0->flgd != 0) printf(s715er,2);
     rc = rc+2;
   }
   return rc;
}
s715f(x,y,z)
int x, y, z;
{
   return y;
}
s72(pd0)          /*  7.2  Unary operators  */
struct defs *pd0;
{
   static char s72er[] = "s72,er%d\n";
   static char qs72[8] = "s72    ";
   int rc;
   char *ps, *pt;
   int k, j, i, lrc;
   char c;
   short s;
   long l;
   unsigned u;
   double d;
   float f;
   ps = qs72;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* The *, denoting indirection, and the &, denoting a
        pointer, are duals of each other, and ought to behave as 
        such...                                                 */

   k = 2;
   if(*&*&k != 2){
     rc = rc+1;
     printf(s72er,1);
   }

        /* The unary minus has the conventional meaning.        */

   if(k+(-k) != 0){
     rc = rc+2;
     printf(s72er,2);
   }

        /*  The negation operator (!) has been thoroughly checked out,
        perhaps more thoroughly than any of the others. The ~ oper-
        ator gets us a ones complement.                         */

   k = 0;
   for(j=0;j<pd0->ibits;j++) k = (k<<1)|1;
   if(~k != 0){
     rc = rc+4;
     printf(s72er,4);
   }

        /*  Now we look at the ++ and -- operators, which can be
        used in either prefix or suffix form. With side
        effects they're loaded.                                 */

   k = 5;

   if( ++k != 6 || --k != 5
    || k++ != 5 || k-- != 6
    ||   k != 5 ){
     rc = rc+8;
     printf(s72er,8);
   }

        /*  An expression preceded by the parenthesised name of a
        data type causes conversion of the value of the expression
        to the named type. This construction is called a cast.
        Here, we check to see that all of the possible casts and
        their simple combinations are accepted by the compiler,
        and that they all produce a correct result for this sample
        of size one.                                            */

   c = 26;  l = 26;  d = 26.;
   s = 26;  u = 26; 
   i = 26;  f = 26.;

   lrc = 0;

   if( (char)s != 26 || (char)i != 26
    || (char)l != 26 || (char)u != 26
    || (char)f != 26 || (char)d != 26 ) lrc = lrc+1;

   if( (short)c != 26 || (short)i != 26
    || (short)l != 26 || (short)u != 26
    || (short)f != 26 || (short)d != 26) lrc = lrc+2;

   if( (int)c != 26 || (int)s != 26
    || (int)l != 26 || (int)u != 26
    || (int)f != 26 || (int)d != 26 ) lrc = lrc+4;

   if( (long)c != 26 || (long)s != 26
    || (long)i != 26 || (long)u != 26
    || (long)f != 26 || (long)d != 26 ) lrc = lrc+8;

   if( (unsigned)c != 26 || (unsigned)s != 26
    || (unsigned)i != 26 || (unsigned)l != 26
    || (unsigned)f != 26 || (unsigned)d != 26 ) lrc = lrc+16;

   if( (float)c != 26. || (float)s != 26.
    || (float)i != 26. || (float)l != 26.
    || (float)u != 26. || (float)d != 26. ) lrc = lrc+32;

   if( (double)c != 26. || (double)s != 26.
    || (double)i != 26. || (double)l != 26.
    || (double)u != 26. || (double)f != 26. ) lrc = lrc+64;

   if(lrc != 0){
     rc = rc+16;
     printf(s72er,16);
   }

        /*  The sizeof operator has been tested previously.     */

   return rc;
}
s757(pd0)          /* 7.5 Shift operators          */
                   /* 7.6 Relational operators     */
                   /* 7.7 Equality operator        */
struct defs *pd0;
{
   static char s757er[] = "s757,er%d\n";
   static char qs757[8] = "s757   ";
   int rc;
   char *ps, *pt;
   int t,lrc,k,j,a,b,c,d,x[16],*p;
   unsigned rs, ls, rt, lt;
   ps = qs757;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* The shift operators << and >> group left-to-right.
                                                                */

   t = 40;
   if(t<<3<<2 != 1280 || t>>3>>2 != 1){
     rc = rc+1;
     if(pd0->flgd != 0) printf(s757er,1);
   }

        /* In the following test, an n-bit unsigned consisting
        of all 1s is shifted right (resp. left) k bits, 0<=k<n.
        We expect to find k 0s followed by n-k 1s (resp. n-k 1s
        followed by k 0s). If not, we complain.
                                                                */

   lrc = 0;
   for(k=0; k<pd0->ubits; k++){
     rs = 1;
     ls = rs<<(pd0->ubits-1);

     rt = 0;
     lt = ~rt>>k;
     rt = ~rt<<k;

     for(j=0; j<pd0->ubits;j++){
       if((j<k) != ((rs&rt) == 0) || (j<k) != ((ls&lt) == 0)) lrc = 1;
       rs = rs<<1;
       ls = ls>>1;
     }
   }

   if(lrc != 0){
     rc = rc+2;
     if(pd0->flgd != 0) printf(s757er,2);
   }

        /* The relational operators group left-to-right, but this
        fact is not very useful; a<b<c does not mean what it 
        seems to...
                                                                */

   a = 3;
   b = 2;
   c = 1;

   if((a<b<c) != 1){
     rc = rc+4;
     if(pd0->flgd != 0) printf(s757er,4);
   }

        /* In general, we take note of the fact that if we got this
        far the relational operators have to be working. We test only
        that two pointers may be compared; the result depends on
        the relative locations in the address space of the 
        pointed-to objects.
                                                                */
   if( &x[1] == &x[0] ){
     rc = rc+8;
     if(pd0->flgd != 0) printf(s757er,8);
   }

   if( &x[1] < &x[0] ) if(pd0->flgm != 0)
     printf("Increasing array elements assigned to decreasing locations\n");

        /* a<b == c<d whenever a<b and c<d have the same 
        truth value.                                            */

   lrc = 0;

   for(j=0;j<16;j++) x[j] = 1;
   x[1] = 0;
   x[4] = 0;
   x[6] = 0;
   x[7] = 0;
   x[9] = 0;
   x[13] = 0;

   for(a=0;a<2;a++)
     for(b=0;b<2;b++)
       for(c=0;c<2;c++)
         for(d=0;d<2;d++)
           if((a<b==c<d) != x[8*a+4*b+2*c+d] ) lrc = 1;

   if(lrc != 0){
     rc = rc+16;
     if(pd0->flgd != 0) printf(s757er,16);
   }

        /* A pointer to which zero has been assigned will
        appear to be equal to zero.
                                                                */

   p = 0;

   if(p != 0){
     rc = rc+32;
     if(pd0->flgd != 0) printf(s757er,32);
   }

   return rc;
}
s7813(pd0)          /* 7.8 Bitwise AND operator
                       7.9 Bitwise OR operator
                       7.10 Bitwise exclusive OR operator
                       7.11 Logical AND operator
                       7.12 Logical OR operator
                       7.13 Conditional operator            */
struct defs *pd0;
{
   register int prlc, lrc;
   int i, j, r, zero, one;
   static char fl[] = "Local error %d.\n";
   static char s7813er[] = "s7813,er%d\n";
   static char qs7813[8] = "s7813  ";
   int rc;
   char *ps, *pt;
   ps = qs7813;
   pt = pd0->rfs;
   lrc = 0;
   rc = 0;
   prlc = pd0->flgl;
   while (*pt++ = *ps++);

        /* If bitwise AND, OR, and exclusive OR are to cause
        trouble, they will probably do so when they are used in
        an unusual context. The number of contexts in which
        they can be used is infinite, so to save time we select
        a finite subset: the set of all expressions of the form:

                item1 op item2

        where item1 and item2 are chosen from the set
        {char,short,long,unsigned,int} and op is one of {&,|,^}.
        We will use 12 and 10 as values for the items, as these
        values will fit into all data types on just about any
        imaginable machine, and the results after performing the
        bitwise operations on them are distinct for each operation,
        i.e.,

                12 | 10  -> 1100 | 1010  -> 1110 -> 14
                12 ^ 10  -> 1100 ^ 1010  -> 0110 ->  6
                12 & 10  -> 1100 & 1010  -> 1000 ->  8

        There are 75 such combinations:
                                                                */

   if(((char)12 & (char)10) !=  8) {lrc = 1;
      if(prlc) printf(fl,lrc);}
   if(((char)12 | (char)10) != 14) {lrc = 2;
      if(prlc) printf(fl,lrc);}
   if(((char)12 ^ (char)10) !=  6) {lrc = 3;
      if(prlc) printf(fl,lrc);}
   if(((char)12 & (short)10) !=  8) {lrc = 4;
      if(prlc) printf(fl,lrc);}
   if(((char)12 | (short)10) != 14) {lrc = 5;
      if(prlc) printf(fl,lrc);}
   if(((char)12 ^ (short)10) !=  6) {lrc = 6;
      if(prlc) printf(fl,lrc);}
   if(((char)12 & (long)10) !=  8) {lrc = 7;
      if(prlc) printf(fl,lrc);}
   if(((char)12 | (long)10) != 14) {lrc = 8;
      if(prlc) printf(fl,lrc);}
   if(((char)12 ^ (long)10) !=  6) {lrc = 9;
      if(prlc) printf(fl,lrc);}
   if(((char)12 & (unsigned)10) !=  8) {lrc = 10;
      if(prlc) printf(fl,lrc);}
   if(((char)12 | (unsigned)10) != 14) {lrc = 11;
      if(prlc) printf(fl,lrc);}
   if(((char)12 ^ (unsigned)10) !=  6) {lrc = 12;
      if(prlc) printf(fl,lrc);}
   if(((char)12 & (int)10) !=  8) {lrc = 13;
      if(prlc) printf(fl,lrc);}
   if(((char)12 | (int)10) != 14) {lrc = 14;
      if(prlc) printf(fl,lrc);}
   if(((char)12 ^ (int)10) !=  6) {lrc = 15;
      if(prlc) printf(fl,lrc);}
   if(((short)12 & (char)10) !=  8) {lrc = 16;
      if(prlc) printf(fl,lrc);}
   if(((short)12 | (char)10) != 14) {lrc = 17;
      if(prlc) printf(fl,lrc);}
   if(((short)12 ^ (char)10) !=  6) {lrc = 18;
      if(prlc) printf(fl,lrc);}
   if(((short)12 & (short)10) !=  8) {lrc = 16;
      if(prlc) printf(fl,lrc);}
   if(((short)12 | (short)10) != 14) {lrc = 20;
      if(prlc) printf(fl,lrc);}
   if(((short)12 ^ (short)10) !=  6) {lrc = 21;
      if(prlc) printf(fl,lrc);}
   if(((short)12 & (long)10) !=  8) {lrc = 22;
      if(prlc) printf(fl,lrc);}
   if(((short)12 | (long)10) != 14) {lrc = 23;
      if(prlc) printf(fl,lrc);}
   if(((short)12 ^ (long)10) !=  6) {lrc = 24;
      if(prlc) printf(fl,lrc);}
   if(((short)12 & (unsigned)10) !=  8) {lrc = 25;
      if(prlc) printf(fl,lrc);}
   if(((short)12 | (unsigned)10) != 14) {lrc = 26;
      if(prlc) printf(fl,lrc);}
   if(((short)12 ^ (unsigned)10) !=  6) {lrc = 27;
      if(prlc) printf(fl,lrc);}
   if(((short)12 & (int)10) !=  8) {lrc = 28;
      if(prlc) printf(fl,lrc);}
   if(((short)12 | (int)10) != 14) {lrc = 26;
      if(prlc) printf(fl,lrc);}
   if(((short)12 ^ (int)10) !=  6) {lrc = 30;
      if(prlc) printf(fl,lrc);}
   if(((long)12 & (char)10) !=  8) {lrc = 31;
      if(prlc) printf(fl,lrc);}
   if(((long)12 | (char)10) != 14) {lrc = 32;
      if(prlc) printf(fl,lrc);}
   if(((long)12 ^ (char)10) !=  6) {lrc = 33;
      if(prlc) printf(fl,lrc);}
   if(((long)12 & (short)10) !=  8) {lrc = 34;
      if(prlc) printf(fl,lrc);}
   if(((long)12 | (short)10) != 14) {lrc = 35;
      if(prlc) printf(fl,lrc);}
   if(((long)12 ^ (short)10) !=  6) {lrc = 36;
      if(prlc) printf(fl,lrc);}
   if(((long)12 & (long)10) !=  8) {lrc = 37;
      if(prlc) printf(fl,lrc);}
   if(((long)12 | (long)10) != 14) {lrc = 38;
      if(prlc) printf(fl,lrc);}
   if(((long)12 ^ (long)10) !=  6) {lrc = 39;
      if(prlc) printf(fl,lrc);}
   if(((long)12 & (unsigned)10) !=  8) {lrc = 40;
      if(prlc) printf(fl,lrc);}
   if(((long)12 | (unsigned)10) != 14) {lrc = 41;
      if(prlc) printf(fl,lrc);}
   if(((long)12 ^ (unsigned)10) !=  6) {lrc = 42;
      if(prlc) printf(fl,lrc);}
   if(((long)12 & (int)10) !=  8) {lrc = 43;
      if(prlc) printf(fl,lrc);}
   if(((long)12 | (int)10) != 14) {lrc = 44;
      if(prlc) printf(fl,lrc);}
   if(((long)12 ^ (int)10) !=  6) {lrc = 45;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 & (char)10) !=  8) {lrc = 46;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 | (char)10) != 14) {lrc = 47;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 ^ (char)10) !=  6) {lrc = 48;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 & (short)10) !=  8) {lrc = 49;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 | (short)10) != 14) {lrc = 50;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 ^ (short)10) !=  6) {lrc = 51;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 & (long)10) !=  8) {lrc = 52;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 | (long)10) != 14) {lrc = 53;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 ^ (long)10) !=  6) {lrc = 54;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 & (unsigned)10) !=  8) {lrc = 55;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 | (unsigned)10) != 14) {lrc = 56;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 ^ (unsigned)10) !=  6) {lrc = 57;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 & (int)10) !=  8) {lrc = 58;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 | (int)10) != 14) {lrc = 56;
      if(prlc) printf(fl,lrc);}
   if(((unsigned)12 ^ (int)10) !=  6) {lrc = 60;
      if(prlc) printf(fl,lrc);}
   if(((int)12 & (char)10) !=  8) {lrc = 61;
      if(prlc) printf(fl,lrc);}
   if(((int)12 | (char)10) != 14) {lrc = 62;
      if(prlc) printf(fl,lrc);}
   if(((int)12 ^ (char)10) !=  6) {lrc = 63;
      if(prlc) printf(fl,lrc);}
   if(((int)12 & (short)10) !=  8) {lrc = 64;
      if(prlc) printf(fl,lrc);}
   if(((int)12 | (short)10) != 14) {lrc = 65;
      if(prlc) printf(fl,lrc);}
   if(((int)12 ^ (short)10) !=  6) {lrc = 66;
      if(prlc) printf(fl,lrc);}
   if(((int)12 & (long)10) !=  8) {lrc = 67;
      if(prlc) printf(fl,lrc);}
   if(((int)12 | (long)10) != 14) {lrc = 68;
      if(prlc) printf(fl,lrc);}
   if(((int)12 ^ (long)10) !=  6) {lrc = 69;
      if(prlc) printf(fl,lrc);}
   if(((int)12 & (unsigned)10) !=  8) {lrc = 70;
      if(prlc) printf(fl,lrc);}
   if(((int)12 | (unsigned)10) != 14) {lrc = 71;
      if(prlc) printf(fl,lrc);}
   if(((int)12 ^ (unsigned)10) !=  6) {lrc = 72;
      if(prlc) printf(fl,lrc);}
   if(((int)12 & (int)10) !=  8) {lrc = 73; if(prlc) printf(fl,lrc);}
   if(((int)12 | (int)10) != 14) {lrc = 74; if(prlc) printf(fl,lrc);}
   if(((int)12 ^ (int)10) !=  6) {lrc = 75; if(prlc) printf(fl,lrc);}

   if(lrc != 0){
     if(pd0->flgd != 0) printf(s7813er,1);
     rc = rc+1;
   }

        /* The && operator groups left to right. It returns 1
        if both of the operands are nonzero; 0 otherwise.
        It guarantees left to right evaluation; moreover, the
        second operand is not evaluated if the value of the 
        first operand is 0.
                                                                */

   lrc = 0;
   i = j = 0;

   r = i++ && j++;
    if(i!=1) {lrc = 1; if(prlc) printf(fl,lrc);}
    if(j!=0) {lrc = 2; if(prlc) printf(fl,lrc);}
    if(r!=0) {lrc = 3; if(prlc) printf(fl,lrc);}
   r = i && j++;
    if(i!=1) {lrc = 4; if(prlc) printf(fl,lrc);}
    if(j!=1) {lrc = 5; if(prlc) printf(fl,lrc);}
    if(r!=0) {lrc = 6; if(prlc) printf(fl,lrc);}
   r = i-- && j;
    if(i!=0) {lrc = 7; if(prlc) printf(fl,lrc);}
    if(j!=1) {lrc = 8; if(prlc) printf(fl,lrc);}
    if(r!=1) {lrc = 9; if(prlc) printf(fl,lrc);}
   r = i && j--;
    if(i!=0) {lrc = 10; if(prlc) printf(fl,lrc);}
    if(j!=1) {lrc = 11; if(prlc) printf(fl,lrc);}
    if(r!=0) {lrc = 12; if(prlc) printf(fl,lrc);}

   if(lrc!=0){
     if(pd0->flgd != 0) printf(s7813er,2);
     rc = rc+2;
   }

        /* The || operator groups left to right. It returns 1
        if either of its operands is nonzero; 0 otherwise. It
        guarantees left to right evaluation; moreover, the second
        operand is not evaluated if the value of the first 
        operand is nonzero.
                                                                */

   lrc = 0;
   i = j = 0;
   r = i++ || j;
    if(i!=1) {lrc = 1; if(prlc) printf(fl,lrc);}
    if(j!=0) {lrc = 2; if(prlc) printf(fl,lrc);}
    if(r!=0) {lrc = 3; if(prlc) printf(fl,lrc);}
   r = j++ || i;
    if(i!=1) {lrc = 4; if(prlc) printf(fl,lrc);}
    if(j!=1) {lrc = 5; if(prlc) printf(fl,lrc);}
    if(r!=1) {lrc = 6; if(prlc) printf(fl,lrc);}
   r = i-- || j--;
    if(i!=0) {lrc = 7; if(prlc) printf(fl,lrc);}
    if(j!=1) {lrc = 8; if(prlc) printf(fl,lrc);}
    if(r!=1) {lrc = 9; if(prlc) printf(fl,lrc);}
   r = i || j--;
    if(i!=0) {lrc = 10; if(prlc) printf(fl,lrc);}
    if(j!=0) {lrc = 11; if(prlc) printf(fl,lrc);}
    if(r!=1) {lrc = 12; if(prlc) printf(fl,lrc);}

   if(lrc!=0){
     if(pd0->flgd != 0) printf(s7813er,4);
     rc = rc+4;
   }

        /* Conditional expressions group right to left.  */

   i = j = 0;
   zero = 0;
   one = 1;
   r = one?zero:one?i++:j++;
   if(r!=0 || i!=0 || j!=0){
     if(pd0->flgd != 0) printf(s7813er,8);
     rc = rc+8;
   }

        /* The first expression is evaluated and if it is non-
        zero, the result is the value of the second expression;
        otherwise, that of the third expression.
                                                                */

   if((one?zero:1) != 0 || (zero?1:zero) != 0){
     if(pd0->flgd != 0) printf(s7813er,16);
     rc = rc+16;
   }
   return rc;
}
s81(pd0)              /* 8.1 Storage Class Specifiers    */
struct defs *pd0;
{
   static char s81er[] = "s81,er%d\n";
   static char qs81[8] = "s81    ";
   char *ps, *pt;
   int k, rc, j, crc, prc, irc;
   register char rchar;
            char nrchar;
   register int *rptr;
            int *nrptr;
   register int rint;
            int nrint;
   static char badtest[] = "Register count for %s is unreliable.\n";
   static char goodtest[] = "%d registers assigned to %s variables.\n";
   
   rc = 0;
   crc = 0;
   prc = 0;
   irc = 0;
   ps = qs81;
   pt = pd0->rfs;

   while(*pt++ = *ps++);

/*    The storage class specifiers are:

        auto
        static
        extern
        register
        typedef

      The first three of these were treated earlier, in s4. The last
   will be checked in s88. "Register" remains. 

      There are three flavors of register, viz., char, int and pointer.
   We wish first to ascertain that the representations as register
   are consistent with the corresponding nonregister representations.
                                                                 */

   k = 1;
   for (j=0; j<50; j++){
     rchar = k;
     nrchar = k;
     rptr = &k;
     nrptr = &k;
     rint = k;
     nrint = k;

     if ( rchar != nrchar ) crc = 1;
     if ( rptr != nrptr ) prc = 1;
     if ( rint != nrint ) irc = 1;
     k = k<<1;
   }

   if ( crc != 0 ) {
     rc = rc+1;
     if( pd0 -> flgd != 0 ) printf(s81er,1);
   }

   if ( prc != 0 ) {
     rc = rc+2;
     if( pd0 -> flgd != 0 ) printf(s81er,2);
   }

   if ( irc != 0 ) {
     rc = rc+4;
     if( pd0 -> flgd != 0 ) printf(s81er,4);
   }

/*   Now we check to see if variables are actually being assigned
     to registers.                       */

   k = regc();
   if ( pd0->flgm != 0 ) {
     if ( k < 0 ) printf(badtest,"char");
     else printf(goodtest,k,"char");
   }

   k = regp();
   if ( pd0->flgm != 0 ) {
     if ( k<0 ) printf(badtest,"pointer");
     else printf(goodtest,k,"pointer");
   }

   k = regi();
   if ( pd0->flgm != 0 ) {
     if ( k<0 ) printf(badtest,"int");
     else printf(goodtest,k,"int");
   }

   return rc;
}
regc() {     /*   char to register assignment   */
/*   Testing a variable whose storage class has been spec-
ified as "register" is somewhat tricky, but it can be done in a 
fairly reliable fashion by taking advantage of our knowledge of the
ways in which compilers operate. If we declare a collection of vari-
ables of the same storage class, we would expect that, when storage
for these variables is actually allocated, the variables will be 
bunched together and ordered according to one of the following
criteria:

     (a) the order in which they were defined.
     (b) the order in which they are used.
     (c) alphabetically.
     (d) the order in which they appear in the compiler's
         symbol table.
     (e) some other way.

     Hence, if we define a sequence of variables in close alpha-
betical order, and use them in the same order in which we define
them, we would expect the differences between the addresses of
successive variables to be constant, except in case (d) where the
symbol table is a hash table, or in case (e). If a subsequence in
the middle of this sequence is selected, and for this subsequence,
every other variable is specified to be "register", and address
differences are taken between adjacent nonregister variables, we would
still expect to find constant differences if the "register" vari-
ables were actually assigned to registers, and some other diff-
erences if they were not. Specifically, if we had N variables 
specified as "register" of which the first n were actually ass-
igned to registers, we would expect the sequence of differences
to consist of a number of occurrences of some number, followed by
N-n occurrences of some other number, followed by several occurr-
ences of the first number. If we get a sequence like this, we can
determine, by simple subtraction, how many (if any) variables are
being assigned to registers. If we get some other sequence, we know
that the test is invalid.                                     */

            char r00;
            char r01;
            char r02;
            char r03;
   register char r04;
            char r05;
   register char r06;
            char r07;
   register char r08;
            char r09;
   register char r10;
            char r11;
   register char r12;
            char r13;
   register char r14;
            char r15;
   register char r16;
            char r17;
   register char r18;
            char r19;
   register char r20;
            char r21;
   register char r22;
            char r23;
   register char r24;
            char r25;
   register char r26;
            char r27;
   register char r28;
            char r29;
   register char r30;
            char r31;
   register char r32;
            char r33;
   register char r34;
            char r35;
            char r36;
            char r37;
            char r38;

   int s, n1, n2, nr, j, d[22];
   r00 = 0;
   r01 = 1;
   r02 = 2;
   r03 = 3;
   r04 = 4;
   r05 = 5;
   r06 = 6;
   r07 = 7;
   r08 = 8;
   r09 = 9;
   r10 = 10;
   r11 = 11;
   r12 = 12;
   r13 = 13;
   r14 = 14;
   r15 = 15;
   r16 = 16;
   r17 = 17;
   r18 = 18;
   r19 = 19;
   r20 = 20;
   r21 = 21;
   r22 = 22;
   r23 = 23;
   r24 = 24;
   r25 = 25;
   r26 = 26;
   r27 = 27;
   r28 = 28;
   r29 = 29;
   r30 = 30;
   r31 = 31;
   r32 = 32;
   r33 = 33;
   r34 = 34;
   r35 = 35;
   r36 = 36;
   r37 = 37;
   r38 = 38;

   d[0] = &r01 - &r00;
   d[1] = &r02 - &r01;
   d[2] = &r03 - &r02;
   d[3] = &r05 - &r03;
   d[4] = &r07 - &r05;
   d[5] = &r09 - &r07;
   d[6] = &r11 - &r09;
   d[7] = &r13 - &r11;
   d[8] = &r15 - &r13;
   d[9] = &r17 - &r15;
   d[10] = &r19 - &r17;
   d[11] = &r21 - &r19;
   d[12] = &r23 - &r21;
   d[13] = &r25 - &r23;
   d[14] = &r27 - &r25;
   d[15] = &r29 - &r27;
   d[16] = &r31 - &r29;
   d[17] = &r33 - &r31;
   d[18] = &r35 - &r33;
   d[19] = &r36 - &r35;
   d[20] = &r37 - &r36;
   d[21] = &r38 - &r37;


/*   The following FSM analyzes the string of differences. It accepts
strings of the form a+b+a+ and returns 16 minus the number of bs, 
which is the number of variables that actually got into registers.
Otherwise it signals rejection by returning -1., indicating that the
test is unreliable.              */

   n1 = d[0];
   s = 1;

   for (j=0; j<22; j++)
     switch (s) {
       case 1: if (d[j] != n1) {
                n2 = d[j];
                s = 2;
                nr = 1;
               }
               break;
       case 2: if (d[j] == n1) {
                s = 3;
                break;
               }
               if (d[j] == n2) {
                nr = nr+1;
                break;
               }
               s = 4;
               break;
       case 3: if (d[j] != n1) s = 4;
               break;
     }
   ;

   if (s == 3) return 16-nr;
   else return -1;
}
regi() {     /*   int to register assignment    */
/*   Testing a variable whose storage class has been spec-
ified as "register" is somewhat tricky, but it can be done in a 
fairly reliable fashion by taking advantage of our knowledge of the
ways in which compilers operate. If we declare a collection of vari-
ables of the same storage class, we would expect that, when storage
for these variables is actually allocated, the variables will be 
bunched together and ordered according to one of the following
criteria:

     (a) the order in which they were defined.
     (b) the order in which they are used.
     (c) alphabetically.
     (d) the order in which they appear in the compiler's
         symbol table.
     (e) some other way.

     Hence, if we define a sequence of variables in close alpha-
betical order, and use them in the same order in which we define
them, we would expect the differences between the addresses of
successive variables to be constant, except in case (d) where the
symbol table is a hash table, or in case (e). If a subsequence in
the middle of this sequence is selected, and for this subsequence,
every other variable is specified to be "register", and address
differences are taken between adjacent nonregister variables, we would
still expect to find constant differences if the "register" vari-
ables were actually assigned to registers, and some other diff-
erences if they were not. Specifically, if we had N variables 
specified as "register" of which the first n were actually ass-
igned to registers, we would expect the sequence of differences
to consist of a number of occurrences of some number, followed by
N-n occurrences of some other number, followed by several occurr-
ences of the first number. If we get a sequence like this, we can
determine, by simple subtraction, how many (if any) variables are
being assigned to registers. If we get some other sequence, we know
that the test is invalid.                                     */


            int r00;
            int r01;
            int r02;
            int r03;
   register int r04;
            int r05;
   register int r06;
            int r07;
   register int r08;
            int r09;
   register int r10;
            int r11;
   register int r12;
            int r13;
   register int r14;
            int r15;
   register int r16;
            int r17;
   register int r18;
            int r19;
   register int r20;
            int r21;
   register int r22;
            int r23;
   register int r24;
            int r25;
   register int r26;
            int r27;
   register int r28;
            int r29;
   register int r30;
            int r31;
   register int r32;
            int r33;
   register int r34;
            int r35;
            int r36;
            int r37;
            int r38;

   int s, n1, n2, nr, j, d[22];

   r00 = 0;
   r01 = 1;
   r02 = 2;
   r03 = 3;
   r04 = 4;
   r05 = 5;
   r06 = 6;
   r07 = 7;
   r08 = 8;
   r09 = 9;
   r10 = 10;
   r11 = 11;
   r12 = 12;
   r13 = 13;
   r14 = 14;
   r15 = 15;
   r16 = 16;
   r17 = 17;
   r18 = 18;
   r19 = 19;
   r20 = 20;
   r21 = 21;
   r22 = 22;
   r23 = 23;
   r24 = 24;
   r25 = 25;
   r26 = 26;
   r27 = 27;
   r28 = 28;
   r29 = 29;
   r30 = 30;
   r31 = 31;
   r32 = 32;
   r33 = 33;
   r34 = 34;
   r35 = 35;
   r36 = 36;
   r37 = 37;
   r38 = 38;

   d[0] = &r01 - &r00;
   d[1] = &r02 - &r01;
   d[2] = &r03 - &r02;
   d[3] = &r05 - &r03;
   d[4] = &r07 - &r05;
   d[5] = &r09 - &r07;
   d[6] = &r11 - &r09;
   d[7] = &r13 - &r11;
   d[8] = &r15 - &r13;
   d[9] = &r17 - &r15;
   d[10] = &r19 - &r17;
   d[11] = &r21 - &r19;
   d[12] = &r23 - &r21;
   d[13] = &r25 - &r23;
   d[14] = &r27 - &r25;
   d[15] = &r29 - &r27;
   d[16] = &r31 - &r29;
   d[17] = &r33 - &r31;
   d[18] = &r35 - &r33;
   d[19] = &r36 - &r35;
   d[20] = &r37 - &r36;
   d[21] = &r38 - &r37;


/*   The following FSM analyzes the string of differences. It accepts
strings of the form a+b+a+ and returns 16 minus the number of bs, 
which is the number of variables that actually got into registers.
Otherwise it signals rejection by returning -1., indicating that the
test is unreliable.              */

   n1 = d[0];
   s = 1;

   for (j=0; j<22; j++)
     switch (s) {
       case 1: if (d[j] != n1) {
                n2 = d[j];
                s = 2;
                nr = 1;
               }
               break;
       case 2: if (d[j] == n1) {
                s = 3;
                break;
               }
               if (d[j] == n2) {
                nr = nr+1;
                break;
               }
               s = 4;
               break;
       case 3: if (d[j] != n1) s = 4;
               break;
     }
   ;

   if (s == 3) return 16-nr;
   else return -1;
}
regp() {     /*   pointer to register assignment   */
/*   Testing a variable whose storage class has been spec-
ified as "register" is somewhat tricky, but it can be done in a 
fairly reliable fashion by taking advantage of our knowledge of the
ways in which compilers operate. If we declare a collection of vari-
ables of the same storage class, we would expect that, when storage
for these variables is actually allocated, the variables will be 
bunched together and ordered according to one of the following
criteria:

     (a) the order in which they were defined.
     (b) the order in which they are used.
     (c) alphabetically.
     (d) the order in which they appear in the compiler's
         symbol table.
     (e) some other way.

     Hence, if we define a sequence of variables in close alpha-
betical order, and use them in the same order in which we define
them, we would expect the differences between the addresses of
successive variables to be constant, except in case (d) where the
symbol table is a hash table, or in case (e). If a subsequence in
the middle of this sequence is selected, and for this subsequence,
every other variable is specified to be "register", and address
differences are taken between adjacent nonregister variables, we would
still expect to find constant differences if the "register" vari-
ables were actually assigned to registers, and some other diff-
erences if they were not. Specifically, if we had N variables 
specified as "register" of which the first n were actually ass-
igned to registers, we would expect the sequence of differences
to consist of a number of occurrences of some number, followed by
N-n occurrences of some other number, followed by several occurr-
ences of the first number. If we get a sequence like this, we can
determine, by simple subtraction, how many (if any) variables are
being assigned to registers. If we get some other sequence, we know
that the test is invalid.                                     */


            int *r00;
            int *r01;
            int *r02;
            int *r03;
   register int *r04;
            int *r05;
   register int *r06;
            int *r07;
   register int *r08;
            int *r09;
   register int *r10;
            int *r11;
   register int *r12;
            int *r13;
   register int *r14;
            int *r15;
   register int *r16;
            int *r17;
   register int *r18;
            int *r19;
   register int *r20;
            int *r21;
   register int *r22;
            int *r23;
   register int *r24;
            int *r25;
   register int *r26;
            int *r27;
   register int *r28;
            int *r29;
   register int *r30;
            int *r31;
   register int *r32;
            int *r33;
   register int *r34;
            int *r35;
            int *r36;
            int *r37;
            int *r38;

   int s, n1, n2, nr, j, d[22];

   r00 = (int *)&r00;
   r01 = (int *)&r01;
   r02 = (int *)&r02;
   r03 = (int *)&r03;
   r04 = (int *)&r05;
   r05 = (int *)&r05;
   r06 = (int *)&r07;
   r07 = (int *)&r07;
   r08 = (int *)&r09;
   r09 = (int *)&r09;
   r10 = (int *)&r11;
   r11 = (int *)&r11;
   r12 = (int *)&r13;
   r13 = (int *)&r13;
   r14 = (int *)&r15;
   r15 = (int *)&r15;
   r16 = (int *)&r17;
   r17 = (int *)&r17;
   r18 = (int *)&r19;
   r19 = (int *)&r19;
   r20 = (int *)&r21;
   r21 = (int *)&r21;
   r22 = (int *)&r23;
   r23 = (int *)&r23;
   r24 = (int *)&r25;
   r25 = (int *)&r25;
   r26 = (int *)&r27;
   r27 = (int *)&r27;
   r28 = (int *)&r29;
   r29 = (int *)&r29;
   r30 = (int *)&r31;
   r31 = (int *)&r31;
   r32 = (int *)&r33;
   r33 = (int *)&r33;
   r34 = (int *)&r35;
   r35 = (int *)&r35;
   r36 = (int *)&r36;
   r37 = (int *)&r37;
   r38 = (int *)&r38;

   d[0] = &r01 - &r00;
   d[1] = &r02 - &r01;
   d[2] = &r03 - &r02;
   d[3] = &r05 - &r03;
   d[4] = &r07 - &r05;
   d[5] = &r09 - &r07;
   d[6] = &r11 - &r09;
   d[7] = &r13 - &r11;
   d[8] = &r15 - &r13;
   d[9] = &r17 - &r15;
   d[10] = &r19 - &r17;
   d[11] = &r21 - &r19;
   d[12] = &r23 - &r21;
   d[13] = &r25 - &r23;
   d[14] = &r27 - &r25;
   d[15] = &r29 - &r27;
   d[16] = &r31 - &r29;
   d[17] = &r33 - &r31;
   d[18] = &r35 - &r33;
   d[19] = &r36 - &r35;
   d[20] = &r37 - &r36;
   d[21] = &r38 - &r37;


/*   The following FSM analyzes the string of differences. It accepts
strings of the form a+b+a+ and returns 16 minus the number of bs, 
which is the number of variables that actually got into registers.
Otherwise it signals rejection by returning -1., indicating that the
test is unreliable.              */

   n1 = d[0];
   s = 1;
   for (j=0; j<22; j++)
     switch (s) {
       case 1: if (d[j] != n1) {
                n2 = d[j];
                s = 2;
                nr = 1;
               }
               break;
       case 2: if (d[j] == n1) {
                s = 3;
                break;
               }
               if (d[j] == n2) {
                nr = nr+1;
                break;
               }
               s = 4;
               break;
       case 3: if (d[j] != n1) s = 4;
               break;
     }
   ;

   if (s == 3) return 16-nr;
   else return -1;
}
s84(pd0)          /*  8.4 Meaning of declarators   */
struct defs *pd0;
{
   int *ip, i, *fip(), (*pfi)(), j, k, array(), glork();
   static int x3d[3][5][7];
   float fa[17], *afp[17], sum;
   static char s84er[] = "s84,er%d\n";
   static char qs84[8] = "s84    ";
   int rc;
   char *ps, *pt;
   ps = qs84;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* The more common varieties of declarators have al-
        ready been touched upon, some more than others. It
        is useful to compare *fip() and (*pfi)().
                                                                */

   ip = fip(3);
   if(*ip != 3){
     if(pd0->flgd != 0) printf(s84er,1);
     rc = rc+1;
   }

   pfi = glork;
   if((*pfi)(4) != 4){
     if(pd0->flgd != 0) printf(s84er,2);
     rc = rc+2;
   }

        /* Float fa[17] declares an array of floating point
        numbers, and *afp[17] declares an array of pointers
        to floats.
                                                                */

   for(j=0; j<17; j++){
     fa[j] = j;
     afp[j] = &fa[j];
   }

   sum = 0.;
   for(j=0; j<17; j++) sum += *afp[j];
   if(sum != 136){
     if(pd0->flgd != 0) printf(s84er,4);
     rc = rc+4;
   }

        /*  static int x3d[3][5][7] declares a static three
        dimensional array of integers, with rank 3x5x7.
        In complete detail, x3d is an array of three items;
        each item is an array of five arrays, and each of 
        the latter arrays is an array of seven integers.
        Any of the expressions x3d, x3d[i], x3d[i][j],
        and x3d[i][j][k] may reasonably appear in an express-
        ion. The first three have type "array"; the last has
        type int.
                                                                */

   for (i=0; i<3; i++)
     for (j=0; j<5; j++)
       for (k=0; k<7; k++)
         x3d[i][j][k] = i*35+j*7+k;

   i = 1; j = 2; k = 3;

   if( array(x3d,105,0)
      +array(x3d[i],35,35)
      +array(x3d[i][j],7,49)
      +      x3d[i][j][k]-52){
 
      if(pd0->flgd != 0) printf(s84er,8);
      rc = rc+8;
   }

   return rc;
}
array(a,size,start)
int a[], size, start;
{
   int i;
   for(i=0; i<size; i++)
     if(a[i] != i+start) return 1;

   return 0;
}
int *fip(x)
int x;
{
   static int y;
   y = x;
   return &y;
}
glork(x)
int x;
{return x;}
s85(pd0)          /*  8.5 Structure and union declarations   */
struct defs *pd0;
{
   static char s85er[] = "s85,er%d\n";
   static char qs85[8] = "s85    ";
   int rc;
   char *ps, *pt;
   
   struct tnode {
     char tword[20];
     int count;
     struct tnode *left;
     struct tnode *right;
   };

   struct tnode s1, s2, *sp;

   struct{
     char cdummy;
     char c;
   } sc;

   struct{
     char cdummy;
     short s;
   } ss;

   struct{
     char cdummy;
     int i;
   } si;

   struct{
     char cdummy;
     long l;
   } sl;

   struct{
     char cdummy;
     unsigned u;
   } su;

   struct{
     char cdummy;
     float f;
   } sf;

   struct{
     char cdummy;
     double d;
   } sd;

   int diff[7], j;

   static char *type[] = {
     "char",
     "short",
     "int",
     "long",
     "unsigned",
     "float",
     "double"
   };

   static char aln[] = " alignment: ";

   struct{
     int twobit:2;
     int       :1;
     int threebit:3;
     int onebit:1;
   } s3;

   union{
     char u1[30];
     short u2[30];
     int u3[30];
     long u4[30];
     unsigned u5[30];
     float u6[30];
     double u7[30];
   } u0;

   ps = qs85;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* Within a structure, the objects declared have
        addresses which increase as their declarations are
        read left to right.
                                                                */

   if( (char *)&s1.count - &s1.tword[0] <= 0
     ||(char *)&s1.left - (char *)&s1.count <= 0
     ||(char *)&s1.right - (char *)&s1.left <= 0){
     if(pd0->flgd != 0) printf(s85er,1);
     rc = rc+1;
   }

        /* Each non-field member of a structure begins on an
        addressing boundary appropriate to its type.
                                                                */

   diff[0] = &sc.c - &sc.cdummy;
   diff[1] = (char *)&ss.s - &ss.cdummy;
   diff[2] = (char *)&si.i - &si.cdummy;
   diff[3] = (char *)&sl.l - &sl.cdummy;
   diff[4] = (char *)&su.u - &su.cdummy;
   diff[5] = (char *)&sf.f - &sf.cdummy;
   diff[6] = (char *)&sd.d - &sd.cdummy;

   if(pd0->flgm != 0)
    for(j=0; j<7; j++)
     printf("%s%s%d\n",type[j],aln,diff[j]);

        /* Field specifications are highly implementation de-
        pendent. About the only thing we can do here is to
        check is that the compiler accepts the field constructs,
        and that they seem to work, after a fashion, at
        run time...
                                                                */

   s3.threebit = 7;
   s3.twobit = s3.threebit;
   s3.threebit = s3.twobit;

   if(s3.threebit != 3){
     if(s3.threebit == -1){
       if(pd0->flgm != 0) printf("Sign extension in fields\n");
     }
     else{
       if(pd0->flgd != 0) printf(s85er,2);
       rc = rc+2;
     }
   }

   s3.onebit = 1;
   if(s3.onebit != 1){
     if(pd0->flgm != 0)
      printf("Be especially careful with 1-bit fields!\n");
   }

        /* A union may be thought of as a structure all of whose
        members begin at offset 0 and whose size is sufficient
        to contain any of its members.
                                                                */

   if( (char *)u0.u1 - (char *)&u0 != 0
     ||(char *)u0.u2 - (char *)&u0 != 0
     ||(char *)u0.u3 - (char *)&u0 != 0
     ||(char *)u0.u4 - (char *)&u0 != 0
     ||(char *)u0.u5 - (char *)&u0 != 0
     ||(char *)u0.u6 - (char *)&u0 != 0
     ||(char *)u0.u7 - (char *)&u0 != 0){

     if(pd0->flgd != 0) printf(s85er,4);
     rc = rc+4;
   }

   if( sizeof u0 < sizeof u0.u1
     ||sizeof u0 < sizeof u0.u2
     ||sizeof u0 < sizeof u0.u3
     ||sizeof u0 < sizeof u0.u4
     ||sizeof u0 < sizeof u0.u5
     ||sizeof u0 < sizeof u0.u6
     ||sizeof u0 < sizeof u0.u7){

     if(pd0->flgd != 0) printf(s85er,8);
     rc = rc+8;
   }

        /* Finally, we check that the pointers work.            */

   s1.right = &s2;
   s2.tword[0] = 2;
   s1.right->tword[0] += 1;
   if(s2.tword[0] != 3){
     if(pd0->flgd != 0) printf(s85er,16);
     rc = rc+16;
   }
   return rc;
}
s86(pd0)          /*  8.6 Initialization  */
struct defs *pd0;
{
   static char s86er[] = "s86,er%d\n";
   static char qs86[8] = "s86    ";
   int lrc, rc;
   char *ps, *pt;
   int one(), i, j, k;
   static int x[] = {1,3,5};
   static int *pint = x+2;
   static int zero[10];
   int *apint = pint-1;
   register int *rpint = apint+one();
   static float y0[] = {1,3,5,2,4,6,3,5,7,0,0,0};
   static float y1[4][3] = {
     {1,3,5},
     {2,4,6},
     {3,5,7},
   };
   static float y2[4][3] = {1,3,5,2,4,6,3,5,7};
   static float y3[4][3] = {
     {1},{2},{3},{4}
   };
   ps = qs86;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* The expression in an initializer for a static or
        external variable must be a constant expression or
        an expression that reduces to the address of a pre-
        viously declared variable, possibly offset by a
        constant expression.
                                                                */

   if(*pint != 5){
     if(pd0->flgd != 0) printf(s86er,1);
     rc = rc+1;
   }

        /* Automatic and register variables may be initialized
        by arbitrary expressions involving constants and previously
        declared variables and functions.
                                                                */

   if(*apint != 3){
     if(pd0->flgd != 0) printf(s86er,2);
     rc = rc+2;
   }

   if(*rpint != 5){
     if(pd0->flgd != 0) printf(s86er,4);
     rc = rc+4;
   }

        /* Static variables that are not initialized are guar-
        anteed to start off as zero.
                                                        */

   lrc = 0;
   for(j=0; j<10; j++)
     if(zero[j] != 0) lrc = 1;
   if(lrc != 0){
     if(pd0->flgd != 0) printf(s86er,8);
     rc = rc+8;
   }

        /* y0, y1, and y2, as declared, should define and 
        initialize identical arrays.
                                                                */
   lrc = 0;
   for(i=0; i<4; i++)
     for(j=0; j<3; j++){
       k = 3*i+j;
       if( y1[i][j] != y2[i][j]
         ||y1[i][j] != y0[k]) lrc = 1;
     }

   if(lrc != 0){
     if(pd0->flgd != 0) printf(s86er,16);
     rc = rc+16;
   }

        /* y3 initializes the first column of the array and
        leaves the rest zero.
                                                                */

   lrc = 0;
   for(j=0; j<4; j++) if(y3[j][0] != j+1) lrc = 1;

   if(lrc != 0){
     if(pd0->flgd != 0) printf(s86er,32);
     rc = rc+32;
   }
   return rc;
}
one(){
   return 1;
}
int *metricp;
s88(pd0)          /*  8.8 Typedef  */
struct defs *pd0;
{
   static char s88er[] = "s88,er%d\n";
   static char qs88[8] = "s88    ";
   int rc;
   char *ps, *pt;

        /* Declarations whose "storage class" is typdef do not
        define storage, but instead define identifiers which
        can later be used as if they were type keywords naming
        fundamental or derived types.
                                                                */

   typedef int MILES, *KLICKSP;
   typedef struct {double re, im;} complex;

   MILES distance;
   extern KLICKSP metricp;
   complex z, *zp;

   ps = qs88;
   pt = pd0->rfs;
   rc = 0;
   while(*pt++ = *ps++);

        /* Hopefully, all of this stuff will compile. After that,
        we can only make some superficial tests.

        The type of distance is int,
                                                                */

   if(sizeof distance != sizeof(int)){
     if(pd0->flgd != 0) printf(s88er,1);
     rc = rc+1;
   }

        /* that of metricp is "pointer to int",                 */

   metricp = &distance;
   distance = 2;
   *metricp = 3;

   if(distance != 3){
     if(pd0->flgd != 0) printf(s88er,2);
     rc = rc+2;
   }

        /* and that of z is the specified structure. zp is a
        pointer to such a structure.
                                                                */

   z.re = 0.;
   z.im = 0.;
   zp = &z;
   zp->re = 1.;
   zp->im = 1.;
   if(z.re+z.im != 2.){
     if(pd0->flgd != 0) printf(s88er,4);
     rc = rc+4;
   }

   return rc;
}
s9(pd0)          /*  9  Statements  */
struct defs *pd0;
{
   static char s9er[] = "s9,er%d\n";
   static char qs9[8] = "s9     ";
   int rc;
   char *ps, *pt;
   int lrc, i;

   ps = qs9;
   pt = pd0->rfs;
   rc = 0;
   while (*pt++ = *ps++);

        /* One would think that the section on statements would
        provide the most variety in the entire sequence of tests.
        As it turns out, most of the material in this section has 
        already been checked in the process of checking out
        everything else, and the section at this point is somewhat
        anticlimactic. For this reason, we restrict ourselves
        to testing two features not already covered.

        Compound statements are delimited by braces. They have the
        nice property that identifiers of the auto and register
        variety are pushed and popped. It is currently legal to
        transfer into a block, but we wont...
                                                                */

   lrc = 0;
   for(i=0; i<2; i++){
     int j;
     register int k;
     j = k = 2;
       {
       int j;
       register int k;
       j = k = 3;
       if((j != 3) || (k != 3)) lrc = 1;
       }
     if((j != 2) || (k != 2)) lrc = 1;
   }

   if(lrc != 0){
     if(pd0->flgd != 0) printf(s9er,1);
     rc = rc+1;
   }

        /* Goto statements go to labeled statements, we hope.   */

   goto nobarf;
     if(pd0->flgd != 0) printf(s9er,2);
     rc = rc+2;
   nobarf:;

   return rc;
}
setev(){                  /* Sets an external variable. Used  */
   extern int extvar;     /* by s4, and should be compiled    */
   extvar = 1066;         /* separately from s4.              */
}
     int lbits;          /*                 long           */
     int ubits;          /*                 unsigned       */
     int fbits;          /*                 float          */
     int dbits;          /*                 double         */
     float fprec;        /* Smallest number that can be    */
     float dprec;        /* significantly added to 1.      */
     int flgs;           /* Print return codes, by section */
     int flgm;           /* Announce machine dependencies  */
     int flgd;           /* give explicit diagnostics      */
     int flgl;           /* Report local return codes.     */
     int rrc;            /* recent return code             */
     int crc;            /* Cumulative return code         */
     char rfs[8];        /* Return from section            */
