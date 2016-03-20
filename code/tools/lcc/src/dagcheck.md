%{
#include "c.h"
typedef Node NODEPTR_TYPE;
#define OP_LABEL(p)     (specific((p)->op))
#define LEFT_CHILD(p)   ((p)->kids[0])
#define RIGHT_CHILD(p)  ((p)->kids[1])
#define STATE_LABEL(p)  ((p)->x.state)
#define PANIC	   error
%}
%term CNSTF=17 CNSTI=21 CNSTP=23 CNSTU=22
%term ARGB=41 ARGF=33 ARGI=37 ARGP=39 ARGU=38
%term ASGNB=57 ASGNF=49 ASGNI=53 ASGNP=55 ASGNU=54
%term INDIRB=73 INDIRF=65 INDIRI=69 INDIRP=71 INDIRU=70
%term CVFF=113 CVFI=117
%term CVIF=129 CVII=133 CVIU=134
%term CVPP=151 CVPU=150
%term CVUI=181 CVUP=183 CVUU=182
%term NEGF=193 NEGI=197
%term CALLB=217 CALLF=209 CALLI=213 CALLP=215 CALLU=214 CALLV=216
%term RETF=241 RETI=245 RETP=247 RETU=246 RETV=248
%term ADDRGP=263
%term ADDRFP=279
%term ADDRLP=295
%term ADDF=305 ADDI=309 ADDP=311 ADDU=310
%term SUBF=321 SUBI=325 SUBP=327 SUBU=326
%term LSHI=341 LSHU=342
%term MODI=357 MODU=358
%term RSHI=373 RSHU=374
%term BANDI=389 BANDU=390
%term BCOMI=405 BCOMU=406
%term BORI=421 BORU=422
%term BXORI=437 BXORU=438
%term DIVF=449 DIVI=453 DIVU=454
%term MULF=465 MULI=469 MULU=470
%term EQF=481 EQI=485 EQU=486
%term GEF=497 GEI=501 GEU=502
%term GTF=513 GTI=517 GTU=518
%term LEF=529 LEI=533 LEU=534
%term LTF=545 LTI=549 LTU=550
%term NEF=561 NEI=565 NEU=566
%term JUMPV=584
%term LABELV=600
%%
stmt: INDIRB(P) ""
stmt: INDIRF(P) ""
stmt: INDIRI(P) ""
stmt: INDIRU(P) ""
stmt: INDIRP(P) ""
stmt: CALLF(P) ""
stmt: CALLI(P) ""
stmt: CALLU(P) ""
stmt: CALLP(P) ""
stmt: V ""
bogus: I "" 1
bogus: U "" 1
bogus: P "" 1
bogus: F "" 1
bogus: B "" 1
bogus: V "" 1
I: bogus "" 1
U: bogus "" 1
P: bogus "" 1
F: bogus "" 1
B: bogus "" 1
V: bogus "" 1
F: CNSTF ""
I: CNSTI ""
P: CNSTP ""
U: CNSTU ""
V: ARGB(B) ""
V: ARGF(F) ""
V: ARGI(I) ""
V: ARGU(U) ""
V: ARGP(P) ""
V: ASGNB(P,B) ""
V: ASGNF(P,F) ""
V: ASGNI(P,I) ""
V: ASGNU(P,U) ""
V: ASGNP(P,P) ""
B: INDIRB(P) ""
F: INDIRF(P) ""
I: INDIRI(P) ""
U: INDIRU(P) ""
P: INDIRP(P) ""
I: CVII(I) ""
I: CVUI(U) ""
I: CVFI(F) ""
U: CVIU(I) ""
U: CVUU(U) ""
U: CVPU(P) ""
F: CVIF(I) ""
F: CVFF(F) ""
P: CVUP(U) ""
P: CVPP(P) ""
F: NEGF(F) ""
I: NEGI(I) ""
V: CALLB(P,P) ""
F: CALLF(P) ""
I: CALLI(P) ""
U: CALLU(P) ""
P: CALLP(P) ""
V: CALLV(P) ""
V: RETF(F) ""
V: RETI(I) ""
V: RETU(U) ""
V: RETP(P) ""
V: RETV ""
P: ADDRGP ""
P: ADDRFP ""
P: ADDRLP ""
F: ADDF(F,F) ""
I: ADDI(I,I) ""
P: ADDP(P,I) ""
P: ADDP(I,P) ""
P: ADDP(U,P) ""
P: ADDP(P,U) ""
U: ADDU(U,U) ""
F: SUBF(F,F) ""
I: SUBI(I,I) ""
P: SUBP(P,I) ""
P: SUBP(P,U) ""
U: SUBU(U,U) ""
I: LSHI(I,I) ""
U: LSHU(U,I) ""
I: MODI(I,I) ""
U: MODU(U,U) ""
I: RSHI(I,I) ""
U: RSHU(U,I) ""
U: BANDU(U,U) ""
I: BANDI(I,I) ""
U: BCOMU(U) ""
I: BCOMI(I) ""
I: BORI(I,I) ""
U: BORU(U,U) ""
U: BXORU(U,U) ""
I: BXORI(I,I) ""
F: DIVF(F,F) ""
I: DIVI(I,I) ""
U: DIVU(U,U) ""
F: MULF(F,F) ""
I: MULI(I,I) ""
U: MULU(U,U) ""
V: EQF(F,F) ""
V: EQI(I,I) ""
V: EQU(U,U) ""
V: GEF(F,F) ""
V: GEI(I,I) ""
V: GEU(U,U) ""
V: GTF(F,F) ""
V: GTI(I,I) ""
V: GTU(U,U) ""
V: LEF(F,F) ""
V: LEI(I,I) ""
V: LEU(U,U) ""
V: LTF(F,F) ""
V: LTI(I,I) ""
V: LTU(U,U) ""
V: NEF(F,F) ""
V: NEI(I,I) ""
V: NEU(U,U) ""
V: JUMPV(P) ""
V: LABELV ""
%%

static void reduce(NODEPTR_TYPE p, int goalnt) {
	int i, sz = opsize(p->op), rulenumber = _rule(p->x.state, goalnt);
	short *nts = _nts[rulenumber];
	NODEPTR_TYPE kids[10];

	assert(rulenumber);
	_kids(p, rulenumber, kids);
	for (i = 0; nts[i]; i++)
		reduce(kids[i], nts[i]);
	switch (optype(p->op)) {
#define xx(ty) if (sz == ty->size) return
	case I:
	case U:
		xx(chartype);
		xx(shorttype);
		xx(inttype);
		xx(longtype);
		xx(longlong);
		break;
	case F:
		xx(floattype);
		xx(doubletype);
		xx(longdouble);
		break;
	case P:
		xx(voidptype);
		xx(funcptype);
		break;
	case V:
	case B: if (sz == 0) return;
#undef xx
	}
	printdag(p, 2);
	assert(0);
}

void check(Node p) {
	struct _state { short cost[1]; };

	_label(p);
	if (((struct _state *)p->x.state)->cost[1] > 0) {
		printdag(p, 2);
		assert(0);
	}
	reduce(p, 1);
}
