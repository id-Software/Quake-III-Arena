%{
#define INTTMP 0x0100ff00
#define INTVAR 0x40ff0000
#define FLTTMP 0x000f0ff0
#define FLTVAR 0xfff00000

#define INTRET 0x00000004
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
static int      bitcount       (unsigned);
static Symbol   argreg         (int, int, int, int, int);

static Symbol ireg[32], freg2[32], d6;
static Symbol iregw, freg2w;
static int tmpregs[] = {3, 9, 10};
static Symbol blkreg;

static int gnum = 8;
static int pic;

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
acon: con     "%0"
acon: ADDRGP4  "%a"
addr: ADDI4(reg,acon)  "%1($%0)"
addr: ADDU4(reg,acon)  "%1($%0)"
addr: ADDP4(reg,acon)  "%1($%0)"
addr: acon  "%0"
addr: reg   "($%0)"
addr: ADDRFP4  "%a+%F($sp)"
addr: ADDRLP4  "%a+%F($sp)"
reg: addr  "la $%c,%0\n"  1
reg: CNSTI1  "# reg\n"  range(a, 0, 0)
reg: CNSTI2  "# reg\n"  range(a, 0, 0)
reg: CNSTI4  "# reg\n"  range(a, 0, 0)
reg: CNSTU1  "# reg\n"  range(a, 0, 0)
reg: CNSTU2  "# reg\n"  range(a, 0, 0)
reg: CNSTU4  "# reg\n"  range(a, 0, 0)
reg: CNSTP4  "# reg\n"  range(a, 0, 0)
stmt: ASGNI1(addr,reg)  "sb $%1,%0\n"  1
stmt: ASGNU1(addr,reg)  "sb $%1,%0\n"  1
stmt: ASGNI2(addr,reg)  "sh $%1,%0\n"  1
stmt: ASGNU2(addr,reg)  "sh $%1,%0\n"  1
stmt: ASGNI4(addr,reg)  "sw $%1,%0\n"  1
stmt: ASGNU4(addr,reg)  "sw $%1,%0\n"  1
stmt: ASGNP4(addr,reg)  "sw $%1,%0\n"  1
reg:  INDIRI1(addr)     "lb $%c,%0\n"  1
reg:  INDIRU1(addr)     "lbu $%c,%0\n"  1
reg:  INDIRI2(addr)     "lh $%c,%0\n"  1
reg:  INDIRU2(addr)     "lhu $%c,%0\n"  1
reg:  INDIRI4(addr)     "lw $%c,%0\n"  1
reg:  INDIRU4(addr)     "lw $%c,%0\n"  1
reg:  INDIRP4(addr)     "lw $%c,%0\n"  1

reg:  CVII4(INDIRI1(addr))     "lb $%c,%0\n"  1
reg:  CVII4(INDIRI2(addr))     "lh $%c,%0\n"  1
reg:  CVUU4(INDIRU1(addr))     "lbu $%c,%0\n"  1
reg:  CVUU4(INDIRU2(addr))     "lhu $%c,%0\n"  1
reg:  CVUI4(INDIRU1(addr))     "lbu $%c,%0\n"  1
reg:  CVUI4(INDIRU2(addr))     "lhu $%c,%0\n"  1
reg:  INDIRF4(addr)     "l.s $f%c,%0\n"  1
reg:  INDIRF8(addr)     "l.d $f%c,%0\n"  1
stmt: ASGNF4(addr,reg)  "s.s $f%1,%0\n"  1
stmt: ASGNF8(addr,reg)  "s.d $f%1,%0\n"  1
reg: DIVI4(reg,reg)  "div $%c,$%0,$%1\n"   1
reg: DIVU4(reg,reg)  "divu $%c,$%0,$%1\n"  1
reg: MODI4(reg,reg)  "rem $%c,$%0,$%1\n"   1
reg: MODU4(reg,reg)  "remu $%c,$%0,$%1\n"  1
reg: MULI4(reg,reg)  "mul $%c,$%0,$%1\n"   1
reg: MULU4(reg,reg)  "mul $%c,$%0,$%1\n"   1
rc:  con            "%0"
rc:  reg            "$%0"

reg: ADDI4(reg,rc)   "addu $%c,$%0,%1\n"  1
reg: ADDP4(reg,rc)   "addu $%c,$%0,%1\n"  1
reg: ADDU4(reg,rc)   "addu $%c,$%0,%1\n"  1
reg: BANDI4(reg,rc)  "and $%c,$%0,%1\n"   1
reg: BORI4(reg,rc)   "or $%c,$%0,%1\n"    1
reg: BXORI4(reg,rc)  "xor $%c,$%0,%1\n"   1
reg: BANDU4(reg,rc)  "and $%c,$%0,%1\n"   1
reg: BORU4(reg,rc)   "or $%c,$%0,%1\n"    1
reg: BXORU4(reg,rc)  "xor $%c,$%0,%1\n"   1
reg: SUBI4(reg,rc)   "subu $%c,$%0,%1\n"  1
reg: SUBP4(reg,rc)   "subu $%c,$%0,%1\n"  1
reg: SUBU4(reg,rc)   "subu $%c,$%0,%1\n"  1
rc5: CNSTI4         "%a"                range(a,0,31)
rc5: reg            "$%0"

reg: LSHI4(reg,rc5)  "sll $%c,$%0,%1\n"  1
reg: LSHU4(reg,rc5)  "sll $%c,$%0,%1\n"  1
reg: RSHI4(reg,rc5)  "sra $%c,$%0,%1\n"  1
reg: RSHU4(reg,rc5)  "srl $%c,$%0,%1\n"  1
reg: BCOMI4(reg)  "not $%c,$%0\n"   1
reg: BCOMU4(reg)  "not $%c,$%0\n"   1
reg: NEGI4(reg)   "negu $%c,$%0\n"  1
reg: LOADI1(reg)  "move $%c,$%0\n"  move(a)
reg: LOADU1(reg)  "move $%c,$%0\n"  move(a)
reg: LOADI2(reg)  "move $%c,$%0\n"  move(a)
reg: LOADU2(reg)  "move $%c,$%0\n"  move(a)
reg: LOADI4(reg)  "move $%c,$%0\n"  move(a)
reg: LOADP4(reg)  "move $%c,$%0\n"  move(a)
reg: LOADU4(reg)  "move $%c,$%0\n"  move(a)
reg: ADDF4(reg,reg)  "add.s $f%c,$f%0,$f%1\n"  1
reg: ADDF8(reg,reg)  "add.d $f%c,$f%0,$f%1\n"  1
reg: DIVF4(reg,reg)  "div.s $f%c,$f%0,$f%1\n"  1
reg: DIVF8(reg,reg)  "div.d $f%c,$f%0,$f%1\n"  1
reg: MULF4(reg,reg)  "mul.s $f%c,$f%0,$f%1\n"  1
reg: MULF8(reg,reg)  "mul.d $f%c,$f%0,$f%1\n"  1
reg: SUBF4(reg,reg)  "sub.s $f%c,$f%0,$f%1\n"  1
reg: SUBF8(reg,reg)  "sub.d $f%c,$f%0,$f%1\n"  1
reg: LOADF4(reg)     "mov.s $f%c,$f%0\n"       move(a)
reg: LOADF8(reg)     "mov.d $f%c,$f%0\n"       move(a)
reg: NEGF4(reg)      "neg.s $f%c,$f%0\n"       1
reg: NEGF8(reg)      "neg.d $f%c,$f%0\n"       1
reg: CVII4(reg)  "sll $%c,$%0,8*(4-%a); sra $%c,$%c,8*(4-%a)\n"  2
reg: CVUI4(reg)  "and $%c,$%0,(1<<(8*%a))-1\n"  1
reg: CVUU4(reg)  "and $%c,$%0,(1<<(8*%a))-1\n"  1
reg: CVFF4(reg)  "cvt.s.d $f%c,$f%0\n"  1
reg: CVFF8(reg)  "cvt.d.s $f%c,$f%0\n"  1
reg: CVIF4(reg)  "mtc1 $%0,$f%c; cvt.s.w $f%c,$f%c\n"  2
reg: CVIF8(reg)  "mtc1 $%0,$f%c; cvt.d.w $f%c,$f%c\n"  2
reg: CVFI4(reg)  "trunc.w.s $f2,$f%0,$%c; mfc1 $%c,$f2\n"  (a->syms[0]->u.c.v.i==4?2:LBURG_MAX)
reg: CVFI4(reg)  "trunc.w.d $f2,$f%0,$%c; mfc1 $%c,$f2\n"  (a->syms[0]->u.c.v.i==8?2:LBURG_MAX)
stmt: LABELV  "%a:\n"
stmt: JUMPV(acon)  "b %0\n"   1
stmt: JUMPV(reg)   ".cpadd $%0\nj $%0\n"  !pic
stmt: JUMPV(reg)   "j $%0\n"  pic
stmt: EQI4(reg,reg)  "beq $%0,$%1,%a\n"   1
stmt: EQU4(reg,reg)  "beq $%0,$%1,%a\n"   1
stmt: GEI4(reg,reg)  "bge $%0,$%1,%a\n"   1
stmt: GEU4(reg,reg)  "bgeu $%0,$%1,%a\n"  1
stmt: GTI4(reg,reg)  "bgt $%0,$%1,%a\n"   1
stmt: GTU4(reg,reg)  "bgtu $%0,$%1,%a\n"  1
stmt: LEI4(reg,reg)  "ble $%0,$%1,%a\n"   1
stmt: LEU4(reg,reg)  "bleu $%0,$%1,%a\n"  1
stmt: LTI4(reg,reg)  "blt $%0,$%1,%a\n"   1
stmt: LTU4(reg,reg)  "bltu $%0,$%1,%a\n"  1
stmt: NEI4(reg,reg)  "bne $%0,$%1,%a\n"   1
stmt: NEU4(reg,reg)  "bne $%0,$%1,%a\n"   1
stmt: EQF4(reg,reg)  "c.eq.s $f%0,$f%1; bc1t %a\n"  2
stmt: EQF8(reg,reg)  "c.eq.d $f%0,$f%1; bc1t %a\n"  2
stmt: LEF4(reg,reg)  "c.le.s $f%0,$f%1; bc1t %a\n"  2
stmt: LEF8(reg,reg)  "c.le.d $f%0,$f%1; bc1t %a\n"  2
stmt: LTF4(reg,reg)  "c.lt.s $f%0,$f%1; bc1t %a\n"  2
stmt: LTF8(reg,reg)  "c.lt.d $f%0,$f%1; bc1t %a\n"  2
stmt: GEF4(reg,reg)  "c.lt.s $f%0,$f%1; bc1f %a\n"  2
stmt: GEF8(reg,reg)  "c.lt.d $f%0,$f%1; bc1f %a\n"  2
stmt: GTF4(reg,reg)  "c.le.s $f%0,$f%1; bc1f %a\n"  2
stmt: GTF8(reg,reg)  "c.le.d $f%0,$f%1; bc1f %a\n"  2
stmt: NEF4(reg,reg)  "c.eq.s $f%0,$f%1; bc1f %a\n"  2
stmt: NEF8(reg,reg)  "c.eq.d $f%0,$f%1; bc1f %a\n"  2
ar:   ADDRGP4     "%a"

reg:  CALLF4(ar)  "jal %0\n"  1
reg:  CALLF8(ar)  "jal %0\n"  1
reg:  CALLI4(ar)  "jal %0\n"  1
reg:  CALLP4(ar)  "jal %0\n"  1
reg:  CALLU4(ar)  "jal %0\n"  1
stmt: CALLV(ar)  "jal %0\n"  1
ar: reg    "$%0"
ar: CNSTP4  "%a"   range(a, 0, 0x0fffffff)
stmt: RETF4(reg)  "# ret\n"  1
stmt: RETF8(reg)  "# ret\n"  1
stmt: RETI4(reg)  "# ret\n"  1
stmt: RETU4(reg)  "# ret\n"  1
stmt: RETP4(reg)  "# ret\n"  1
stmt: RETV(reg)   "# ret\n"  1
stmt: ARGF4(reg)  "# arg\n"  1
stmt: ARGF8(reg)  "# arg\n"  1
stmt: ARGI4(reg)  "# arg\n"  1
stmt: ARGP4(reg)  "# arg\n"  1
stmt: ARGU4(reg)  "# arg\n"  1

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
        print(".set reorder\n");
        pic = !IR->little_endian;
        parseflags(argc, argv);
        for (i = 0; i < argc; i++)
                if (strncmp(argv[i], "-G", 2) == 0)
                        gnum = atoi(argv[i] + 2);
                else if (strcmp(argv[i], "-pic=1") == 0
                ||       strcmp(argv[i], "-pic=0") == 0)
                        pic = argv[i][5]-'0';
        for (i = 0; i < 31; i += 2)
                freg2[i] = mkreg("%d", i, 3, FREG);
        for (i = 0; i < 32; i++)
                ireg[i]  = mkreg("%d", i, 1, IREG);
        ireg[29]->x.name = "sp";
        d6 = mkreg("6", 6, 3, IREG);
        freg2w = mkwildcard(freg2);
        iregw = mkwildcard(ireg);
        tmask[IREG] = INTTMP; tmask[FREG] = FLTTMP;
        vmask[IREG] = INTVAR; vmask[FREG] = FLTVAR;
        blkreg = mkreg("8", 8, 7, IREG);
}
static Symbol rmap(int opk) {
        switch (optype(opk)) {
        case I: case U: case P: case B:
                return iregw;
        case F:
                return freg2w;
        default:
                return 0;
        }
}
static void target(Node p) {
        assert(p);
        switch (specific(p->op)) {
        case CNST+I: case CNST+U: case CNST+P:
                if (range(p, 0, 0) == 0) {
                        setreg(p, ireg[0]);
                        p->x.registered = 1;
                }
                break;
        case CALL+V:
                rtarget(p, 0, ireg[25]);
                break;
        case CALL+F:
                rtarget(p, 0, ireg[25]);
                setreg(p, freg2[0]);
                break;
        case CALL+I: case CALL+P: case CALL+U:
                rtarget(p, 0, ireg[25]);
                setreg(p, ireg[2]);
                break;
        case RET+F:
                rtarget(p, 0, freg2[0]);
                break;
        case RET+I: case RET+U: case RET+P:
                rtarget(p, 0, ireg[2]);
                break;
        case ARG+F: case ARG+I: case ARG+P: case ARG+U: {
                static int ty0;
                int ty = optype(p->op);
                Symbol q;

                q = argreg(p->x.argno, p->syms[2]->u.c.v.i, ty, opsize(p->op), ty0);
                if (p->x.argno == 0)
                        ty0 = ty;
                if (q &&
                !(ty == F && q->x.regnode->set == IREG))
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
                if (p->x.argno == 0)
                        ty0 = ty;
                q = argreg(p->x.argno, p->syms[2]->u.c.v.i, ty, sz, ty0);
                src = getregnum(p->x.kids[0]);
                if (q == NULL && ty == F && sz == 4)
                        print("s.s $f%d,%d($sp)\n", src, p->syms[2]->u.c.v.i);
                else if (q == NULL && ty == F)
                        print("s.d $f%d,%d($sp)\n", src, p->syms[2]->u.c.v.i);
                else if (q == NULL)
                        print("sw $%d,%d($sp)\n", src, p->syms[2]->u.c.v.i);
                else if (ty == F && sz == 4 && q->x.regnode->set == IREG)
                        print("mfc1 $%d,$f%d\n", q->x.regnode->number, src);
                else if (ty == F && q->x.regnode->set == IREG)
                        print("mfc1.d $%d,$f%d\n", q->x.regnode->number, src);
                break;
        case ASGN+B:
                dalign = salign = p->syms[1]->u.c.v.i;
                blkcopy(getregnum(p->x.kids[0]), 0,
                        getregnum(p->x.kids[1]), 0,
                        p->syms[0]->u.c.v.i, tmpregs);
                break;
        case ARG+B:
                dalign = 4;
                salign = p->syms[1]->u.c.v.i;
                blkcopy(29, p->syms[2]->u.c.v.i,
                        getregnum(p->x.kids[0]), 0,
                        p->syms[0]->u.c.v.i, tmpregs);
                n   = p->syms[2]->u.c.v.i + p->syms[0]->u.c.v.i;
                dst = p->syms[2]->u.c.v.i;
                for ( ; dst <= 12 && dst < n; dst += 4)
                        print("lw $%d,%d($sp)\n", (dst/4)+4, dst);
                break;
        }
}
static Symbol argreg(int argno, int offset, int ty, int sz, int ty0) {
        assert((offset&3) == 0);
        if (offset > 12)
                return NULL;
        else if (argno == 0 && ty == F)
                return freg2[12];
        else if (argno == 1 && ty == F && ty0 == F)
                return freg2[14];
        else if (argno == 1 && ty == F && sz == 8)
                return d6;  /* Pair! */
        else
                return ireg[(offset/4) + 4];
}
static void doarg(Node p) {
        static int argno;
        int align;

        if (argoffset == 0)
                argno = 0;
        p->x.argno = argno++;
        align = p->syms[1]->u.c.v.i < 4 ? 4 : p->syms[1]->u.c.v.i;
        p->syms[2] = intconst(mkactual(align,
                p->syms[0]->u.c.v.i));
}
static void local(Symbol p) {
        if (askregvar(p, rmap(ttob(p->type))) == 0)
                mkauto(p);
}
static void function(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
        int i, saved, sizefsave, sizeisave, varargs;
        Symbol r, argregs[4];

        usedmask[0] = usedmask[1] = 0;
        freemask[0] = freemask[1] = ~(unsigned)0;
        offset = maxoffset = maxargoffset = 0;
        for (i = 0; callee[i]; i++)
                ;
        varargs = variadic(f->type)
                || i > 0 && strcmp(callee[i-1]->name, "va_alist") == 0;
        for (i = 0; callee[i]; i++) {
                Symbol p = callee[i];
                Symbol q = caller[i];
                assert(q);
                offset = roundup(offset, q->type->align);
                p->x.offset = q->x.offset = offset;
                p->x.name = q->x.name = stringd(offset);
                r = argreg(i, offset, optype(ttob(q->type)), q->type->size, optype(ttob(caller[0]->type)));
                if (i < 4)
                        argregs[i] = r;
                offset = roundup(offset + q->type->size, 4);
                if (varargs)
                        p->sclass = AUTO;
                else if (r && ncalls == 0 &&
                         !isstruct(q->type) && !p->addressed &&
                         !(isfloat(q->type) && r->x.regnode->set == IREG)
) {
                        p->sclass = q->sclass = REGISTER;
                        askregvar(p, r);
                        assert(p->x.regnode && p->x.regnode->vbl == p);
                        q->x = p->x;
                        q->type = p->type;
                }
                else if (askregvar(p, rmap(ttob(p->type)))
                         && r != NULL
                         && (isint(p->type) || p->type == q->type)) {
                        assert(q->sclass != REGISTER);
                        p->sclass = q->sclass = REGISTER;
                        q->type = p->type;
                }
        }
        assert(!caller[i]);
        offset = 0;
        gencode(caller, callee);
        if (ncalls)
                usedmask[IREG] |= ((unsigned)1)<<31;
        usedmask[IREG] &= 0xc0ff0000;
        usedmask[FREG] &= 0xfff00000;
        if (pic && ncalls)
                usedmask[IREG] |= 1<<25;
        maxargoffset = roundup(maxargoffset, usedmask[FREG] ? 8 : 4);
        if (ncalls && maxargoffset < 16)
                maxargoffset = 16;
        sizefsave = 4*bitcount(usedmask[FREG]);
        sizeisave = 4*bitcount(usedmask[IREG]);
        framesize = roundup(maxargoffset + sizefsave
                + sizeisave + maxoffset, 8);
        segment(CODE);
        print(".align 2\n");
        print(".ent %s\n", f->x.name);
        print("%s:\n", f->x.name);
        i = maxargoffset + sizefsave - framesize;
        print(".frame $sp,%d,$31\n", framesize);
        if (pic)
                print(".set noreorder\n.cpload $25\n.set reorder\n");
        if (framesize > 0)
                print("addu $sp,$sp,%d\n", -framesize);
        if (usedmask[FREG])
                print(".fmask 0x%x,%d\n", usedmask[FREG], i - 8);
        if (usedmask[IREG])
                print(".mask 0x%x,%d\n",  usedmask[IREG],
                        i + sizeisave - 4);
        saved = maxargoffset;
        for (i = 20; i <= 30; i += 2)
                if (usedmask[FREG]&(3<<i)) {
                        print("s.d $f%d,%d($sp)\n", i, saved);
                        saved += 8;
                }

        for (i = 16; i <= 31; i++)
                if (usedmask[IREG]&(1<<i)) {
                        if (i == 25)
                                print(".cprestore %d\n", saved);
                        else
                                print("sw $%d,%d($sp)\n", i, saved);
                        saved += 4;
                }
        for (i = 0; i < 4 && callee[i]; i++) {
                r = argregs[i];
                if (r && r->x.regnode != callee[i]->x.regnode) {
                        Symbol out = callee[i];
                        Symbol in  = caller[i];
                        int rn = r->x.regnode->number;
                        int rs = r->x.regnode->set;
                        int tyin = ttob(in->type);

                        assert(out && in && r && r->x.regnode);
                        assert(out->sclass != REGISTER || out->x.regnode);
                        if (out->sclass == REGISTER
                        && (isint(out->type) || out->type == in->type)) {
                                int outn = out->x.regnode->number;
                                if (rs == FREG && tyin == F+sizeop(8))
                                        print("mov.d $f%d,$f%d\n", outn, rn);
                                else if (rs == FREG && tyin == F+sizeop(4))
                                        print("mov.s $f%d,$f%d\n", outn, rn);
                                else if (rs == IREG && tyin == F+sizeop(8))
                                        print("mtc1.d $%d,$f%d\n", rn,   outn);
                                else if (rs == IREG && tyin == F+sizeop(4))
                                        print("mtc1 $%d,$f%d\n",   rn,   outn);
                                else
                                        print("move $%d,$%d\n",    outn, rn);
                        } else {
                                int off = in->x.offset + framesize;
                                if (rs == FREG && tyin == F+sizeop(8))
                                        print("s.d $f%d,%d($sp)\n", rn, off);
                                else if (rs == FREG && tyin == F+sizeop(4))
                                        print("s.s $f%d,%d($sp)\n", rn, off);
                                else {
                                        int i, n = (in->type->size + 3)/4;
                                        for (i = rn; i < rn+n && i <= 7; i++)
                                                print("sw $%d,%d($sp)\n", i, off + (i-rn)*4);
                                }
                        }
                }
        }
        if (varargs && callee[i-1]) {
                i = callee[i-1]->x.offset + callee[i-1]->type->size;
                for (i = roundup(i, 4)/4; i <= 3; i++)
                        print("sw $%d,%d($sp)\n", i + 4, framesize + 4*i);
                }
        emitcode();
        saved = maxargoffset;
        for (i = 20; i <= 30; i += 2)
                if (usedmask[FREG]&(3<<i)) {
                        print("l.d $f%d,%d($sp)\n", i, saved);
                        saved += 8;
                }
        for (i = 16; i <= 31; i++)
                if (usedmask[IREG]&(1<<i)) {
                        print("lw $%d,%d($sp)\n", i, saved);
                        saved += 4;
                }
        if (framesize > 0)
                print("addu $sp,$sp,%d\n", framesize);
        print("j $31\n");
        print(".end %s\n", f->x.name);
}
static void defconst(int suffix, int size, Value v) {
        if (suffix == F && size == 4) {
                float f = v.d;
                print(".word 0x%x\n", *(unsigned *)&f);
        }
        else if (suffix == F && size == 8) {
                double d = v.d;
                unsigned *p = (unsigned *)&d;
                print(".word 0x%x\n.word 0x%x\n", p[swap], p[!swap]);
        }
        else if (suffix == P)
                print(".word 0x%x\n", v.p);
        else if (size == 1)
                print(".byte 0x%x\n", suffix == I ? v.i : v.u);
        else if (size == 2)
                print(".half 0x%x\n", suffix == I ? v.i : v.u);
        else if (size == 4)
                print(".word 0x%x\n", suffix == I ? v.i : v.u);
}
static void defaddress(Symbol p) {
        if (pic && p->scope == LABELS)
                print(".gpword %s\n", p->x.name);
        else
                print(".word %s\n", p->x.name);
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
        if (p->u.seg == BSS) {
                if (p->sclass == STATIC || Aflag >= 2)
                        print(".lcomm %s,%d\n", p->x.name, p->type->size);
                else
                        print( ".comm %s,%d\n", p->x.name, p->type->size);
        } else {
                if (p->u.seg == DATA
                && (p->type->size == 0 || p->type->size > gnum))
                        print(".data\n");
                else if (p->u.seg == DATA)
                        print(".sdata\n");
                print(".align %c\n", ".01.2...3"[p->type->align]);
                print("%s:\n", p->x.name);
        }
}
static void segment(int n) {
        cseg = n;
        switch (n) {
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

        print("addu $%d,$%d,%d\n", sreg, sreg, size&~7);
        print("addu $%d,$%d,%d\n", tmps[2], dreg, size&~7);
        blkcopy(tmps[2], doff, sreg, soff, size&7, tmps);
        print("L.%d:\n", lab);
        print("addu $%d,$%d,%d\n", sreg, sreg, -8);
        print("addu $%d,$%d,%d\n", tmps[2], tmps[2], -8);
        blkcopy(tmps[2], doff, sreg, soff, 8, tmps);
        print("bltu $%d,$%d,L.%d\n", dreg, tmps[2], lab);
}
static void blkfetch(int size, int off, int reg, int tmp) {
        assert(size == 1 || size == 2 || size == 4);
        if (size == 1)
                print("lbu $%d,%d($%d)\n",  tmp, off, reg);
        else if (salign >= size && size == 2)
                print("lhu $%d,%d($%d)\n",  tmp, off, reg);
        else if (salign >= size)
                print("lw $%d,%d($%d)\n",   tmp, off, reg);
        else if (size == 2)
                print("ulhu $%d,%d($%d)\n", tmp, off, reg);
        else
                print("ulw $%d,%d($%d)\n",  tmp, off, reg);
}
static void blkstore(int size, int off, int reg, int tmp) {
        if (size == 1)
                print("sb $%d,%d($%d)\n",  tmp, off, reg);
        else if (dalign >= size && size == 2)
                print("sh $%d,%d($%d)\n",  tmp, off, reg);
        else if (dalign >= size)
                print("sw $%d,%d($%d)\n",  tmp, off, reg);
        else if (size == 2)
                print("ush $%d,%d($%d)\n", tmp, off, reg);
        else
                print("usw $%d,%d($%d)\n", tmp, off, reg);
}
static void stabinit(char *, int, char *[]);
static void stabline(Coordinate *);
static void stabsym(Symbol);

static char *currentfile;

static int bitcount(unsigned mask) {
        unsigned i, n = 0;

        for (i = 1; i; i <<= 1)
                if (mask&i)
                        n++;
        return n;
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
Interface mipsebIR = {
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
                4,      /* max_unaligned_load */
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
}, mipselIR = {
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
                4,      /* max_unaligned_load */
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
static char rcsid[] = "$Id: mips.md 145 2001-10-17 21:53:10Z timo $";
