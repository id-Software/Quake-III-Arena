#include "c.h"
extern Interface alphaIR;
extern Interface mipsebIR, mipselIR;
extern Interface sparcIR, solarisIR;
extern Interface x86IR, x86linuxIR;
extern Interface symbolicIR, symbolic64IR;
extern Interface nullIR;
extern Interface bytecodeIR;
Binding bindings[] = {
	"alpha/osf",     &alphaIR,
	"mips/irix",     &mipsebIR,
	"mips/ultrix",   &mipselIR,
	"sparc/sun",     &sparcIR,
	"sparc/solaris", &solarisIR,
	"x86/win32",	 &x86IR,
	"x86/linux",	 &x86linuxIR,
	"symbolic/osf",  &symbolic64IR,
	"symbolic/irix", &symbolicIR,
	"symbolic",      &symbolicIR,
	"null",          &nullIR,
	"bytecode",      &bytecodeIR,
	NULL,            NULL
};
