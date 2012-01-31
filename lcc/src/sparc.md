%{
#include "c.h"
#define NODEPTR_TYPE Node
#define OP_LABEL(p) ((p)->op)
#define LEFT_CHILD(p) ((p)->kids[0])
#define RIGHT_CHILD(p) ((p)->kids[1])
#define STATE_LABEL(p) ((p)->x.state)
static void address(Symbol, Symbol, long);
static void blkfetch(int, int, int, int);
static void blkloop(int, int, int, int, int, int[]);
static void blkstore(int, int, int, int);
static void defaddress(Symbol);
static void defconst(int, int, Value);
static void defstring(int, char *);
static void defsymbol(Symbol);
static void doarg(Node);
static void emit2(Node);
static void export(Symbol);
static void clobber(Node);
static void function(Symbol, Symbol [], Symbol [], int);
static void global(Symbol);
static void import(Symbol);
static void local(Symbol);
static void progbeg(int, char **);
static void progend(void);
static void segment(int);
static void space(int);
static void target(Node);
static int imm(Node);
static void renameregs(void);
extern Interface sparcIR, solarisIR;
static void defsymbol2(Symbol);
static void export2(Symbol);
static void globalend(void);
static void global2(Symbol);
static void segment2(int);
static void progend2(void);

extern char *stabprefix;
extern void stabblock(int, int, Symbol*);
extern void stabend(Coordinate *, Symbol, Coordinate **, Symbol *, Symbol *);
extern void stabfend(Symbol, int);
extern void stabinit(char *, int, char *[]);
extern void stabline(Coordinate *);
extern void stabsym(Symbol);
extern void stabtype(Symbol);
static Symbol greg[32], gregw;
static Symbol *oreg = &greg[8], *ireg = &greg[24];
static Symbol freg[32], freg2[32];
static Symbol fregw, freg2w;

static int regvars;
static int retstruct;

static int pflag = 0;

static int cseg;

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

reg:  INDIRF4(VREGP)     "# read register\n"
reg:  INDIRI4(VREGP)     "# read register\n"
reg:  INDIRP4(VREGP)     "# read register\n"
reg:  INDIRU4(VREGP)     "# read register\n"

reg:  INDIRF8(VREGP)     "# read register\n"
reg:  INDIRI8(VREGP)     "# read register\n"
reg:  INDIRP8(VREGP)     "# read register\n"
reg:  INDIRU8(VREGP)     "# read register\n"

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
con: CNSTI1  "%a"
con: CNSTU1  "%a"

con: CNSTI2  "%a"
con: CNSTU2  "%a"

con: CNSTI4  "%a"
con: CNSTU4  "%a"
con: CNSTP4  "%a"

con: CNSTI8  "%a"
con: CNSTU8  "%a"
con: CNSTP8  "%a"
stmt: reg  ""
reg: ADDRGP4  "set %a,%%%c\n"  1
stk13: ADDRFP4  "%a"                  imm(a)
stk13: ADDRLP4  "%a"                  imm(a)
reg:   stk13   "add %0,%%fp,%%%c\n"  1
stk: ADDRFP4  "set %a,%%%c\n"                      2
stk: ADDRLP4  "set %a,%%%c\n"                      2
reg: ADDRFP4  "set %a,%%%c\nadd %%%c,%%fp,%%%c\n"  3
reg: ADDRLP4  "set %a,%%%c\nadd %%%c,%%fp,%%%c\n"  3
con13: CNSTI1  "%a"  imm(a)
con13: CNSTI2  "%a"  imm(a)
con13: CNSTI4  "%a"  imm(a)
con13: CNSTU1  "%a"  imm(a)
con13: CNSTU2  "%a"  imm(a)
con13: CNSTU4  "%a"  imm(a)
con13: CNSTP4  "%a"  imm(a)
base: ADDI4(reg,con13)  "%%%0+%1"
base: ADDP4(reg,con13)  "%%%0+%1"
base: ADDU4(reg,con13)  "%%%0+%1"
base: reg    "%%%0"
base: con13  "%0"
base: stk13  "%%fp+%0"
addr: base           "%0"
addr: ADDI4(reg,reg)  "%%%0+%%%1"
addr: ADDP4(reg,reg)  "%%%0+%%%1"
addr: ADDU4(reg,reg)  "%%%0+%%%1"
addr: stk            "%%fp+%%%0"
reg:  INDIRI1(addr)     "ldsb [%0],%%%c\n"  1
reg:  INDIRI2(addr)     "ldsh [%0],%%%c\n"  1
reg:  INDIRI4(addr)     "ld [%0],%%%c\n"    1
reg:  INDIRU1(addr)     "ldub [%0],%%%c\n"  1
reg:  INDIRU2(addr)     "lduh [%0],%%%c\n"  1
reg:  INDIRU4(addr)     "ld [%0],%%%c\n"    1
reg:  INDIRP4(addr)     "ld [%0],%%%c\n"    1
reg:  INDIRF4(addr)     "ld [%0],%%f%c\n"   1
stmt: ASGNI1(addr,reg)  "stb %%%1,[%0]\n"   1
stmt: ASGNI2(addr,reg)  "sth %%%1,[%0]\n"   1
stmt: ASGNI4(addr,reg)  "st %%%1,[%0]\n"    1
stmt: ASGNU1(addr,reg)  "stb %%%1,[%0]\n"   1
stmt: ASGNU2(addr,reg)  "sth %%%1,[%0]\n"   1
stmt: ASGNU4(addr,reg)  "st %%%1,[%0]\n"    1
stmt: ASGNP4(addr,reg)  "st %%%1,[%0]\n"    1
stmt: ASGNF4(addr,reg)  "st %%f%1,[%0]\n"   1
addrl: ADDRLP4            "%%%fp+%a"          imm(a)

reg:   INDIRF8(addrl)     "ldd [%0],%%f%c\n"  1
stmt:  ASGNF8(addrl,reg)  "std %%f%1,[%0]\n"  1
reg:  INDIRF8(base)     "ld2 [%0],%%f%c\n"  2
stmt: ASGNF8(base,reg)  "st2 %%f%1,[%0]\n"  2
spill:  ADDRLP4          "%a" !imm(a)

stmt: ASGNI1(spill,reg)  "set %0,%%g1\nstb %%%1,[%%fp+%%g1]\n"
stmt: ASGNI2(spill,reg)  "set %0,%%g1\nsth %%%1,[%%fp+%%g1]\n"
stmt: ASGNI4(spill,reg)  "set %0,%%g1\nst %%%1,[%%fp+%%g1]\n"
stmt: ASGNU1(spill,reg)  "set %0,%%g1\nstb %%%1,[%%fp+%%g1]\n"
stmt: ASGNU2(spill,reg)  "set %0,%%g1\nsth %%%1,[%%fp+%%g1]\n"
stmt: ASGNU4(spill,reg)  "set %0,%%g1\nst %%%1,[%%fp+%%g1]\n"
stmt: ASGNP4(spill,reg)  "set %0,%%g1\nst %%%1,[%%fp+%%g1]\n"
stmt: ASGNF4(spill,reg)  "set %0,%%g1\nst %%f%1,[%%fp+%%g1]\n"
stmt: ASGNF8(spill,reg)  "set %0,%%g1\nstd %%f%1,[%%fp+%%g1]\n"
reg: CVII4(INDIRI1(addr))  "ldsb [%0],%%%c\n"  1
reg: CVII4(INDIRI2(addr))  "ldsh [%0],%%%c\n"  1
reg: CVUU4(INDIRU1(addr))  "ldub [%0],%%%c\n"  1
reg: CVUU4(INDIRU2(addr))  "lduh [%0],%%%c\n"  1
reg: CVUI4(INDIRU1(addr))  "ldub [%0],%%%c\n"  1
reg: CVUI4(INDIRU2(addr))  "lduh [%0],%%%c\n"  1
reg: LOADI1(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADI2(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADI4(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADP4(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADU1(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADU2(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADU4(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CNSTI1  "# reg\n"  range(a, 0, 0)
reg: CNSTI2  "# reg\n"  range(a, 0, 0)
reg: CNSTI4  "# reg\n"  range(a, 0, 0)
reg: CNSTP4  "# reg\n"  range(a, 0, 0)
reg: CNSTU1  "# reg\n"  range(a, 0, 0)
reg: CNSTU2  "# reg\n"  range(a, 0, 0)
reg: CNSTU4  "# reg\n"  range(a, 0, 0)
reg: con  "set %0,%%%c\n"  1
rc: con13  "%0"
rc: reg    "%%%0"
reg: ADDI4(reg,rc)   "add %%%0,%1,%%%c\n"  1
reg: ADDP4(reg,rc)   "add %%%0,%1,%%%c\n"  1
reg: ADDU4(reg,rc)   "add %%%0,%1,%%%c\n"  1
reg: BANDI4(reg,rc)  "and %%%0,%1,%%%c\n"  1
reg: BORI4(reg,rc)   "or %%%0,%1,%%%c\n"   1
reg: BXORI4(reg,rc)  "xor %%%0,%1,%%%c\n"  1
reg: BANDU4(reg,rc)  "and %%%0,%1,%%%c\n"  1
reg: BORU4(reg,rc)   "or %%%0,%1,%%%c\n"   1
reg: BXORU4(reg,rc)  "xor %%%0,%1,%%%c\n"  1
reg: SUBI4(reg,rc)   "sub %%%0,%1,%%%c\n"  1
reg: SUBP4(reg,rc)   "sub %%%0,%1,%%%c\n"  1
reg: SUBU4(reg,rc)   "sub %%%0,%1,%%%c\n"  1
rc5: CNSTI4  "%a"    range(a, 0, 31)
rc5: reg    "%%%0"
reg: LSHI4(reg,rc5)  "sll %%%0,%1,%%%c\n"  1
reg: LSHU4(reg,rc5)  "sll %%%0,%1,%%%c\n"  1
reg: RSHI4(reg,rc5)  "sra %%%0,%1,%%%c\n"  1
reg: RSHU4(reg,rc5)  "srl %%%0,%1,%%%c\n"  1
reg: BANDI4(reg,BCOMI4(rc))  "andn %%%0,%1,%%%c\n"  1
reg: BORI4(reg,BCOMI4(rc))   "orn %%%0,%1,%%%c\n"   1
reg: BXORI4(reg,BCOMI4(rc))  "xnor %%%0,%1,%%%c\n"  1
reg: BANDU4(reg,BCOMU4(rc))  "andn %%%0,%1,%%%c\n"  1
reg: BORU4(reg,BCOMU4(rc))   "orn %%%0,%1,%%%c\n"   1
reg: BXORU4(reg,BCOMU4(rc))  "xnor %%%0,%1,%%%c\n"  1
reg: NEGI4(reg)   "neg %%%0,%%%c\n"  1
reg: BCOMI4(reg)  "not %%%0,%%%c\n"  1
reg: BCOMU4(reg)  "not %%%0,%%%c\n"  1
reg: CVII4(reg)  "sll %%%0,8*(4-%a),%%%c; sra %%%c,8*(4-%a),%%%c\n"  2
reg: CVUU4(reg)  "sll %%%0,8*(4-%a),%%%c; srl %%%c,8*(4-%a),%%%c\n"  2
reg: CVUU4(reg)  "and %%%0,0xff,%%%c\n" (a->syms[0]->u.c.v.i == 1 ? 1 : LBURG_MAX)
reg: CVUU4(reg)  "set 0xffff,%%g1; and %%%0,%%g1,%%%c\n"  2
reg: CVUI4(reg)  "and %%%0,0xff,%%%c\n" (a->syms[0]->u.c.v.i == 1 ? 1 : LBURG_MAX)
reg: CVUI4(reg)  "set 0xffff,%%g1; and %%%0,%%g1,%%%c\n"  2
addrg: ADDRGP4        "%a"
stmt:  JUMPV(addrg)  "ba %0; nop\n"   2
stmt:  JUMPV(addr)   "jmp %0; nop\n"  2
stmt:  LABELV        "%a:\n"
stmt: EQI4(reg,rc)  "cmp %%%0,%1; be %a; nop\n"    3
stmt: EQU4(reg,rc)  "cmp %%%0,%1; be %a; nop\n"    3
stmt: GEI4(reg,rc)  "cmp %%%0,%1; bge %a; nop\n"   3
stmt: GEU4(reg,rc)  "cmp %%%0,%1; bgeu %a; nop\n"  3
stmt: GTI4(reg,rc)  "cmp %%%0,%1; bg %a; nop\n"    3
stmt: GTU4(reg,rc)  "cmp %%%0,%1; bgu %a; nop\n"   3
stmt: LEI4(reg,rc)  "cmp %%%0,%1; ble %a; nop\n"   3
stmt: LEU4(reg,rc)  "cmp %%%0,%1; bleu %a; nop\n"  3
stmt: LTI4(reg,rc)  "cmp %%%0,%1; bl %a; nop\n"    3
stmt: LTU4(reg,rc)  "cmp %%%0,%1; blu %a; nop\n"   3
stmt: NEI4(reg,rc)  "cmp %%%0,%1; bne %a; nop\n"   3
stmt: NEU4(reg,rc)  "cmp %%%0,%1; bne %a; nop\n"   3
call: ADDRGP4           "%a"
call: addr             "%0"
reg:  CALLF8(call)      "call %0; nop\n"                2
reg:  CALLF4(call)      "call %0; nop\n"                2
reg:  CALLI4(call)      "call %0; nop\n"                2
reg:  CALLP4(call)      "call %0; nop\n"                2
reg:  CALLU4(call)      "call %0; nop\n"                2
stmt: CALLV(call)       "call %0; nop\n"                2
stmt: CALLB(call,reg)   "call %0; st %%%1,[%%sp+64]; unimp %b&0xfff\n"  3

stmt: RETF8(reg)  "# ret\n"  1
stmt: RETF4(reg)  "# ret\n"  1
stmt: RETI4(reg)  "# ret\n"  1
stmt: RETU4(reg)  "# ret\n"  1
stmt: RETP4(reg)  "# ret\n"  1
stmt: ARGI4(reg)  "st %%%0,[%%sp+4*%c+68]\n"  1
stmt: ARGU4(reg)  "st %%%0,[%%sp+4*%c+68]\n"  1
stmt: ARGP4(reg)  "st %%%0,[%%sp+4*%c+68]\n"  1
stmt: ARGF4(reg)  "# ARGF4\n"  1
stmt: ARGF8(reg)  "# ARGF8\n"  1

reg: DIVI4(reg,rc)   "sra %%%0,31,%%g1; wr %%g0,%%g1,%%y; nop; nop; nop; sdiv %%%0,%1,%%%c\n"       6

reg: DIVU4(reg,rc)   "wr %%g0,%%g0,%%y; nop; nop; nop; udiv %%%0,%1,%%%c\n"       5

reg: MODI4(reg,rc)   "sra %%%0,31,%%g1; wr %%g0,%%g1,%%y; nop; nop; nop; sdiv %%%0,%1,%%g1\n; smul %%g1,%1,%%g1; sub %%%0,%%g1,%%%c\n"  8


reg: MODU4(reg,rc)   "wr %%g0,%%g0,%%y; nop; nop; nop; udiv %%%0,%1,%%g1\n; umul %%g1,%1,%%g1; sub %%%0,%%g1,%%%c\n"  7


reg: MULI4(rc,reg)   "smul %%%1,%0,%%%c\n"  1
reg: MULU4(rc,reg)   "umul %%%1,%0,%%%c\n"  1
reg: ADDF8(reg,reg)  "faddd %%f%0,%%f%1,%%f%c\n"  1
reg: ADDF4(reg,reg)  "fadds %%f%0,%%f%1,%%f%c\n"  1
reg: DIVF8(reg,reg)  "fdivd %%f%0,%%f%1,%%f%c\n"  1
reg: DIVF4(reg,reg)  "fdivs %%f%0,%%f%1,%%f%c\n"  1
reg: MULF8(reg,reg)  "fmuld %%f%0,%%f%1,%%f%c\n"  1
reg: MULF4(reg,reg)  "fmuls %%f%0,%%f%1,%%f%c\n"  1
reg: SUBF8(reg,reg)  "fsubd %%f%0,%%f%1,%%f%c\n"  1
reg: SUBF4(reg,reg)  "fsubs %%f%0,%%f%1,%%f%c\n"  1
reg: NEGF4(reg)   "fnegs %%f%0,%%f%c\n"  1
reg: LOADF4(reg)  "fmovs %%f%0,%%f%c\n"  1
reg: CVFF4(reg)   "fdtos %%f%0,%%f%c\n"  1
reg: CVFF8(reg)   "fstod %%f%0,%%f%c\n"  1
reg: CVFI4(reg)  "fstoi %%f%0,%%f0; st %%f0,[%%sp+64]; ld [%%sp+64],%%%c\n"  (a->syms[0]->u.c.v.i==4?3:LBURG_MAX)

reg: CVFI4(reg)  "fdtoi %%f%0,%%f0; st %%f0,[%%sp+64]; ld [%%sp+64],%%%c\n"  (a->syms[0]->u.c.v.i==8?3:LBURG_MAX)

reg: CVIF4(reg)  "st %%%0,[%%sp+64]; ld [%%sp+64],%%f%c; fitos %%f%c,%%f%c\n"  3

reg: CVIF8(reg)  "st %%%0,[%%sp+64]; ld [%%sp+64],%%f%c; fitod %%f%c,%%f%c\n"  3

rel: EQF8(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbue"
rel: EQF4(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbue"
rel: GEF8(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbuge"
rel: GEF4(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbuge"
rel: GTF8(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbug"
rel: GTF4(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbug"
rel: LEF8(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbule"
rel: LEF4(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbule"
rel: LTF8(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbul"
rel: LTF4(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbul"
rel: NEF8(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbne"
rel: NEF4(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbne"

stmt: rel  "%0 %a; nop\n"  4
reg:  LOADF8(reg)  "# LOADD\n"  2

reg:  NEGF8(reg)  "# NEGD\n"  2

stmt:  ASGNB(reg,INDIRB(reg))  "# ASGNB\n"

%%
static void progend(void){}
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
        if (IR == &solarisIR)
                stabprefix = ".LL";
        else
                stabprefix = "L";
        for (i = 0; i < 8; i++) {
                greg[i +  0] = mkreg(stringf("g%d", i), i +  0, 1, IREG);
                greg[i +  8] = mkreg(stringf("o%d", i), i +  8, 1, IREG);
                greg[i + 16] = mkreg(stringf("l%d", i), i + 16, 1, IREG);
                greg[i + 24] = mkreg(stringf("i%d", i), i + 24, 1, IREG);
        }
        gregw = mkwildcard(greg);
        for (i = 0; i < 32; i++)
                freg[i]  = mkreg("%d", i, 1, FREG);
        for (i = 0; i < 31; i += 2)
                freg2[i] = mkreg("%d", i, 3, FREG);
        fregw = mkwildcard(freg);
        freg2w = mkwildcard(freg2);
        tmask[IREG] = 0x3fff3e00;
        vmask[IREG] = 0x3ff00000;
        tmask[FREG]  = ~(unsigned)0;
        vmask[FREG]  = 0;
}
static Symbol rmap(int opk) {
        switch (optype(opk)) {
        case I: case U: case P: case B:
                return gregw;
        case F:
                return opsize(opk) == 4 ? fregw : freg2w;
        default:
                return 0;
        }
}
static void target(Node p) {
        assert(p);
        switch (specific(p->op)) {
        case CNST+I: case CNST+U: case CNST+P:
                if (range(p, 0, 0) == 0) {
                        setreg(p, greg[0]);
                        p->x.registered = 1;
                }
                break;
        case CALL+B:
                assert(p->syms[1] && p->syms[1]->type && isfunc(p->syms[1]->type));
                p->syms[1] = intconst(freturn(p->syms[1]->type)->size);
                break;
        case CALL+F: setreg(p, opsize(p->op)==4?freg[0]:freg2[0]);     break;
        case CALL+I: case CALL+P: case CALL+U:
        case CALL+V: setreg(p, oreg[0]);      break;
        case RET+F:  rtarget(p, 0, opsize(p->op)==4?freg[0]:freg2[0]);  break;
        case RET+I: case RET+P: case RET+U:
                rtarget(p, 0, ireg[0]);
                p->kids[0]->x.registered = 1;
                break;
        case ARG+I: case ARG+P: case ARG+U:
                if (p->syms[RX]->u.c.v.i < 6) {
                        rtarget(p, 0, oreg[p->syms[RX]->u.c.v.i]);
                        p->op = LOAD+opkind(p->op);
                        setreg(p, oreg[p->syms[RX]->u.c.v.i]);
                }
                break;
        }
}
static void clobber(Node p) {
        assert(p);
        switch (specific(p->op)) {
        case CALL+B: case CALL+F: case CALL+I:
                spill(~(unsigned)3, FREG, p);
                break;
        case CALL+V:
                spill(oreg[0]->x.regnode->mask, IREG, p);
                spill(~(unsigned)3, FREG, p);
                break;
        case ARG+F:
                if (opsize(p->op) == 4 && p->syms[2]->u.c.v.i <= 6)
                        spill((1<<(p->syms[2]->u.c.v.i + 8)), IREG, p);
                else if (opsize(p->op) == 8 && p->syms[2]->u.c.v.i <= 5)
                        spill((3<<(p->syms[2]->u.c.v.i + 8))&0xff00, IREG, p);
                break;
        }
}
static int imm(Node p) {
        return range(p, -4096, 4091);
}
static void doarg(Node p) {
        assert(p && p->syms[0] && p->op != ARG+B);
        p->syms[RX] = intconst(mkactual(4,
                p->syms[0]->u.c.v.i)/4);
}
static void emit2(Node p) {
        switch (p->op) {
        case ARG+F+sizeop(4): {
                int n = p->syms[RX]->u.c.v.i;
                print("st %%f%d,[%%sp+4*%d+68]\n",
                        getregnum(p->x.kids[0]), n);
                if (n <= 5)
                        print("ld [%%sp+4*%d+68],%%o%d\n", n, n);
                break;
        }
        case ARG+F+sizeop(8): {
                int n = p->syms[RX]->u.c.v.i;
                int src = getregnum(p->x.kids[0]);
                print("st %%f%d,[%%sp+4*%d+68]\n", src, n);
                print("st %%f%d,[%%sp+4*%d+68]\n", src+1, n+1);
                if (n <= 5)
                        print("ld [%%sp+4*%d+68],%%o%d\n", n, n);
                if (n <= 4)
                        print("ld [%%sp+4*%d+68],%%o%d\n", n+1, n+1);
                break;
        }
        case LOAD+F+sizeop(8): {
                int dst = getregnum(p);
                int src = getregnum(p->x.kids[0]);
                print("fmovs %%f%d,%%f%d; ", src,   dst);
                print("fmovs %%f%d,%%f%d\n", src+1, dst+1);
                break;
        }
        case NEG+F+sizeop(8): {
                int dst = getregnum(p);
                int src = getregnum(p->x.kids[0]);
                print("fnegs %%f%d,%%f%d; ", src,   dst);
                print("fmovs %%f%d,%%f%d\n", src+1, dst+1);
                break;
        }
        case ASGN+B: {
                static int tmpregs[] = { 1, 2, 3 };
                dalign = salign = p->syms[1]->u.c.v.i;
                blkcopy(getregnum(p->x.kids[0]), 0,
                        getregnum(p->x.kids[1]), 0,
                        p->syms[0]->u.c.v.i, tmpregs);
                break;
        }
        }
}
static void local(Symbol p) {
        if (retstruct) {
                assert(p == retv);
                p->x.name = stringd(4*16);
                p->x.offset = 4*16;
                p->sclass = AUTO;
                retstruct = 0;
                return;
        }
        if (isscalar(p->type) && !p->addressed && !isfloat(p->type))
                p->sclass = REGISTER;
        if (askregvar(p, rmap(ttob(p->type))) == 0)
                mkauto(p);
        else if (p->scope > LOCAL)
                regvars++;
}
static void function(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
        int autos = 0, i, leaf, reg, varargs;

        if (IR == &solarisIR)
                globalend();
        regvars = 0;
        for (i = 0; callee[i]; i++)
                ;
        varargs = variadic(f->type)
                || i > 0 && strcmp(callee[i-1]->name,
                        "__builtin_va_alist") == 0;
        usedmask[0] = usedmask[1] = 0;
        freemask[0] = freemask[1] = ~(unsigned)0;
        for (i = 0; i < 8; i++)
                ireg[i]->x.regnode->vbl = NULL;
        offset = 68;
        maxargoffset = 24;
        reg = 0;
        for (i = 0; callee[i]; i++) {
                Symbol p = callee[i], q = caller[i];
                int size = roundup(q->type->size, 4);
                assert(q);
                if (isfloat(p->type) || reg >= 6) {
                        p->x.offset = q->x.offset = offset;
                        p->x.name = q->x.name = stringd(offset);
                        p->sclass = q->sclass = AUTO;
                        autos++;
                }
                else if (p->addressed || varargs) {
                        p->x.offset = offset;
                        p->x.name = stringd(p->x.offset);
                        p->sclass = AUTO;
                        q->sclass = REGISTER;
                        askregvar(q, ireg[reg]);
                        assert(q->x.regnode);
                        autos++;
                }
                else {
                        p->sclass = q->sclass = REGISTER;
                        askregvar(p, ireg[reg]);
                        assert(p->x.regnode);
                        q->x.name = p->x.name;
                }
                offset += size;
                reg += isstruct(p->type) ? 1 : size/4;
        }
        assert(caller[i] == 0);
        offset = maxoffset = 0;
        retstruct = isstruct(freturn(f->type));
        gencode(caller, callee);
        maxargoffset = roundup(maxargoffset, 4);
        framesize = roundup(maxoffset + maxargoffset + 4*(16+1), 8);
        assert(!varargs || autos);
        leaf = (!ncalls
                && !maxoffset && !autos && !regvars
                && !isstruct(freturn(f->type))
                && !(usedmask[IREG]&0x00ffff01)
                && !(usedmask[FREG]&~(unsigned)3)
                && !pflag && !glevel);
        print(".align 4\n%s:\n", f->x.name);
        if (leaf) {
                for (i = 0; caller[i] && callee[i]; i++) {
                        Symbol p = caller[i], q = callee[i];
                        if (p->sclass == REGISTER && q->sclass == REGISTER) {
                                assert(q->x.regnode);
                                assert(q->x.regnode->set == IREG);
                                assert(q->x.regnode->number >= 24);
                                assert(q->x.regnode->number <= 31);
                                p->x.name = greg[q->x.regnode->number - 16]->x.name;
                        }
                }
                renameregs();
        } else if (framesize <= 4095)
                print("save %%sp,%d,%%sp\n", -framesize);
        else
                print("set %d,%%g1; save %%sp,%%g1,%%sp\n", -framesize);
        if (varargs)
                for (; reg < 6; reg++)
                        print("st %%i%d,[%%fp+%d]\n", reg, 4*reg + 68);
        else {
                offset = 4*(16 + 1);
                reg = 0;
                for (i = 0; caller[i]; i++) {
                        Symbol p = caller[i];
                        if (isfloat(p->type) && p->type->size == 8 && reg <= 4) {
                                print("st %%r%d,[%%fp+%d]\n",
                                        ireg[reg++]->x.regnode->number, offset);
                                print("st %%r%d,[%%fp+%d]\n",
                                        ireg[reg++]->x.regnode->number, offset + 4);
                        } else if (isfloat(p->type) && p->type->size == 4 && reg <= 5)
                                print("st %%r%d,[%%fp+%d]\n",
                                        ireg[reg++]->x.regnode->number, offset);
                        else
                                reg++;
                        offset += roundup(p->type->size, 4);
                }
        }
        if (pflag) {
                int lab = genlabel(1);
                print("set L%d,%%o0; call mcount; nop\n", lab);
                print(".seg \"data\"\n.align 4; L%d:.word 0\n.seg \"text\"\n", lab);
        }
        emitcode();
        if (isstruct(freturn(f->type)))
                print("jmp %%i7+12; restore\n");
        else if (!leaf)
                print("ret; restore\n");
        else {
                renameregs();
                print("retl; nop\n");
        }
        if (IR == &solarisIR) {
                print(".type %s,#function\n", f->x.name);
                print(".size %s,.-%s\n", f->x.name, f->x.name);
        }
}
#define exch(x, y, t) (((t) = x), ((x) = (y)), ((y) = (t)))

static void renameregs(void) {
        int i;

        for (i = 0; i < 8; i++) {
                char *ptmp;
                int itmp;
                if (ireg[i]->x.regnode->vbl)
                        ireg[i]->x.regnode->vbl->x.name = oreg[i]->x.name;
                exch(ireg[i]->x.name, oreg[i]->x.name, ptmp);
                exch(ireg[i]->x.regnode->number,
                        oreg[i]->x.regnode->number, itmp);
        }
}
static void defconst(int suffix, int size, Value v) {
        if (suffix == F && size == 4) {
                float f = v.d;
                print(".word 0x%x\n", *(unsigned *)&f);
        } else if (suffix == F && size == 8) {
                double d = v.d;
                unsigned *p = (unsigned *)&d;
                print(".word 0x%x\n.word 0x%x\n", p[swap], p[!swap]);
        } else if (suffix == P)
                print(".word 0x%x\n", v.p);
        else if (size == 1)
                print(".byte 0x%x\n", suffix == I ? v.i : v.u);
        else if (size == 2)
                print(".half 0x%x\n", suffix == I ? v.i : v.u);
        else if (size == 4)
                print(".word 0x%x\n", suffix == I ? v.i : v.u);
        else assert(0);
}

static void defaddress(Symbol p) {
        print(".word %s\n", p->x.name);
}

static void defstring(int n, char *str) {
        char *s;

        for (s = str; s < str + n; s++)
                print(".byte %d\n", (*s)&0377);
}

static void address(Symbol q, Symbol p, long n) {
        if (p->scope == GLOBAL || p->sclass == STATIC || p->sclass == EXTERN)
                q->x.name = stringf("%s%s%D", p->x.name, n >= 0 ? "+" : "", n);
        else {
                assert(n <= INT_MAX && n >= INT_MIN);
                q->x.offset = p->x.offset + n;
                q->x.name = stringd(q->x.offset);
        }
}
static void export(Symbol p) {
        print(".global %s\n", p->x.name);
}
static void import(Symbol p) {}
static void defsymbol(Symbol p) {
        if (p->scope >= LOCAL && p->sclass == STATIC)
                p->x.name = stringf("%d", genlabel(1));
        else
                assert(p->scope != CONSTANTS || isint(p->type) || isptr(p->type)),
                p->x.name = p->name;
        if (p->scope >= LABELS)
                p->x.name = stringf(p->generated ? "L%s" : "_%s",
                        p->x.name);
}
static void segment(int n) {
        cseg = n;
        switch (n) {
        case CODE: print(".seg \"text\"\n"); break;
        case BSS:  print(".seg \"bss\"\n");  break;
        case DATA: print(".seg \"data\"\n"); break;
        case LIT:  print(".seg \"text\"\n"); break;
        }
}
static void space(int n) {
        if (cseg != BSS)
                print(".skip %d\n", n);
}
static void global(Symbol p) {
        print(".align %d\n", p->type->align);
        assert(p->u.seg);
        if (p->u.seg == BSS
        && (p->sclass == STATIC || Aflag >= 2))
                print(".reserve %s,%d\n", p->x.name, p->type->size);
        else if (p->u.seg == BSS)
                print(".common %s,%d\n",  p->x.name, p->type->size);
        else
                print("%s:\n", p->x.name);
}
static void blkfetch(int k, int off, int reg, int tmp) {
        assert(k == 1 || k == 2 || k == 4);
        assert(salign >= k);
        if (k == 1)
                print("ldub [%%r%d+%d],%%r%d\n", reg, off, tmp);
        else if (k == 2)
                print("lduh [%%r%d+%d],%%r%d\n", reg, off, tmp);
        else
                print("ld [%%r%d+%d],%%r%d\n",   reg, off, tmp);
}
static void blkstore(int k, int off, int reg, int tmp) {
        assert(k == 1 || k == 2 || k == 4);
        assert(dalign >= k);
        if (k == 1)
                print("stb %%r%d,[%%r%d+%d]\n", tmp, reg, off);
        else if (k == 2)
                print("sth %%r%d,[%%r%d+%d]\n", tmp, reg, off);
        else
                print("st %%r%d,[%%r%d+%d]\n",  tmp, reg, off);
}
static void blkloop(int dreg, int doff, int sreg, int soff, int size, int tmps[]) {
        if ((size&~7) < 4096) {
                print("add %%r%d,%d,%%r%d\n", sreg, size&~7, sreg);
                print("add %%r%d,%d,%%r%d\n", dreg, size&~7, tmps[2]);
        } else {
                print("set %d,%%r%d\n", size&~7, tmps[2]);
                print("add %%r%d,%%r%d,%%r%d\n", sreg, tmps[2], sreg);
                print("add %%r%d,%%r%d,%%r%d\n", dreg, tmps[2], tmps[2]);
        }
        blkcopy(tmps[2], doff, sreg, soff, size&7, tmps);
        print("1: dec 8,%%r%d\n", tmps[2]);
        blkcopy(tmps[2], doff, sreg, soff - 8, 8, tmps);
        print("cmp %%r%d,%%r%d; ", tmps[2], dreg);
        print("bgt 1b; ");
        print("dec 8,%%r%d\n", sreg);
}
static void defsymbol2(Symbol p) {
        if (p->scope >= LOCAL && p->sclass == STATIC)
                p->x.name = stringf(".%d", genlabel(1));
        else
                assert(p->scope != CONSTANTS || isint(p->type) || isptr(p->type)),
                p->x.name = p->name;
        if (p->scope >= LABELS)
                p->x.name = stringf(p->generated ? ".L%s" : "%s",
                        p->x.name);
}

static Symbol prevg;

static void globalend(void) {
        if (prevg && prevg->type->size > 0)
                print(".size %s,%d\n", prevg->x.name, prevg->type->size);
        prevg = NULL;
}

static void export2(Symbol p) {
        globalend();
        print(".global %s\n", p->x.name);
}

static void progend2(void) {
        globalend();
}

static void global2(Symbol p) {
        globalend();
        assert(p->u.seg);
        if (!p->generated) {
                print(".type %s,#%s\n", p->x.name,
                        isfunc(p->type) ? "function" : "object");
                if (p->type->size > 0)
                        print(".size %s,%d\n", p->x.name, p->type->size);
                else
                        prevg = p;
        }
        if (p->u.seg == BSS && p->sclass == STATIC)
                print(".local %s\n.common %s,%d,%d\n", p->x.name, p->x.name,
                        p->type->size, p->type->align);
        else if (p->u.seg == BSS && Aflag >= 2)
                print(".align %d\n%s:.skip %d\n", p->type->align, p->x.name,
                        p->type->size);
        else if (p->u.seg == BSS)
                print(".common %s,%d,%d\n", p->x.name, p->type->size, p->type->align);
        else
                print(".align %d\n%s:\n", p->type->align, p->x.name);
}

static void segment2(int n) {
        cseg = n;
        switch (n) {
        case CODE: print(".section \".text\"\n");   break;
        case BSS:  print(".section \".bss\"\n");    break;
        case DATA: print(".section \".data\"\n");   break;
        case LIT:  print(".section \".rodata\"\n"); break;
        }
}
Interface sparcIR = {
        1, 1, 0,  /* char */
        2, 2, 0,  /* short */
        4, 4, 0,  /* int */
        4, 4, 0,  /* long */
        4, 4, 0,  /* long long */
        4, 4, 1,  /* float */
        8, 8, 1,  /* double */
        8, 8, 1,  /* long double */
        4, 4, 0,  /* T * */
        0, 1, 0,  /* struct */
        0,  /* little_endian */
        0,  /* mulops_calls */
        1,  /* wants_callb */
        0,  /* wants_argb */
        1,  /* left_to_right */
        0,  /* wants_dag */
        0,  /* unsigned_char */
        address,
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
        local,
        progbeg,
        progend,
        segment,
        space,
        stabblock, 0, 0, stabinit, stabline, stabsym, stabtype,
        {
                1,  /* max_unaligned_load */
                rmap,
                blkfetch, blkstore, blkloop,
                _label,
                _rule,
                _nts,
                _kids,
                _string,
                _templates,
                _isinstruction,
                _ntname,
                emit2,
                doarg,
                target,
                clobber,

        }
};

Interface solarisIR = {
        1, 1, 0,  /* char */
        2, 2, 0,  /* short */
        4, 4, 0,  /* int */
        4, 4, 0,  /* long */
        4, 4, 0,  /* long long */
        4, 4, 1,  /* float */
        8, 8, 1,  /* double */
        8, 8, 1,  /* long double */
        4, 4, 0,  /* T * */
        0, 1, 0,  /* struct */
        0,      /* little_endian */
        0,      /* mulops_calls */
        1,      /* wants_callb */
        0,      /* wants_argb */
        1,      /* left_to_right */
        0,      /* wants_dag */
        0,      /* unsigned_char */
        address,
        blockbeg,
        blockend,
        defaddress,
        defconst,
        defstring,
        defsymbol2,
        emit,
        export2,
        function,
        gen,
        global2,
        import,
        local,
        progbeg,
        progend2,
        segment2,
        space,
        stabblock, 0, 0, stabinit, stabline, stabsym, stabtype,
        {
                1,      /* max_unaligned_load */
                rmap,
                blkfetch, blkstore, blkloop,
                _label,
                _rule,
                _nts,
                _kids,
                _string,
                _templates,
                _isinstruction,
                _ntname,
                emit2,
                doarg,
                target,
                clobber,

        }
};
static char rcsid[] = "$Id: sparc.md 145 2001-10-17 21:53:10Z timo $";
