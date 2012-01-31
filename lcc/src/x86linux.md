%{
/* x86/linux lburg spec. Derived from x86.md by
Marcos Ramirez <marcos@inf.utfsm.cl>
Horst von Brand <vonbrand@sleipnir.valparaiso.cl>
Jacob Navia <jacob@jacob.remcomp.fr>
*/
enum { EAX=0, ECX=1, EDX=2, EBX=3, ESI=6, EDI=7 };
#include "c.h"
#define NODEPTR_TYPE Node
#define OP_LABEL(p) ((p)->op)
#define LEFT_CHILD(p) ((p)->kids[0])
#define RIGHT_CHILD(p) ((p)->kids[1])
#define STATE_LABEL(p) ((p)->x.state)
extern int ckstack(Node, int);
extern int memop(Node);
extern int sametree(Node, Node);
static Symbol charreg[32], shortreg[32], intreg[32];
static Symbol fltreg[32];

static Symbol charregw, shortregw, intregw, fltregw;

static int cseg;

static Symbol quo, rem;

extern char *stabprefix;
extern void stabblock(int, int, Symbol*);
extern void stabend(Coordinate *, Symbol, Coordinate **, Symbol *, Symbol *);
extern void stabfend(Symbol, int);
extern void stabinit(char *, int, char *[]);
extern void stabline(Coordinate *);
extern void stabsym(Symbol);
extern void stabtype(Symbol);

static int pflag = 0;
static char rcsid[] = "$Id: x86linux.md 145 2001-10-17 21:53:10Z timo $";

#define hasargs(p) (p->syms[0] && p->syms[0]->u.c.v.i > 0 ? 0 : LBURG_MAX)
%}
%start stmt
%term CNSTF4=4113
%term CNSTF8=8209
%term CNSTF16=16401
%term CNSTI1=1045
%term CNSTI2=2069
%term CNSTI4=4117
%term CNSTI8=8213
%term CNSTP4=4119
%term CNSTP8=8215
%term CNSTU1=1046
%term CNSTU2=2070
%term CNSTU4=4118
%term CNSTU8=8214
 
%term ARGB=41
%term ARGF4=4129
%term ARGF8=8225
%term ARGF16=16417
%term ARGI4=4133
%term ARGI8=8229
%term ARGP4=4135
%term ARGP8=8231
%term ARGU4=4134
%term ARGU8=8230

%term ASGNB=57
%term ASGNF4=4145
%term ASGNF8=8241
%term ASGNF16=16433
%term ASGNI1=1077
%term ASGNI2=2101
%term ASGNI4=4149
%term ASGNI8=8245
%term ASGNP4=4151
%term ASGNP8=8247
%term ASGNU1=1078
%term ASGNU2=2102
%term ASGNU4=4150
%term ASGNU8=8246

%term INDIRB=73
%term INDIRF4=4161
%term INDIRF8=8257
%term INDIRF16=16449
%term INDIRI1=1093
%term INDIRI2=2117
%term INDIRI4=4165
%term INDIRI8=8261
%term INDIRP4=4167
%term INDIRP8=8263
%term INDIRU1=1094
%term INDIRU2=2118
%term INDIRU4=4166
%term INDIRU8=8262

%term CVFF4=4209
%term CVFF8=8305
%term CVFF16=16497
%term CVFI4=4213
%term CVFI8=8309

%term CVIF4=4225
%term CVIF8=8321
%term CVIF16=16513
%term CVII1=1157
%term CVII2=2181
%term CVII4=4229
%term CVII8=8325
%term CVIU1=1158
%term CVIU2=2182
%term CVIU4=4230
%term CVIU8=8326

%term CVPP4=4247
%term CVPP8=8343
%term CVPP16=16535
%term CVPU4=4246
%term CVPU8=8342

%term CVUI1=1205
%term CVUI2=2229
%term CVUI4=4277
%term CVUI8=8373
%term CVUP4=4279
%term CVUP8=8375
%term CVUP16=16567
%term CVUU1=1206
%term CVUU2=2230
%term CVUU4=4278
%term CVUU8=8374

%term NEGF4=4289
%term NEGF8=8385
%term NEGF16=16577
%term NEGI4=4293
%term NEGI8=8389

%term CALLB=217
%term CALLF4=4305
%term CALLF8=8401
%term CALLF16=16593
%term CALLI4=4309
%term CALLI8=8405
%term CALLP4=4311
%term CALLP8=8407
%term CALLU4=4310
%term CALLU8=8406
%term CALLV=216

%term RETF4=4337
%term RETF8=8433
%term RETF16=16625
%term RETI4=4341
%term RETI8=8437
%term RETP4=4343
%term RETP8=8439
%term RETU4=4342
%term RETU8=8438
%term RETV=248

%term ADDRGP4=4359
%term ADDRGP8=8455

%term ADDRFP4=4375
%term ADDRFP8=8471

%term ADDRLP4=4391
%term ADDRLP8=8487

%term ADDF4=4401
%term ADDF8=8497
%term ADDF16=16689
%term ADDI4=4405
%term ADDI8=8501
%term ADDP4=4407
%term ADDP8=8503
%term ADDU4=4406
%term ADDU8=8502

%term SUBF4=4417
%term SUBF8=8513
%term SUBF16=16705
%term SUBI4=4421
%term SUBI8=8517
%term SUBP4=4423
%term SUBP8=8519
%term SUBU4=4422
%term SUBU8=8518

%term LSHI4=4437
%term LSHI8=8533
%term LSHU4=4438
%term LSHU8=8534

%term MODI4=4453
%term MODI8=8549
%term MODU4=4454
%term MODU8=8550

%term RSHI4=4469
%term RSHI8=8565
%term RSHU4=4470
%term RSHU8=8566

%term BANDI4=4485
%term BANDI8=8581
%term BANDU4=4486
%term BANDU8=8582

%term BCOMI4=4501
%term BCOMI8=8597
%term BCOMU4=4502
%term BCOMU8=8598

%term BORI4=4517
%term BORI8=8613
%term BORU4=4518
%term BORU8=8614

%term BXORI4=4533
%term BXORI8=8629
%term BXORU4=4534
%term BXORU8=8630

%term DIVF4=4545
%term DIVF8=8641
%term DIVF16=16833
%term DIVI4=4549
%term DIVI8=8645
%term DIVU4=4550
%term DIVU8=8646

%term MULF4=4561
%term MULF8=8657
%term MULF16=16849
%term MULI4=4565
%term MULI8=8661
%term MULU4=4566
%term MULU8=8662

%term EQF4=4577
%term EQF8=8673
%term EQF16=16865
%term EQI4=4581
%term EQI8=8677
%term EQU4=4582
%term EQU8=8678

%term GEF4=4593
%term GEF8=8689
%term GEI4=4597
%term GEI8=8693
%term GEI16=16885
%term GEU4=4598
%term GEU8=8694

%term GTF4=4609
%term GTF8=8705
%term GTF16=16897
%term GTI4=4613
%term GTI8=8709
%term GTU4=4614
%term GTU8=8710

%term LEF4=4625
%term LEF8=8721
%term LEF16=16913
%term LEI4=4629
%term LEI8=8725
%term LEU4=4630
%term LEU8=8726

%term LTF4=4641
%term LTF8=8737
%term LTF16=16929
%term LTI4=4645
%term LTI8=8741
%term LTU4=4646
%term LTU8=8742

%term NEF4=4657
%term NEF8=8753
%term NEF16=16945
%term NEI4=4661
%term NEI8=8757
%term NEU4=4662
%term NEU8=8758

%term JUMPV=584

%term LABELV=600

%term LOADB=233
%term LOADF4=4321
%term LOADF8=8417
%term LOADF16=16609
%term LOADI1=1253
%term LOADI2=2277
%term LOADI4=4325
%term LOADI8=8421
%term LOADP4=4327
%term LOADP8=8423
%term LOADU1=1254
%term LOADU2=2278
%term LOADU4=4326
%term LOADU8=8422

%term VREGP=711
%%
reg:  INDIRI1(VREGP)     "# read register\n"
reg:  INDIRU1(VREGP)     "# read register\n"

reg:  INDIRI2(VREGP)     "# read register\n"
reg:  INDIRU2(VREGP)     "# read register\n"

reg:  INDIRI4(VREGP)     "# read register\n"
reg:  INDIRP4(VREGP)     "# read register\n"
reg:  INDIRU4(VREGP)     "# read register\n"

reg:  INDIRI8(VREGP)     "# read register\n"
reg:  INDIRP8(VREGP)     "# read register\n"
reg:  INDIRU8(VREGP)     "# read register\n"

freg:  INDIRF4(VREGP)     "# read register\n"
freg:  INDIRF8(VREGP)     "# read register\n"

stmt: ASGNI1(VREGP,reg)  "# write register\n"
stmt: ASGNU1(VREGP,reg)  "# write register\n"

stmt: ASGNI2(VREGP,reg)  "# write register\n"
stmt: ASGNU2(VREGP,reg)  "# write register\n"

stmt: ASGNF4(VREGP,reg)  "# write register\n"
stmt: ASGNI4(VREGP,reg)  "# write register\n"
stmt: ASGNP4(VREGP,reg)  "# write register\n"
stmt: ASGNU4(VREGP,reg)  "# write register\n"

stmt: ASGNF8(VREGP,reg)  "# write register\n"
stmt: ASGNI8(VREGP,reg)  "# write register\n"
stmt: ASGNP8(VREGP,reg)  "# write register\n"
stmt: ASGNU8(VREGP,reg)  "# write register\n"

cnst: CNSTI1    "%a"
cnst: CNSTU1    "%a"

cnst: CNSTI2    "%a"
cnst: CNSTU2    "%a"

cnst: CNSTI4    "%a"
cnst: CNSTU4    "%a"
cnst: CNSTP4    "%a"

cnst: CNSTI8    "%a"
cnst: CNSTU8    "%a"
cnst: CNSTP8    "%a"

con: cnst       "$%0"

stmt: reg       ""
stmt: freg      ""

acon: ADDRGP4  "%a"
acon: ADDRGP8  "%a"
acon: cnst       "%0"

baseaddr: ADDRGP4       "%a"
base: reg               "(%0)"
base: ADDI4(reg,acon)   "%1(%0)"
base: ADDP4(reg,acon)   "%1(%0)"
base: ADDU4(reg,acon)   "%1(%0)"
base: ADDRFP4           "%a(%%ebp)"
base: ADDRLP4           "%a(%%ebp)"

index: reg              "%0"
index: LSHI4(reg,con1)  "%0,2"
index: LSHI4(reg,con2)  "%0,4"
index: LSHI4(reg,con3)  "%0,8"
index: LSHU4(reg,con1)  "%0,2"
index: LSHU4(reg,con2)  "%0,4"
index: LSHU4(reg,con3)  "%0,8"

con0:  CNSTI4  "1"  range(a, 0, 0)
con0:  CNSTU4  "1"  range(a, 0, 0)
con1:  CNSTI4  "1"  range(a, 1, 1)
con1:  CNSTU4  "1"  range(a, 1, 1)
con2:  CNSTI4  "2"  range(a, 2, 2)
con2:  CNSTU4  "2"  range(a, 2, 2)
con3:  CNSTI4  "3"  range(a, 3, 3)
con3:  CNSTU4  "3"  range(a, 3, 3)

addr: base                      "%0"
addr: baseaddr                  "%0"
addr: ADDI4(index,baseaddr)     "%1(,%0)"
addr: ADDP4(index,baseaddr)     "%1(,%0)"
addr: ADDU4(index,baseaddr)     "%1(,%0)"

addr: ADDI4(reg,baseaddr)       "%1(%0)"
addr: ADDP4(reg,baseaddr)       "%1(%0)"
addr: ADDU4(reg,baseaddr)       "%1(%0)"

addr: ADDI4(index,reg)          "(%1,%0)"
addr: ADDP4(index,reg)          "(%1,%0)"
addr: ADDU4(index,reg)          "(%1,%0)"

addr: index  "(,%0)"

mem1: INDIRI1(addr)  "%0"
mem1: INDIRU1(addr)  "%0"
mem2: INDIRI2(addr)  "%0"
mem2: INDIRU2(addr)  "%0"
mem4: INDIRI4(addr)  "%0"
mem4: INDIRU4(addr)  "%0"
mem4: INDIRP4(addr)  "%0"

rc: reg         "%0"
rc: con         "%0"

mr: reg         "%0"
mr: mem4        "%0"

mr1: reg        "%0"
mr1: mem1       "%0"

mr2: reg        "%0"
mr2: mem2       "%0"

mrc: mem4       "%0"  1
mrc: mem1       "%0"  1
mrc: mem2       "%0"  1
mrc: rc         "%0"

reg: addr       "leal %0,%c\n"  1
reg: mr         "movl %0,%c\n"  1
reg: mr1        "movb %0,%c\n"  1
reg: mr2        "movw %0,%c\n"  1
reg: con        "movl %0,%c\n"  1

reg: LOADI1(reg)        "# move\n"  1
reg: LOADI2(reg)        "# move\n"  1
reg: LOADI4(reg)        "# move\n"  move(a)
reg: LOADU1(reg)        "# move\n"  1
reg: LOADU2(reg)        "# move\n"  1
reg: LOADU4(reg)        "# move\n"  move(a)
reg: LOADP4(reg)        "# move\n"  move(a)
reg: ADDI4(reg,mrc)     "?movl %0,%c\naddl %1,%c\n"  1
reg: ADDP4(reg,mrc)     "?movl %0,%c\naddl %1,%c\n"  1
reg: ADDU4(reg,mrc)     "?movl %0,%c\naddl %1,%c\n"  1
reg: SUBI4(reg,mrc)     "?movl %0,%c\nsubl %1,%c\n"  1
reg: SUBP4(reg,mrc)     "?movl %0,%c\nsubl %1,%c\n"  1
reg: SUBU4(reg,mrc)     "?movl %0,%c\nsubl %1,%c\n"  1
reg: BANDI4(reg,mrc)    "?movl %0,%c\nandl %1,%c\n"  1
reg: BORI4(reg,mrc)     "?movl %0,%c\norl %1,%c\n"   1
reg: BXORI4(reg,mrc)    "?movl %0,%c\nxorl %1,%c\n"  1
reg: BANDU4(reg,mrc)    "?movl %0,%c\nandl %1,%c\n"  1
reg: BORU4(reg,mrc)     "?movl %0,%c\norl %1,%c\n"   1
reg: BXORU4(reg,mrc)    "?movl %0,%c\nxorl %1,%c\n"  1

stmt: ASGNI4(addr,ADDI4(mem4,con1))     "incl %1\n"  memop(a)
stmt: ASGNI4(addr,ADDU4(mem4,con1))     "incl %1\n"  memop(a)
stmt: ASGNP4(addr,ADDP4(mem4,con1))     "incl %1\n"  memop(a)
stmt: ASGNI4(addr,SUBI4(mem4,con1))     "decl %1\n"  memop(a)
stmt: ASGNI4(addr,SUBU4(mem4,con1))     "decl %1\n"  memop(a)
stmt: ASGNP4(addr,SUBP4(mem4,con1))     "decl %1\n"  memop(a)
stmt: ASGNI4(addr,ADDI4(mem4,rc))       "addl %2,%1\n"  memop(a)
stmt: ASGNI4(addr,SUBI4(mem4,rc))       "sub %2,%1\n"  memop(a)
stmt: ASGNU4(addr,ADDU4(mem4,rc))       "add %2,%1\n"  memop(a)
stmt: ASGNU4(addr,SUBU4(mem4,rc))       "sub %2,%1\n"  memop(a)

stmt: ASGNI4(addr,BANDI4(mem4,rc))      "andl %2,%1\n"  memop(a)
stmt: ASGNI4(addr,BORI4(mem4,rc))       "orl %2,%1\n"   memop(a)
stmt: ASGNI4(addr,BXORI4(mem4,rc))      "xorl %2,%1\n"  memop(a)
stmt: ASGNU4(addr,BANDU4(mem4,rc))      "andl %2,%1\n"  memop(a)
stmt: ASGNU4(addr,BORU4(mem4,rc))       "orl %2,%1\n"   memop(a)
stmt: ASGNU4(addr,BXORU4(mem4,rc))      "xorl %2,%1\n"  memop(a)
reg: BCOMI4(reg)        "?movl %0,%c\nnotl %c\n"  2
reg: BCOMU4(reg)        "?movl %0,%c\nnotl %c\n"  2
reg: NEGI4(reg)         "?movl %0,%c\nnegl %c\n"  2

stmt: ASGNI4(addr,BCOMI4(mem4)) "notl %1\n"  memop(a)
stmt: ASGNU4(addr,BCOMU4(mem4)) "notl %1\n"  memop(a)
stmt: ASGNI4(addr,NEGI4(mem4))  "negl %1\n"  memop(a)
reg: LSHI4(reg,rc5)     "?movl %0,%c\nsall %1,%c\n"  2
reg: LSHU4(reg,rc5)     "?movl %0,%c\nshll %1,%c\n"  2
reg: RSHI4(reg,rc5)     "?movl %0,%c\nsarl %1,%c\n"  2
reg: RSHU4(reg,rc5)     "?movl %0,%c\nshrl %1,%c\n"  2

stmt: ASGNI4(addr,LSHI4(mem4,rc5))      "sall %2,%1\n"  memop(a)
stmt: ASGNI4(addr,LSHU4(mem4,rc5))      "shll %2,%1\n"  memop(a)
stmt: ASGNI4(addr,RSHI4(mem4,rc5))      "sarl %2,%1\n"  memop(a)
stmt: ASGNI4(addr,RSHU4(mem4,rc5))      "shrl %2,%1\n"  memop(a)

rc5: CNSTI4     "$%a"  range(a, 0, 31)
rc5: reg        "%%cl"
reg: MULI4(reg,mrc)     "?movl %0,%c\nimull %1,%c\n"  14
reg: MULI4(con,mr)      "imul %0,%1,%c\n"  13
reg: MULU4(reg,mr)      "mull %1\n"  13
reg: DIVU4(reg,reg)     "xorl %%edx,%%edx\ndivl %1\n"
reg: MODU4(reg,reg)     "xorl %%edx,%%edx\ndivl %1\n"
reg: DIVI4(reg,reg)     "cdq\nidivl %1\n"
reg: MODI4(reg,reg)     "cdq\nidivl %1\n"
reg: CVPU4(reg)         "movl %0,%c\n"  move(a)
reg: CVUP4(reg)         "movl %0,%c\n"  move(a)
reg: CVII4(INDIRI1(addr))       "movsbl %0,%c\n"  3
reg: CVII4(INDIRI2(addr))       "movswl %0,%c\n"  3
reg: CVUU4(INDIRU1(addr))       "movzbl %0,%c\n"  3
reg: CVUU4(INDIRU2(addr))       "movzwl %0,%c\n"  3
reg: CVII4(reg) "# extend\n"  3
reg: CVIU4(reg) "# extend\n"  3
reg: CVUI4(reg) "# extend\n"  3
reg: CVUU4(reg) "# extend\n"  3

reg: CVII1(reg) "# truncate\n"  1
reg: CVII2(reg) "# truncate\n"  1
reg: CVUU1(reg) "# truncate\n"  1
reg: CVUU2(reg) "# truncate\n"  1

mrca: mem4      "%0"
mrca: rc        "%0"
mrca: ADDRGP4   "$%a"
mrca: ADDRGP8   "$%a"

stmt: ASGNI1(addr,rc)           "movb %1,%0\n"   1
stmt: ASGNI2(addr,rc)           "movw %1,%0\n"   1
stmt: ASGNI4(addr,rc)           "movl %1,%0\n"   1
stmt: ASGNU1(addr,rc)           "movb %1,%0\n"   1
stmt: ASGNU2(addr,rc)           "movw %1,%0\n"   1
stmt: ASGNU4(addr,rc)           "movl %1,%0\n"   1
stmt: ASGNP4(addr,rc)           "movl %1,%0\n"   1
stmt: ARGI4(mrca)               "pushl %0\n"  1
stmt: ARGU4(mrca)               "pushl %0\n"  1
stmt: ARGP4(mrca)               "pushl %0\n"  1
stmt: ASGNB(reg,INDIRB(reg))    "movl $%a,%%ecx\nrep\nmovsb\n"
stmt: ARGB(INDIRB(reg))         "subl $%a,%%esp\nmovl %%esp,%%edi\nmovl $%a,%%ecx\nrep\nmovsb\n"

memf: INDIRF8(addr)         "l %0"
memf: INDIRF4(addr)         "s %0"
memf: CVFF8(INDIRF4(addr))  "s %0"
memf: CVFF4(INDIRF8(addr))  "l %0"

freg: memf      "fld%0\n"  3

stmt: ASGNF8(addr,freg)         "fstpl %0\n"  7
stmt: ASGNF4(addr,freg)         "fstps %0\n"  7
stmt: ASGNF4(addr,CVFF4(freg))  "fstps %0\n"  7

stmt: ARGF8(freg)       "subl $8,%%esp\nfstpl (%%esp)\n"
stmt: ARGF4(freg)       "subl $4,%%esp\nfstps (%%esp)\n"
freg: NEGF8(freg)       "fchs\n"
freg: NEGF4(freg)       "fchs\n"

flt: memf       "%0"
flt: freg       "p %%st(1),%%st"

freg: ADDF4(freg,flt)   "fadd%1\n"
freg: ADDF8(freg,flt)   "fadd%1\n"

freg: DIVF4(freg,flt)   "fdiv%1\n"
freg: DIVF8(freg,flt)   "fdiv%1\n"

freg: MULF4(freg,flt)   "fmul%1\n"
freg: MULF8(freg,flt)   "fmul%1\n"

freg: SUBF4(freg,flt)   "fsub%1\n"
freg: SUBF8(freg,flt)   "fsub%1\n"

freg: CVFF8(freg)  "# CVFF8\n"
freg: CVFF4(freg)  "sub $4,%%esp\nfstps (%%esp)\nflds (%%esp)\naddl $4,%%esp\n"  12

stmt: ASGNI4(addr,CVFI4(freg))  "fistpl %0\n" 29
reg: CVFI4(freg)  "subl $4,%%esp\nfistpl 0(%%esp)\npopl %c\n" 31

freg: CVIF8(INDIRI4(addr))      "fildl %0\n"  10
freg: CVIF8(reg)  "pushl %0\nfildl (%%esp)\naddl $4,%%esp\n"  12

freg: CVIF4(INDIRI4(addr))      "fildl %0\n"  10
freg: CVIF4(reg)  "pushl %0\nfildl (%%esp)\naddl $4,%%esp\n"  12

addrj: ADDRGP4  "%a"
addrj: reg      "*%0"  2
addrj: mem4     "*%0"  2

stmt: LABELV         "%a:\n"
stmt: JUMPV(addrj)   "jmp %0\n"  3
stmt: EQI4(mem4,rc)  "cmpl %1,%0\nje %a\n"   5
stmt: GEI4(mem4,rc)  "cmpl %1,%0\njge %a\n"  5
stmt: GTI4(mem4,rc)  "cmpl %1,%0\njg %a\n"   5
stmt: LEI4(mem4,rc)  "cmpl %1,%0\njle %a\n"  5
stmt: LTI4(mem4,rc)  "cmpl %1,%0\njl %a\n"   5
stmt: NEI4(mem4,rc)  "cmpl %1,%0\njne %a\n"  5
stmt: GEU4(mem4,rc)  "cmpl %1,%0\njae %a\n"  5
stmt: GTU4(mem4,rc)  "cmpl %1,%0\nja  %a\n"  5
stmt: LEU4(mem4,rc)  "cmpl %1,%0\njbe %a\n"  5
stmt: LTU4(mem4,rc)  "cmpl %1,%0\njb  %a\n"  5
stmt: EQI4(reg,mrc)  "cmpl %1,%0\nje %a\n"   4
stmt: GEI4(reg,mrc)  "cmpl %1,%0\njge %a\n"  4
stmt: GTI4(reg,mrc)  "cmpl %1,%0\njg %a\n"   4
stmt: LEI4(reg,mrc)  "cmpl %1,%0\njle %a\n"  4
stmt: LTI4(reg,mrc)  "cmpl %1,%0\njl %a\n"   4
stmt: NEI4(reg,mrc)  "cmpl %1,%0\njne %a\n"  4

stmt: EQU4(reg,mrc)  "cmpl %1,%0\nje %a\n"   4
stmt: GEU4(reg,mrc)  "cmpl %1,%0\njae %a\n"  4
stmt: GTU4(reg,mrc)  "cmpl %1,%0\nja %a\n"   4
stmt: LEU4(reg,mrc)  "cmpl %1,%0\njbe %a\n"  4
stmt: LTU4(reg,mrc)  "cmpl %1,%0\njb %a\n"   4
stmt: NEU4(reg,mrc)  "cmpl %1,%0\njne %a\n"  4

stmt: EQI4(BANDU4(mr,con),con0) "testl %1,%0\nje %a\n" 3
stmt: NEI4(BANDU4(mr,con),con0) "testl %1,%0\njne %a\n"

stmt: EQI4(BANDU4(CVII2(INDIRI2(addr)),con),con0) "testw %1,%0\nje %a\n"
stmt: NEI4(BANDU4(CVII2(INDIRI2(addr)),con),con0) "testw %1,%0\njne %a\n"
stmt: EQI4(BANDU4(CVIU2(INDIRI2(addr)),con),con0) "testw %1,%0\nje %a\n"
stmt: NEI4(BANDU4(CVIU2(INDIRI2(addr)),con),con0) "testw %1,%0\njne %a\n"
stmt: EQI4(BANDU4(CVII1(INDIRI1(addr)),con),con0) "testb %1,%0\nje %a\n"

cmpf: INDIRF8(addr)             "l %0"
cmpf: INDIRF4(addr)             "s %0"
cmpf: CVFF8(INDIRF4(addr))      "s %0"
cmpf: freg                      "p"

stmt: EQF8(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\nje %a\n"
stmt: GEF8(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njbe %a\n"
stmt: GTF8(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njb %a\n"
stmt: LEF8(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njae %a\n"
stmt: LTF8(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\nja %a\n"
stmt: NEF8(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njne %a\n"

stmt: EQF4(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\nje %a\n"
stmt: GEF4(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njbe %a\n"
stmt: GTF4(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njb %a\n"
stmt: LEF4(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njae %a\n"
stmt: LTF4(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\nja %a\n"
stmt: NEF4(cmpf,freg)  "fcomp%0\nfstsw %%ax\nsahf\njne %a\n"

freg: DIVF8(freg,CVIF8(INDIRI4(addr)))          "fidivl %1\n"
freg: DIVF8(CVIF8(INDIRI4(addr)),freg)          "fidivrl %0\n"
freg: DIVF8(freg,CVIF8(CVII2(INDIRI2(addr))))   "fidivs %1\n"
freg: DIVF8(CVIF8(CVII2(INDIRI2(addr))),freg)   "fidivrs %0\n"
freg: MULF8(freg,CVIF8(INDIRI4(addr)))          "fimull %1\n"
freg: MULF8(freg,CVIF8(CVII2(INDIRI2(addr))))   "fimuls %1\n"
freg: SUBF8(freg,CVIF8(INDIRI4(addr)))          "fisubl %1\n"
freg: SUBF8(CVIF8(INDIRI4(addr)),freg)          "fisubrl %0\n"
freg: SUBF8(freg,CVIF8(CVII2(INDIRI2(addr))))   "fisubs %1\n"
freg: SUBF8(CVIF8(CVII2(INDIRI2(addr))),freg)   "fisubrs %0\n"
freg: ADDF8(freg,CVIF8(INDIRI4(addr)))          "fiaddl %1\n"
freg: ADDF8(freg,CVIF8(CVII2(INDIRI2(addr))))   "fiadds %1\n"
freg: ADDF8(freg,CVFF8(INDIRF4(addr)))          "fdivs %1\n"
freg: SUBF8(freg,CVFF8(INDIRF4(addr)))          "fsubs %1\n"
freg: MULF8(freg,CVFF8(INDIRF4(addr)))          "fmuls %1\n"
freg: DIVF8(freg,CVFF8(INDIRF4(addr)))          "fdivs %1\n"
freg: LOADF8(memf)                              "fld%0\n"

reg:  CALLI4(addrj)  "call %0\naddl $%a,%%esp\n"        hasargs(a)
reg:  CALLU4(addrj)  "call %0\naddl $%a,%%esp\n"        hasargs(a)
reg:  CALLP4(addrj)  "call %0\naddl $%a,%%esp\n"        hasargs(a)

reg:  CALLI4(addrj)  "call %0\n"                        1
reg:  CALLU4(addrj)  "call %0\n"                        1
reg:  CALLP4(addrj)  "call %0\n"                        1

stmt: CALLV(addrj)   "call %0\naddl $%a,%%esp\n"        hasargs(a)
stmt: CALLV(addrj)   "call %0\n"                        1

freg: CALLF4(addrj)  "call %0\naddl $%a,%%esp\n"        hasargs(a)
freg: CALLF4(addrj)  "call %0\n"                        1

stmt: CALLF4(addrj)  "call %0\naddl $%a,%%esp\nfstp %%st(0)\n"  hasargs(a)
stmt: CALLF4(addrj)  "call %0\nfstp %%st(0)\n"                  1

freg: CALLF8(addrj)  "call %0\naddl $%a,%%esp\n"        hasargs(a)
freg: CALLF8(addrj)  "call %0\n"                        1

stmt: CALLF8(addrj)  "call %0\naddl $%a,%%esp\nfstp %%st(0)\n"  hasargs(a)
stmt: CALLF8(addrj)  "call %0\nfstp %%st(0)\n"                  1

stmt: RETI4(reg)  "# ret\n"
stmt: RETU4(reg)  "# ret\n"
stmt: RETP4(reg)  "# ret\n"
stmt: RETF4(freg) "# ret\n"
stmt: RETF8(freg) "# ret\n"
%%
static void progbeg(int argc, char *argv[]) {
        int i;

        {
                union {
                        char c;
                        int i;
                } u;
                u.i = 0;
                u.c = 1;
                swap = ((int)(u.i == 1)) != IR->little_endian;
        }
        parseflags(argc, argv);
        for (i = 0; i < argc; i++)
                if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "-pg") == 0)
                        pflag = 1;      
        intreg[EAX]   = mkreg("%%eax", EAX, 1, IREG);
        intreg[EDX]   = mkreg("%%edx", EDX, 1, IREG);
        intreg[ECX]   = mkreg("%%ecx", ECX, 1, IREG);
        intreg[EBX]   = mkreg("%%ebx", EBX, 1, IREG);
        intreg[ESI]   = mkreg("%%esi", ESI, 1, IREG);
        intreg[EDI]   = mkreg("%%edi", EDI, 1, IREG);
        shortreg[EAX] = mkreg("%%ax", EAX, 1, IREG);
        shortreg[ECX] = mkreg("%%cx", ECX, 1, IREG);
        shortreg[EDX] = mkreg("%%dx", EDX, 1, IREG);
        shortreg[EBX] = mkreg("%%bx", EBX, 1, IREG);
        shortreg[ESI] = mkreg("%%si", ESI, 1, IREG);
        shortreg[EDI] = mkreg("%%di", EDI, 1, IREG);
        charreg[EAX]  = mkreg("%%al", EAX, 1, IREG);
        charreg[ECX]  = mkreg("%%cl", ECX, 1, IREG);
        charreg[EDX]  = mkreg("%%dl", EDX, 1, IREG);
        charreg[EBX]  = mkreg("%%bl", EBX, 1, IREG);
        for (i = 0; i < 8; i++)
                fltreg[i] = mkreg("%d", i, 0, FREG);
        charregw = mkwildcard(charreg);
        shortregw = mkwildcard(shortreg);
        intregw = mkwildcard(intreg);
        fltregw = mkwildcard(fltreg);

        tmask[IREG] = (1<<EDI) | (1<<ESI) | (1<<EBX)
                    | (1<<EDX) | (1<<ECX) | (1<<EAX);
        vmask[IREG] = 0;
        tmask[FREG] = 0xff;
        vmask[FREG] = 0;

        cseg = 0;
        quo = mkreg("%%eax", EAX, 1, IREG);
        quo->x.regnode->mask |= 1<<EDX;
        rem = mkreg("%%edx", EDX, 1, IREG);
        rem->x.regnode->mask |= 1<<EAX;

        stabprefix = ".LL";
}

static Symbol rmap(int opk) {
        switch (optype(opk)) {
        case B: case P:
                return intregw;
        case I: case U:
                if (opsize(opk) == 1)
                        return charregw;
                else if (opsize(opk) == 2)
                        return shortregw;
                else
                        return intregw;
        case F:
                return fltregw;
        default:
                return 0;
        }
}

static Symbol prevg;

static void globalend(void) {
        if (prevg && prevg->type->size > 0)
                print(".size %s,%d\n", prevg->x.name, prevg->type->size);
        prevg = NULL;
}

static void progend(void) {
        globalend();
        (*IR->segment)(CODE);
        print(".ident \"LCC: 4.1\"\n");
}

static void target(Node p) {
        assert(p);
        switch (specific(p->op)) {
        case RSH+I: case RSH+U: case LSH+I: case LSH+U:
                if (generic(p->kids[1]->op) != CNST
                && !(   generic(p->kids[1]->op) == INDIR
                     && specific(p->kids[1]->kids[0]->op) == VREG+P
                     && p->kids[1]->syms[RX]->u.t.cse
                     && generic(p->kids[1]->syms[RX]->u.t.cse->op) == CNST)) {
                        rtarget(p, 1, intreg[ECX]);
                        setreg(p, intreg[EAX]);
                }
                break;
        case MUL+U:
                setreg(p, quo);
                rtarget(p, 0, intreg[EAX]);
                break;
        case DIV+I: case DIV+U:
                setreg(p, quo);
                rtarget(p, 0, intreg[EAX]);
                rtarget(p, 1, intreg[ECX]);
                break;
        case MOD+I: case MOD+U:
                setreg(p, rem);
                rtarget(p, 0, intreg[EAX]);
                rtarget(p, 1, intreg[ECX]);
                break;
        case ASGN+B:
                rtarget(p, 0, intreg[EDI]);
                rtarget(p->kids[1], 0, intreg[ESI]);
                break;
        case ARG+B:
                rtarget(p->kids[0], 0, intreg[ESI]);
                break;
        case CVF+I:
                setreg(p, intreg[EAX]);
                break;
        case CALL+I: case CALL+U: case CALL+P: case CALL+V:
                setreg(p, intreg[EAX]);
                break;
        case RET+I: case RET+U: case RET+P:
                rtarget(p, 0, intreg[EAX]);
                break;
        }
}

static void clobber(Node p) {
        static int nstack = 0;

        assert(p);
        nstack = ckstack(p, nstack);
        switch (specific(p->op)) {
        case ASGN+B: case ARG+B:
                spill(1<<ECX | 1<<ESI | 1<<EDI, IREG, p);
                break;
        case EQ+F: case LE+F: case GE+F: case LT+F: case GT+F: case NE+F:
                spill(1<<EAX, IREG, p);
                break;
        case CALL+F:
                spill(1<<EDX | 1<<EAX | 1<<ECX, IREG, p);
                break;
        case CALL+I: case CALL+U: case CALL+P: case CALL+V:
                spill(1<<EDX | 1<<ECX, IREG, p);
                break;
        }
}

static void emit2(Node p) {
        int op = specific(p->op);
#define preg(f) ((f)[getregnum(p->x.kids[0])]->x.name)

        if (op == CVI+I && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 1)
                print("movsbl %s,%s\n", preg(charreg), p->syms[RX]->x.name);
        else if (op == CVI+U && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 1)
                print("movsbl %s,%s\n", preg(charreg), p->syms[RX]->x.name);
        else if (op == CVI+I && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 2)
                print("movswl %s,%s\n", preg(shortreg), p->syms[RX]->x.name);
        else if (op == CVI+U && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 2)
                print("movswl %s,%s\n", preg(shortreg), p->syms[RX]->x.name);
        else if (op == CVU+I && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 1)
                print("movzbl %s,%s\n", preg(charreg), p->syms[RX]->x.name);
        else if (op == CVU+U && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 1)
                print("movzbl %s,%s\n", preg(charreg), p->syms[RX]->x.name);
        else if (op == CVU+I && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 2)
                print("movzwl %s,%s\n", preg(shortreg), p->syms[RX]->x.name);
        else if (op == CVU+U && opsize(p->op) == 4 && opsize(p->x.kids[0]->op) == 2)
                print("movzwl %s,%s\n", preg(shortreg), p->syms[RX]->x.name);
        else if (generic(op) == CVI || generic(op) == CVU || generic(op) == LOAD) {
                char *dst = intreg[getregnum(p)]->x.name;
                char *src = preg(intreg);
                assert(opsize(p->op) <= opsize(p->x.kids[0]->op));
                if (dst != src)
                        print("movl %s,%s\n", src, dst);
        }       
}

static void function(Symbol f, Symbol caller[], Symbol callee[], int n) {
        int i;
        
        globalend();
        print(".align 16\n");
        print(".type %s,@function\n", f->x.name);
        print("%s:\n", f->x.name);
        print("pushl %%ebp\n");
        if (pflag) {
                static int plab;
                print("movl %%esp,%%ebp\n");
                (*IR->segment)(DATA);
                print(".align 4\n.LP%d:\n.long 0\n", plab);
                (*IR->segment)(CODE);
                print("movl $.LP%d,%%edx\ncall mcount\n", plab);
                plab++;
        }
        print("pushl %%ebx\n");
        print("pushl %%esi\n");
        print("pushl %%edi\n");
        print("movl %%esp,%%ebp\n");

        usedmask[0] = usedmask[1] = 0;
        freemask[0] = freemask[1] = ~0U;
        offset = 16 + 4;
        for (i = 0; callee[i]; i++) {
                Symbol p = callee[i];
                Symbol q = caller[i];
                assert(q);
                offset = roundup(offset, q->type->align);
                p->x.offset = q->x.offset = offset;
                p->x.name = q->x.name = stringf("%d", p->x.offset);
                p->sclass = q->sclass = AUTO;
                offset += roundup(q->type->size, 4);
        }
        assert(caller[i] == 0);
        offset = maxoffset = 0;
        gencode(caller, callee);
        framesize = roundup(maxoffset, 4);
        if (framesize > 0)
                print("subl $%d,%%esp\n", framesize);
        emitcode();
        print("movl %%ebp,%%esp\n");
        print("popl %%edi\n");
        print("popl %%esi\n");
        print("popl %%ebx\n");
        print("popl %%ebp\n");
        print("ret\n");
        { int l = genlabel(1);
          print(".Lf%d:\n", l);
          print(".size %s,.Lf%d-%s\n", f->x.name, l, f->x.name);
        }
}

static void defsymbol(Symbol p) {
        if (p->scope >= LOCAL && p->sclass == STATIC)
                p->x.name = stringf("%s.%d", p->name, genlabel(1));
        else if (p->generated)
                p->x.name = stringf(".LC%s", p->name);
        else if (p->scope == GLOBAL || p->sclass == EXTERN)
                p->x.name = stringf("%s", p->name);
        else
                p->x.name = p->name;
}

static void segment(int n) {
        if (n == cseg)
                return;
        cseg = n;
        if (cseg == CODE)
                print(".text\n");
        else if (cseg == BSS)
                print(".bss\n");
        else if (cseg == DATA || cseg == LIT)
                print(".data\n");
}

static void defconst(int suffix, int size, Value v) {
        if (suffix == I && size == 1)
                print(".byte %d\n",   v.u);
        else if (suffix == I && size == 2)
                print(".word %d\n",   v.i);
        else if (suffix == I && size == 4)
                print(".long %d\n",   v.i);
        else if (suffix == U && size == 1)
                print(".byte %d\n", v.u);
        else if (suffix == U && size == 2)
                print(".word %d\n", v.u);
        else if (suffix == U && size == 4)
                print(".long %d\n", v.u);
        else if (suffix == P && size == 4)
                print(".long %d\n", v.p);
        else if (suffix == F && size == 4) {
                float f = v.d;
                print(".long %d\n", *(unsigned *)&f);
        } else if (suffix == F && size == 8) {
                double d = v.d;
                unsigned *p = (unsigned *)&d;
                print(".long %d\n.long %d\n", p[swap], p[!swap]);
        }
        else assert(0);
}

static void defaddress(Symbol p) {
        print(".long %s\n", p->x.name);
}

static void defstring(int n, char *str) {
        char *s;

        for (s = str; s < str + n; s++)
                print(".byte %d\n", (*s)&0377);
}

static void export(Symbol p) {
        globalend();
        print(".globl %s\n", p->x.name);
}

static void import(Symbol p) {}

static void global(Symbol p) {
        globalend();
        print(".align %d\n", p->type->align > 4 ? 4 : p->type->align);
        if (!p->generated) {
                print(".type %s,@%s\n", p->x.name,
                        isfunc(p->type) ? "function" : "object");
                if (p->type->size > 0)
                        print(".size %s,%d\n", p->x.name, p->type->size);
                else
                        prevg = p;
        }
        if (p->u.seg == BSS) {
                if (p->sclass == STATIC)
                        print(".lcomm %s,%d\n", p->x.name, p->type->size);
                else
                        print(".comm %s,%d\n", p->x.name, p->type->size);
        } else {
                print("%s:\n", p->x.name);
        }
}

static void space(int n) {
        if (cseg != BSS)
                print(".space %d\n", n);
}

Interface x86linuxIR = {
        1, 1, 0,  /* char */
        2, 2, 0,  /* short */
        4, 4, 0,  /* int */
        4, 4, 0,  /* long */
        4, 4, 0,  /* long long */
        4, 4, 1,  /* float */
        8, 4, 1,  /* double */
        8, 4, 1,  /* long double */
        4, 4, 0,  /* T * */
        0, 4, 0,  /* struct; so that ARGB keeps stack aligned */
        1,        /* little_endian */
        0,        /* mulops_calls */
        0,        /* wants_callb */
        1,        /* wants_argb */
        0,        /* left_to_right */
        0,        /* wants_dag */
        0,        /* unsigned_char */
        0, /* address */
        blockbeg,
        blockend,
        defaddress,
        defconst,
        defstring,
        defsymbol,
        emit,
        export,
        function,
        gen,
        global,
        import,
        0, /* local */
        progbeg,
        progend,
        segment,
        space,
        stabblock, stabend, 0, stabinit, stabline, stabsym, stabtype,
        {1, rmap,
            0, 0, 0,    /* blkfetch, blkstore, blkloop */
            _label,
            _rule,
            _nts,
            _kids,
            _string,
            _templates,
            _isinstruction,
            _ntname,
            emit2,
            0, /* doarg */
            target,
            clobber,
        }
};

void x86linux_init(int argc, char *argv[]) {
        static int inited;
        extern Interface x86IR;

        if (inited)
                return;
        inited = 1;
#define xx(f) assert(!x86linuxIR.f); x86linuxIR.f = x86IR.f
        xx(address);
        xx(local);
        xx(x.blkfetch);
        xx(x.blkstore);
        xx(x.blkloop);
        xx(x.doarg);
#undef xx
}
