%{
#define INTTMP ((0xff<<1)|(1<<22)|(1<<25)|(1<<27))
#define INTVAR (0x3f<<9)
#define FLTTMP ((0x3f<<10)|(0x1ff<<22))
#define FLTVAR (0xff<<2)

#define INTRET 0x00000001
#define FLTRET 0x00000003

#define readsreg(p) \
        (generic((p)->op)==INDIR && (p)->kids[0]->op==VREG+P)
#define setsrc(d) ((d) && (d)->x.regnode && \
        (d)->x.regnode->set == src->x.regnode->set && \
        (d)->x.regnode->mask&src->x.regnode->mask)

#define relink(a, b) ((b)->x.prev = (a), (a)->x.next = (b))

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
static Symbol ireg[32], freg[32];
static Symbol iregw, fregw;

static int tmpregs[] = {4, 2, 3};
static Symbol blkreg;

static int cseg;

static char *currentfile;

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
acon: con     "%0"
acon: ADDRGP8  "%a"

addr: ADDI4(reg,acon)  "%1($%0)"
addr: ADDI8(reg,acon)  "%1($%0)"
addr: ADDU8(reg,acon)  "%1($%0)"
addr: ADDP8(reg,acon)  "%1($%0)"

addr: acon  "%0"
addr: reg   "($%0)"

addr: ADDRFP8  "%a+%F($sp)"
addr: ADDRLP8  "%a+%F($sp)"

reg: addr  "lda $%c,%0\n"  1

reg: CNSTI1  "# reg\n"  range(a, 0, 0)
reg: CNSTI2  "# reg\n"  range(a, 0, 0)
reg: CNSTI4  "# reg\n"  range(a, 0, 0)
reg: CNSTI8  "# reg\n"  range(a, 0, 0)
reg: CNSTU1  "# reg\n"  range(a, 0, 0)
reg: CNSTU2  "# reg\n"  range(a, 0, 0)
reg: CNSTU4  "# reg\n"  range(a, 0, 0)
reg: CNSTU8  "# reg\n"  range(a, 0, 0)
reg: CNSTP8  "# reg\n"  range(a, 0, 0)

stmt: ASGNI1(addr,reg)  "stb $%1,%0\n"  1
stmt: ASGNU1(addr,reg)  "stb $%1,%0\n"  1
stmt: ASGNI2(addr,reg)  "stw $%1,%0\n"  1
stmt: ASGNU2(addr,reg)  "stw $%1,%0\n"  1

stmt: ASGNI4(addr,reg)  "stl $%1,%0\n"  1
stmt: ASGNU4(addr,reg)  "stl $%1,%0\n"  1
stmt: ASGNI8(addr,reg)  "stq $%1,%0\n"  1
stmt: ASGNU8(addr,reg)  "stq $%1,%0\n"  1
stmt: ASGNP8(addr,reg)  "stq $%1,%0\n"  1

reg:  INDIRI1(reg)   "ldb $%c,($%0)\n"  1
reg:  INDIRI2(reg)   "ldw $%c,($%0)\n"  1
reg:  INDIRI4(addr)  "ldl $%c,%0\n"  1
reg:  INDIRI8(addr)  "ldq $%c,%0\n"  1
reg:  INDIRP8(addr)  "ldq $%c,%0\n"  1
reg:  INDIRU1(reg)   "ldbu $%c,($%0)\n"  1
reg:  INDIRU2(reg)   "ldwu $%c,($%0)\n"  1
reg:  INDIRU4(addr)  "ldl $%c,%0\nzap $%c,240,$%c\n"  2
reg:  INDIRU8(addr)  "ldq $%c,%0\n"  1

reg:  CVII4(INDIRI1(reg))   "ldb $%c,($%0)\n"  1
reg:  CVII8(INDIRI1(reg))   "ldb $%c,($%0)\n"  1
reg:  CVII4(INDIRI2(reg))   "ldw $%c,($%0)\n"  1
reg:  CVII8(INDIRI2(reg))   "ldw $%c,($%0)\n"  1
reg:  CVII8(INDIRI4(addr))  "ldl $%c,%0\n"  1

reg:  CVUU4(INDIRU1(reg))   "ldbu $%c,($%0)\n"  1
reg:  CVUU8(INDIRU1(reg))   "ldbu $%c,($%0)\n"  1
reg:  CVUU4(INDIRU2(reg))   "ldwu $%c,($%0)\n"  1
reg:  CVUU8(INDIRU2(reg))   "ldwu $%c,($%0)\n"  1
reg:  CVUU8(INDIRU4(addr))  "ldl $%c,%0\nzap $%c,240,$%c\n"  2

reg:  CVUI4(INDIRU1(reg))   "ldbu $%c,($%0)\n"  1
reg:  CVUI8(INDIRU1(reg))   "ldbu $%c,($%0)\n"  1
reg:  CVUI4(INDIRU2(reg))   "ldwu $%c,($%0)\n"  1
reg:  CVUI8(INDIRU2(reg))   "ldwu $%c,($%0)\n"  1
reg:  CVUI8(INDIRU4(addr))  "ldl $%c,%0\nzap $%c,240,$%c\n"  2

reg:  CVIU8(reg)  "mov $%0,$%c\n"  move(a)

reg:  INDIRF4(addr)     "lds $f%c,%0\n"  1
reg:  INDIRF8(addr)     "ldt $f%c,%0\n"  1
stmt: ASGNF4(addr,reg)  "sts $f%1,%0\n"  1
stmt: ASGNF8(addr,reg)  "stt $f%1,%0\n"  1

reg: MULI4(reg,rc)  "mull $%0,%1,$%c\n"   1
reg: MULI8(reg,rc)  "mulq $%0,%1,$%c\n"   1
reg: MULU4(reg,rc)  "mull $%0,%1,$%c\nzap $%c,240,$%c\n"  2
reg: MULU8(reg,rc)  "mulq $%0,%1,$%c\n"   1

reg: DIVI4(reg,rc)  "divl $%0,%1,$%c\n"   1
reg: DIVI8(reg,rc)  "divq $%0,%1,$%c\n"   1
reg: DIVU4(reg,rc)  "divlu $%0,%1,$%c\n"  1
reg: DIVU8(reg,rc)  "divqu $%0,%1,$%c\n"  1
reg: MODI4(reg,rc)  "reml $%0,%1,$%c\n"   1
reg: MODI8(reg,rc)  "remq $%0,%1,$%c\n"   1
reg: MODU4(reg,rc)  "remlu $%0,%1,$%c\n"  1
reg: MODU8(reg,rc)  "remqu $%0,%1,$%c\n"  1

rc:  con            "%0"
rc:  reg            "$%0"

reg: ADDI4(reg,rc)   "addl $%0,%1,$%c\n"  1
reg: ADDI8(reg,rc)   "addq $%0,%1,$%c\n"  1
reg: ADDP8(reg,rc)   "addq $%0,%1,$%c\n"  1
reg: ADDU4(reg,rc)   "addl $%0,%1,$%c\nzap $%c,240,$%c\n"  2
reg: ADDU8(reg,rc)   "addq $%0,%1,$%c\n"  1
reg: SUBI4(reg,rc)   "subl $%0,%1,$%c\n"  1
reg: SUBI8(reg,rc)   "subq $%0,%1,$%c\n"  1
reg: SUBP8(reg,rc)   "subq $%0,%1,$%c\n"  1
reg: SUBU4(reg,rc)   "subl $%0,%1,$%c\nzap $%c,240,$%c\n"  2
reg: SUBU8(reg,rc)   "subq $%0,%1,$%c\n"  1

reg: BANDI4(reg,rc)  "and $%0,%1,$%c\naddl $%c,0,$%c\n"   2
reg: BANDI8(reg,rc)  "and $%0,%1,$%c\n"   1
reg: BANDU4(reg,rc)  "and $%0,%1,$%c\n"   1
reg: BANDU8(reg,rc)  "and $%0,%1,$%c\n"   1
reg: BORI4(reg,rc)   "or $%0,%1,$%c\naddl $%c,0,$%c\n"   2
reg: BORI8(reg,rc)   "or $%0,%1,$%c\n"    1
reg: BORU4(reg,rc)   "or $%0,%1,$%c\n"    1
reg: BORU8(reg,rc)   "or $%0,%1,$%c\n"    1
reg: BXORI4(reg,rc)  "xor $%0,%1,$%c\naddl $%c,0,$%c\n"   2
reg: BXORI8(reg,rc)  "xor $%0,%1,$%c\n"   1
reg: BXORU4(reg,rc)  "xor $%0,%1,$%c\n"   1
reg: BXORU8(reg,rc)  "xor $%0,%1,$%c\n"   1

rc6: CNSTI4         "%a"  range(a,0,63)
rc6: CNSTI8         "%a"  range(a,0,63)
rc6: reg            "$%0"

reg: LSHI4(reg,rc6)  "sll $%0,%1,$%c\naddl $%c,0,$%c\n"   2
reg: LSHI8(reg,rc6)  "sll $%0,%1,$%c\n"  1
reg: LSHU4(reg,rc6)  "sll $%0,%1,$%c\nzap $%c,240,$%c\n"  2
reg: LSHU8(reg,rc6)  "sll $%0,%1,$%c\n"  1
reg: RSHI4(reg,rc6)  "sra $%0,%1,$%c\naddl $%c,0,$%c\n"   2
reg: RSHI8(reg,rc6)  "sra $%0,%1,$%c\n"  1
reg: RSHU4(reg,rc6)  "srl $%0,%1,$%c\n"  1
reg: RSHU8(reg,rc6)  "srl $%0,%1,$%c\n"  1

reg: BCOMI4(reg)  "not $%0,$%c\naddl $%c,0,$%c\n"   2
reg: BCOMU4(reg)  "not $%0,$%c\nzap $%c,240,$%c\n"  2
reg: BCOMI8(reg)  "not $%0,$%c\n"   1
reg: BCOMU8(reg)  "not $%0,$%c\n"   1
reg: NEGI4(reg)   "negl $%0,$%c\n"  1
reg: NEGI8(reg)   "negq $%0,$%c\n"  1
reg: LOADI1(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADI2(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADI4(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADI8(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADP8(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADU1(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADU2(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADU4(reg)  "mov $%0,$%c\n"  move(a)
reg: LOADU8(reg)  "mov $%0,$%c\n"  move(a)

reg: ADDF4(reg,reg)  "adds $f%0,$f%1,$f%c\n"  1
reg: ADDF8(reg,reg)  "addt $f%0,$f%1,$f%c\n"  1
reg: DIVF4(reg,reg)  "divs $f%0,$f%1,$f%c\n"  1
reg: DIVF8(reg,reg)  "divt $f%0,$f%1,$f%c\n"  1
reg: MULF4(reg,reg)  "muls $f%0,$f%1,$f%c\n"  1
reg: MULF8(reg,reg)  "mult $f%0,$f%1,$f%c\n"  1
reg: SUBF4(reg,reg)  "subs $f%0,$f%1,$f%c\n"  1
reg: SUBF8(reg,reg)  "subt $f%0,$f%1,$f%c\n"  1
reg: LOADF4(reg)     "fmov $f%0,$f%c\n"       move(a)
reg: LOADF8(reg)     "fmov $f%0,$f%c\n"       move(a)
reg: NEGF4(reg)      "negs $f%0,$f%c\n"       1
reg: NEGF8(reg)      "negt $f%0,$f%c\n"       1
reg: CVII4(reg)  "sll $%0,8*(8-%a),$%c\nsra $%c,8*(8-%a),$%c\n"  2
reg: CVII8(reg)  "sll $%0,8*(8-%a),$%c\nsra $%c,8*(8-%a),$%c\n"  2
reg: CVUI4(reg)  "and $%0,(1<<(8*%a))-1,$%c\n"  1
reg: CVUI8(reg)  "and $%0,(1<<(8*%a))-1,$%c\n"  1
reg: CVUU4(reg)  "and $%0,(1<<(8*%a))-1,$%c\n"  1
reg: CVUU8(reg)  "and $%0,(1<<(8*%a))-1,$%c\n"  1

reg: CVUP8(reg)  "and $%0,(1<<(8*%a))-1,$%c\n"  1

reg: CVFF4(reg)  "cvtts $f%0,$f%c\n"  1
reg: CVFF8(reg)  "cvtst $f%0,$f%c\n"  1

reg: CVIF4(reg)  "stq $%0,-56+%F($sp)\nldt $%f%c,-56+%F($sp)\ncvtqs $f%c,$f%c\n"  3
reg: CVIF8(reg)  "stq $%0,-56+%F($sp)\nldt $%f%c,-56+%F($sp)\ncvtqt $f%c,$f%c\n"  3
reg: CVIF4(INDIRI4(addr))  "lds $f%c,%0\ncvtlq $f%c,$f%c\ncvtqs $f%c,$f%c\n"  3
reg: CVIF4(INDIRI8(addr))  "ldt $f%c,%0\ncvtqs $f%c,$f%c\n"  2
reg: CVIF8(INDIRI4(addr))  "lds $f%c,%0\ncvtlq $f%c,$f%c\ncvtqt $f%c,$f%c\n"  3
reg: CVIF8(INDIRI8(addr))  "ldt $f%c,%0\ncvtqt $f%c,$f%c\n"  2

reg: CVFI4(reg)  "cvttqc $f%0,$f1\ncvtql $f1,$f1\nsts $f1,-56+%F($sp)\nldl $%c,-56+%F($sp)\n"  4
reg: CVFI8(reg)  "cvttqc $f%0,$f1\nstt $f1,-56+%F($sp)\nldq $%c,-56+%F($sp)\n"  3

stmt: LABELV  "%a:\n"

stmt: JUMPV(acon)  "br %0\n"   1
stmt: JUMPV(reg)   "jmp ($%0)\n"  1

stmt: EQI4(reg,rc6)  "cmpeq $%0,%1,$23\nbne $23,%a\n"   2
stmt: EQU4(reg,rc6)  "cmpeq $%0,%1,$23\nbne $23,%a\n"   2
stmt: EQI8(reg,rc6)  "cmpeq $%0,%1,$23\nbne $23,%a\n"   2
stmt: EQU8(reg,rc6)  "cmpeq $%0,%1,$23\nbne $23,%a\n"   2
stmt: NEI4(reg,rc6)  "cmpeq $%0,%1,$23\nbeq $23,%a\n"   2
stmt: NEU4(reg,rc6)  "cmpeq $%0,%1,$23\nbeq $23,%a\n"   2
stmt: NEI8(reg,rc6)  "cmpeq $%0,%1,$23\nbeq $23,%a\n"   2
stmt: NEU8(reg,rc6)  "cmpeq $%0,%1,$23\nbeq $23,%a\n"   2
stmt: GEI4(reg,rc6)  "cmplt $%0,%1,$23\nbeq $23,%a\n"   2
stmt: GEI8(reg,rc6)  "cmplt $%0,%1,$23\nbeq $23,%a\n"   2
stmt: GEU4(reg,rc6)  "cmpult $%0,%1,$23\nbeq $23,%a\n"  1
stmt: GEU8(reg,rc6)  "cmpult $%0,%1,$23\nbeq $23,%a\n"  1
stmt: GTI4(reg,rc6)  "cmple $%0,%1,$23\nbeq $23,%a\n"   2
stmt: GTI8(reg,rc6)  "cmple $%0,%1,$23\nbeq $23,%a\n"   2
stmt: GTU4(reg,rc6)  "cmpule $%0,%1,$23\nbeq $23,%a\n"  1
stmt: GTU8(reg,rc6)  "cmpule $%0,%1,$23\nbeq $23,%a\n"  1
stmt: LEI4(reg,rc6)  "cmple $%0,%1,$23\nbne $23,%a\n"   2
stmt: LEI8(reg,rc6)  "cmple $%0,%1,$23\nbne $23,%a\n"   2
stmt: LEU4(reg,rc6)  "cmpule $%0,%1,$23\nbne $23,%a\n"  2
stmt: LEU8(reg,rc6)  "cmpule $%0,%1,$23\nbne $23,%a\n"  2
stmt: LTI4(reg,rc6)  "cmplt $%0,%1,$23\nbne $23,%a\n"   2
stmt: LTI8(reg,rc6)  "cmplt $%0,%1,$23\nbne $23,%a\n"   2
stmt: LTU4(reg,rc6)  "cmpult $%0,%1,$23\nbne $23,%a\n"  2
stmt: LTU8(reg,rc6)  "cmpult $%0,%1,$23\nbne $23,%a\n"  2

stmt: EQF4(reg,reg)  "cmpteq $f%0,$f%1,$f1\nfbne $f1,%a\n"  2
stmt: EQF8(reg,reg)  "cmpteq $f%0,$f%1,$f1\nfbne $f1,%a\n"  2
stmt: LEF4(reg,reg)  "cmptle $f%0,$f%1,$f1\nfbne $f1,%a\n"  2
stmt: LEF8(reg,reg)  "cmptle $f%0,$f%1,$f1\nfbne $f1,%a\n"  2
stmt: LTF4(reg,reg)  "cmptlt $f%0,$f%1,$f1\nfbne $f1,%a\n"  2
stmt: LTF8(reg,reg)  "cmptlt $f%0,$f%1,$f1\nfbne $f1,%a\n"  2

stmt: NEF4(reg,reg)  "cmpteq $f%0,$f%1,$f1\nfbeq $f1,%a\n"  2
stmt: NEF8(reg,reg)  "cmpteq $f%0,$f%1,$f1\nfbeq $f1,%a\n"  2
stmt: GEF4(reg,reg)  "cmptlt $f%0,$f%1,$f1\nfbeq $f1,%a\n"  2
stmt: GEF8(reg,reg)  "cmptlt $f%0,$f%1,$f1\nfbeq $f1,%a\n"  2
stmt: GTF4(reg,reg)  "cmptle $f%0,$f%1,$f1\nfbeq $f1,%a\n"  2
stmt: GTF8(reg,reg)  "cmptle $f%0,$f%1,$f1\nfbeq $f1,%a\n"  2

ar:   ADDRGP8     "%a"
ar:   reg    "($%0)"

reg:  CALLF4(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2
reg:  CALLF8(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2
reg:  CALLI4(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2
reg:  CALLI8(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2
reg:  CALLP8(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2
reg:  CALLU4(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2
reg:  CALLU8(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2
stmt: CALLV(ar)  "jsr $26,%0\nldgp $gp,0($26)\n"  2

stmt: RETF4(reg)  "# ret\n"  1
stmt: RETF8(reg)  "# ret\n"  1
stmt: RETI4(reg)  "# ret\n"  1
stmt: RETU4(reg)  "# ret\n"  1
stmt: RETI8(reg)  "# ret\n"  1
stmt: RETU8(reg)  "# ret\n"  1
stmt: RETP8(reg)  "# ret\n"  1
stmt: RETV(reg)   "# ret\n"  1

stmt: ARGF4(reg)  "# arg\n"  1
stmt: ARGF8(reg)  "# arg\n"  1
stmt: ARGI4(reg)  "# arg\n"  1
stmt: ARGI8(reg)  "# arg\n"  1
stmt: ARGP8(reg)  "# arg\n"  1
stmt: ARGU4(reg)  "# arg\n"  1
stmt: ARGU8(reg)  "# arg\n"  1

stmt: ARGB(INDIRB(reg))       "# argb %0\n"      1
stmt: ASGNB(reg,INDIRB(reg))  "# asgnb %0 %1\n"  1

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

        for (i = 0; i < 32; i++)
                freg[i] = mkreg("%d", i, 1, FREG);
        for (i = 0; i < 32; i++)
                ireg[i]  = mkreg("%d", i, 1, IREG);
        ireg[29]->x.name = "gp";
        ireg[30]->x.name = "sp";
        fregw = mkwildcard(freg);
        iregw = mkwildcard(ireg);

        tmask[IREG] = INTTMP; tmask[FREG] = FLTTMP;
        vmask[IREG] = INTVAR; vmask[FREG] = FLTVAR;

        blkreg = mkreg("1", 1, 0xf, IREG);

}

static Symbol rmap(int opk) {
        switch (optype(opk)) {
        case I: case U: case P: case B:
                return iregw;
        case F:
                return fregw;
        default:
                return 0;
        }
}

static Symbol argreg(int offset, int ty) {
        if (offset >= 48)
                return NULL;
        else if (ty == F)
                return freg[(offset/8) + 16];
        else
                return ireg[(offset/8) + 16];
}

static void target(Node p) {
        assert(p);
        switch (specific(p->op)) {
        case CNST+I: case CNST+U: case CNST+P:
                if (range(p, 0, 0) == 0) {
                        setreg(p, ireg[31]);
                        p->x.registered = 1;
                }
                break;
        case CNST+F:
                if (p->syms[0]->u.c.v.d == 0) {
                        setreg(p, freg[31]);
                        p->x.registered = 1;
                }
                break;

        case CALL+V:
                rtarget(p, 0, ireg[27]);
                break;
        case CALL+F:
                rtarget(p, 0, ireg[27]);
                setreg(p, freg[0]);
                break;
        case CALL+I: case CALL+P: case CALL+U:
                rtarget(p, 0, ireg[27]);
                setreg(p, ireg[0]);
                break;
        case RET+F:
                rtarget(p, 0, freg[0]);
                break;
        case RET+I: case RET+U: case RET+P:
                rtarget(p, 0, ireg[0]);
                break;

        case ARG+F: case ARG+I: case ARG+P: case ARG+U: {
                Symbol q = argreg(p->syms[2]->u.c.v.i, optype(p->op));
                if (q)
                        rtarget(p, 0, q);
                break;
                }


        case ASGN+B: rtarget(p->kids[1], 0, blkreg); break;
        case ARG+B:  rtarget(p->kids[0], 0, blkreg); break;

        }
}

static void clobber(Node p) {
        assert(p);
        switch (specific(p->op)) {
        case ASGN+I: case ASGN+U:
                if (opsize(p->op) <= 2)
                        spill(1<<24, IREG, p);
                break;

        case DIV+I: case DIV+U: case MOD+I: case MOD+U:
                spill(((1<<27)|(3<<24))&~p->syms[RX]->x.regnode->mask, IREG, p);
                break;

        case CALL+F:
                spill(INTTMP | INTRET, IREG, p);
                spill(FLTTMP,          FREG, p);
                break;
        case CALL+I: case CALL+P: case CALL+U:
                spill(INTTMP,          IREG, p);
                spill(FLTTMP | FLTRET, FREG, p);
                break;
        case CALL+V:
                spill(INTTMP | INTRET, IREG, p);
                spill(FLTTMP | FLTRET, FREG, p);
                break;

        }
}

static void emit2(Node p) {
        int dst, n, src, sz, ty;
        static int ty0;
        Symbol q;

        switch (specific(p->op)) {
        case ARG+F: case ARG+I: case ARG+P: case ARG+U:
                ty = optype(p->op);
                sz = opsize(p->op);
                q = argreg(p->syms[2]->u.c.v.i, ty);
                src = getregnum(p->x.kids[0]);
                if (q)
                        break;
                else if (ty == F && sz == 4)
                        print("sts $f%d,%d($sp)\n", src, p->syms[2]->u.c.v.i - 48);
                else if (ty == F && sz == 8)
                        print("stt $f%d,%d($sp)\n", src, p->syms[2]->u.c.v.i - 48);
                else if (sz == 4)
                        print("stq $%d,%d($sp)\n", src, p->syms[2]->u.c.v.i - 48);
                else if (sz == 8)
                        print("stq $%d,%d($sp)\n", src, p->syms[2]->u.c.v.i - 48);
                else
                        assert(0);
                break;

        case ASGN+B:
                dalign = salign = p->syms[1]->u.c.v.i;
                blkcopy(getregnum(p->x.kids[0]), 0,
                        getregnum(p->x.kids[1]), 0,
                        p->syms[0]->u.c.v.i, tmpregs);
                break;


        case ARG+B: {
                int doff = p->syms[2]->u.c.v.i, soff = 0, sreg = getregnum(p->x.kids[0]);
                dalign = 8;
                salign = p->syms[1]->u.c.v.i;
                n = p->syms[0]->u.c.v.i;
                for ( ; doff <= 40 && n > 0; doff += 8) {
                        print("uldq $%d,%d($%d)\n", (doff/8)+16, soff, sreg);
                        soff += 8;
                        n -= 8;
                }
                if (n > 0)
                        blkcopy(30, doff - 48, sreg, soff, n, tmpregs);
                break;
                }

        }
}

static void doarg(Node p) {
        p->syms[2] = intconst(mkactual(8, roundup(p->syms[0]->u.c.v.i,8)));
}

static void local(Symbol p) {
        if (askregvar(p, rmap(ttob(p->type))) == 0)
                mkauto(p);
}

static int bitcount(unsigned mask) {
        unsigned i, n = 0;

        for (i = 1; i; i <<= 1)
                if (mask&i)
                        n++;
        return n;
}

static void function(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
        int i, sizeargs, saved, sizefsave, sizeisave, varargs;
        Symbol r, argregs[6];

        usedmask[0] = usedmask[1] = 0;
        freemask[0] = freemask[1] = ~(unsigned)0;
        maxargoffset = offset = maxoffset = 0;

        for (i = 0; callee[i]; i++)
                ;
        varargs = variadic(f->type)
                || i > 0 && strcmp(callee[i-1]->name, "va_alist") == 0;
        if (varargs)
                sizeargs = 2*48;
        else
                sizeargs = 48;

        for (i = 0; callee[i]; i++) {
                Symbol p = callee[i];
                Symbol q = caller[i];
                assert(q);
                if (isfloat(p->type) && varargs) {
                        p->x.offset = q->x.offset = offset - 2*48;
                        p->x.name = q->x.name = stringd(offset - 2*48);
                } else {
                        p->x.offset = q->x.offset = offset - 48;
                        p->x.name = q->x.name = stringd(offset - 48);
                }
                offset = roundup(offset, q->type->align);
                r = argreg(offset, optype(ttob(q->type)));
                if (i < 6)
                        argregs[i] = r;
                offset = roundup(offset + q->type->size, 8);
                if (varargs)
                        p->sclass = AUTO;
                else if (r && ncalls == 0 && !isstruct(q->type) && !p->addressed
) {
                        p->sclass = q->sclass = REGISTER;
                        askregvar(p, r);
                        assert(p->x.regnode && p->x.regnode->vbl == p);
                        q->x = p->x;
                        q->type = p->type;
                } else if (askregvar(p, rmap(ttob(p->type)))
                           && r != NULL /*
                           && (isint(p->type) || p->type == q->type) */
) {
                        assert(q->sclass != REGISTER);
                        p->sclass = q->sclass = REGISTER;
                        q->type = p->type;
                }

        }
        assert(!caller[i]);

        offset = sizeargs + 8;
        gencode(caller, callee);
        usedmask[IREG] &= ~(INTTMP|(0x3f<<16)|INTRET);
        usedmask[FREG] &= ~(FLTTMP|(0x3f<<16)|FLTRET);
        if (ncalls || usedmask[IREG] || usedmask[FREG])
                usedmask[IREG] |= 1<<26;
        sizefsave = 8*bitcount(usedmask[FREG]);
        sizeisave = 8*bitcount(usedmask[IREG]);
        if (maxargoffset > 48)
                maxargoffset -= 48;
        else
                maxargoffset = 0;
        if (maxoffset < sizeargs)
                maxoffset = sizeargs;
        framesize = roundup(maxargoffset + sizefsave + sizeisave + maxoffset, 16);
        segment(CODE);
        print(".ent %s\n", f->x.name);
        print("%s:\n", f->x.name);
        print("ldgp $gp,0($27)\n");
        i = maxargoffset + sizefsave - framesize;
        if (framesize > 0)
                print("lda $sp,%d($sp)\n", -framesize);
        if (usedmask[FREG])
                print(".fmask 0x%x,%d\n", usedmask[FREG], i - 8);
        if (usedmask[IREG])
                print(".mask 0x%x,%d\n",  usedmask[IREG], i + sizeisave - 8);
        print(".frame $sp,%d,$26,%d\n", framesize, sizeargs);

        saved = maxargoffset;
        for (i = 2; i <= 9; i++)
                if (usedmask[FREG]&(1<<i)) {
                        print("stt $f%d,%d($sp)\n", i, saved);
                        saved += 8;
                }

        for (i = 9; i <= 26; i++)
                if (usedmask[IREG]&(1<<i)) {
                        print("stq $%d,%d($sp)\n", i, saved);
                        saved += 8;
                }
        for (i = 0; i < 6 && callee[i]; i++) {
                r = argregs[i];
                if (r && r->x.regnode != callee[i]->x.regnode) {
                        Symbol out = callee[i];
                        Symbol in  = caller[i];
                        int rn = r->x.regnode->number;
                        int rs = r->x.regnode->set;
                        int tyin = ttob(in->type);

                        assert(out && in && r && r->x.regnode);
                        assert(out->sclass != REGISTER || out->x.regnode);
                        if (out->sclass == REGISTER) {
                                if (rs == FREG)
                                        print("fmov $f%d,$f%d\n", rn, out->x.regnode->number);
                                else
                                        print("mov $%d,$%d\n", rn, out->x.regnode->number);

                        } else {
                                int off = in->x.offset + framesize;
                                if (rs == FREG && tyin == F+sizeop(8))
                                        print("stt $f%d,%d($sp)\n", rn, off);
                                else if (rs == FREG && tyin == F+sizeop(4))
                                        print("sts $f%d,%d($sp)\n", rn, off);
                                else {
                                        int i, n = (in->type->size + 7)/8;
                                        for (i = rn; i < rn+n && i <= 21; i++)
                                                print("stq $%d,%d($sp)\n", i, off + (i-rn)*8);
                                }

                        }

                }
        }
        if (varargs && callee[i-1]) {
                i = callee[i-1]->x.offset + roundup(callee[i-1]->type->size, 8);
                for (i = (48+i)/8; i < 6; i++) {
                        print("stq $%d,%d($sp)\n",  i + 16, framesize -   48 + 8*i);
                        print("stt $f%d,%d($sp)\n", i + 16, framesize - 2*48 + 8*i);
                }
        }
        print(".prologue 1\n");

        emitcode();
        saved = maxargoffset;
        for (i = 2; i <= 9; i++)
                if (usedmask[FREG]&(1<<i)) {
                        print("ldt $f%d,%d($sp)\n", i, saved);
                        saved += 8;
                }
        for (i = 9; i <= 26; i++)
                if (usedmask[IREG]&(1<<i)) {
                        print("ldq $%d,%d($sp)\n", i, saved);
                        saved += 8;
                }
        if (framesize > 0)
                print("lda $sp,%d($sp)\n", framesize);
        print("ret\n");
        print(".end %s\n", f->x.name);

}

static void defconst(int suffix, int size, Value v) {
        if (suffix == F && size == 4) {
                float f = v.d;
                print(".long 0x%x\n", *(unsigned *)&f);
        } else if (suffix == F && size == 8) {
                double d = v.d;
                unsigned *p = (unsigned *)&d;
                print(".long 0x%x\n.long 0x%x\n", p[swap], p[!swap]);
        } else if (suffix == P)
                print(".quad 0x%X\n", v.p);
        else if (size == 1)
                print(".byte 0x%x\n", suffix == I ? v.i : v.u);
        else if (size == 2)
                print(".word 0x%x\n", suffix == I ? v.i&0xFFFF : v.u&0xFFFF);
        else if (size == 4)
                print(".long 0x%x\n", suffix == I ? v.i : v.u);
        else if (size == 8)
                print(".quad 0x%X\n", suffix == I ? v.i : v.u);

}

static void defaddress(Symbol p) {
        print(".quad %s\n", p->x.name);
}

static void defstring(int n, char *str) {
        char *s;

        for (s = str; s < str + n; s++)
                print(".byte %d\n", (*s)&0377);
}

static void export(Symbol p) {
        print(".globl %s\n", p->x.name);
}

static void import(Symbol p) {
        if (!isfunc(p->type))
                print(".extern %s %d\n", p->name, p->type->size);
}

static void defsymbol(Symbol p) {
        if (p->scope >= LOCAL && p->sclass == STATIC)
                p->x.name = stringf("L.%d", genlabel(1));
        else if (p->generated)
                p->x.name = stringf("L.%s", p->name);
        else
                assert(p->scope != CONSTANTS || isint(p->type) || isptr(p->type)),
                p->x.name = p->name;
}

static void address(Symbol q, Symbol p, long n) {
        if (p->scope == GLOBAL
        || p->sclass == STATIC || p->sclass == EXTERN)
                q->x.name = stringf("%s%s%D", p->x.name,
                        n >= 0 ? "+" : "", n);
        else {
                assert(n <= INT_MAX && n >= INT_MIN);
                q->x.offset = p->x.offset + n;
                q->x.name = stringd(q->x.offset);
        }
}

static void global(Symbol p) {
        if (p->u.seg == DATA || p->u.seg == LIT) {
                assert(p->type->align <= 8);
                print(".align %c\n", ".01.2...3"[p->type->align]);
                print("%s:\n", p->x.name);
        } else if (p->sclass == STATIC || Aflag >= 2)
                print(".lcomm %s,%d\n", p->x.name, p->type->size);
        else
                print( ".comm %s,%d\n", p->x.name, p->type->size);
}

static void segment(int n) {
        cseg = n;
        switch (n) {
        case DATA: print(".sdata\n"); break;
        case CODE: print(".text\n");  break;
        case LIT:  print(".rdata\n"); break;
        }
}

static void space(int n) {
        if (cseg != BSS)
                print(".space %d\n", n);
}

static void blkloop(int dreg, int doff, int sreg, int soff, int size, int tmps[]) {
        int lab = genlabel(1);

        print("addq $%d,%d,$%d\n",   sreg, size&~7, sreg);
        print("addq $%d,%d,$%d\n",   dreg, size&~7, tmps[2]);
        blkcopy(tmps[2], doff, sreg, soff, size&7, tmps);
        print("L.%d:\n", lab);
        print("addq $%d,%d,$%d\n",    sreg, -8, sreg);
        print("addq $%d,%d,$%d\n", tmps[2], -8, tmps[2]);
        blkcopy(tmps[2], doff, sreg, soff, 8, tmps);
        print("cmpult $%d,$%d,$23\nbne $23,L.%d\n", dreg, tmps[2], lab);
}

static void blkfetch(int size, int off, int reg, int tmp) {
        assert(size == 1 || size == 2 || size == 4 || size == 8);
        if (size == 1)
                print("ldb $%d,%d($%d)\n",  tmp, off, reg);
        else if (size == 2)
                print("ldw $%d,%d($%d)\n",  tmp, off, reg);
        else if (salign >= size && size == 4)
                print("ldl $%d,%d($%d)\n",  tmp, off, reg);
        else if (salign >= size && size == 8)
                print("ldq $%d,%d($%d)\n",  tmp, off, reg);
        else if (size == 4)
                print("uldl $%d,%d($%d)\n", tmp, off, reg);
        else
                print("uldq $%d,%d($%d)\n", tmp, off, reg);
}

static void blkstore(int size, int off, int reg, int tmp) {
        assert(size == 1 || size == 2 || size == 4 || size == 8);
        if (size == 1)
                print("stb $%d,%d($%d)\n",  tmp, off, reg);
        else if (size == 2)
                print("stw $%d,%d($%d)\n",  tmp, off, reg);
        else if (dalign >= size && size == 4)
                print("stl $%d,%d($%d)\n",  tmp, off, reg);
        else if (dalign >= size && size == 8)
                print("stq $%d,%d($%d)\n",  tmp, off, reg);
        else if (size == 4)
                print("ustl $%d,%d($%d)\n", tmp, off, reg);
        else
                print("ustq $%d,%d($%d)\n", tmp, off, reg);
}

/* stabinit - initialize stab output */
static void stabinit(char *file, int argc, char *argv[]) {
        if (file) {
                print(".file 2,\"%s\"\n", file);
                currentfile = file;
        }
}

/* stabline - emit stab entry for source coordinate *cp */
static void stabline(Coordinate *cp) {
        if (cp->file && cp->file != currentfile) {
                print(".file 2,\"%s\"\n", cp->file);
                currentfile = cp->file;
        }
        print(".loc 2,%d\n", cp->y);
}

/* stabsym - output a stab entry for symbol p */
static void stabsym(Symbol p) {
        if (p == cfunc && IR->stabline)
                (*IR->stabline)(&p->src);
}
Interface alphaIR = {
        1, 1, 0,  /* char */
        2, 2, 0,  /* short */
        4, 4, 0,  /* int */
        8, 8, 0,  /* long */
        8, 8, 0,  /* long long */
        4, 4, 1,  /* float */
        8, 8, 1,  /* double */
        8, 8, 1,  /* long double */
        8, 8, 0,  /* T * */
        0, 1, 0,  /* struct */

        1,      /* little_endian */
        0,  /* mulops_calls */
        0,  /* wants_callb */
        1,  /* wants_argb */
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
        0, 0, 0, stabinit, stabline, stabsym, 0,
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


static char rcsid[] = "$Id: alpha.md 145 2001-10-17 21:53:10Z timo $";

