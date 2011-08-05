#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

Includelist	includelist[NINCLUDE];

extern char	*objname;

void appendDirToIncludeList( char *dir )
{
	int i;
	char *fqdir;

	fqdir = (char *)newstring( (uchar *)includelist[NINCLUDE-1].file, 256, 0 );
	strcat( fqdir, "/" );
	strcat( fqdir, dir );

	//avoid adding it more than once
	for (i=NINCLUDE-2; i>=0; i--) {
		if (includelist[i].file &&
				!strcmp (includelist[i].file, fqdir)) {
			return;
		}
	}

	for (i=NINCLUDE-2; i>=0; i--) {
		if (includelist[i].file==NULL) {
			includelist[i].always = 1;
			includelist[i].file = fqdir;
			break;
		}
	}
	if (i<0)
		error(FATAL, "Too many -I directives");
}

void
doinclude(Tokenrow *trp)
{
	char fname[256], iname[256];
	Includelist *ip;
	int angled, len, fd, i;

	trp->tp += 1;
	if (trp->tp>=trp->lp)
		goto syntax;
	if (trp->tp->type!=STRING && trp->tp->type!=LT) {
		len = trp->tp - trp->bp;
		expandrow(trp, "<include>");
		trp->tp = trp->bp+len;
	}
	if (trp->tp->type==STRING) {
		len = trp->tp->len-2;
		if (len > sizeof(fname) - 1)
			len = sizeof(fname) - 1;
		strncpy(fname, (char*)trp->tp->t+1, len);
		angled = 0;
	} else if (trp->tp->type==LT) {
		len = 0;
		trp->tp++;
		while (trp->tp->type!=GT) {
			if (trp->tp>trp->lp || len+trp->tp->len+2 >= sizeof(fname))
				goto syntax;
			strncpy(fname+len, (char*)trp->tp->t, trp->tp->len);
			len += trp->tp->len;
			trp->tp++;
		}
		angled = 1;
	} else
		goto syntax;
	trp->tp += 2;
	if (trp->tp < trp->lp || len==0)
		goto syntax;
	fname[len] = '\0';

	appendDirToIncludeList( basepath( fname ) );

	if (fname[0]=='/') {
		fd = open(fname, 0);
		strcpy(iname, fname);
	} else for (fd = -1,i=NINCLUDE-1; i>=0; i--) {
		ip = &includelist[i];
		if (ip->file==NULL || ip->deleted || (angled && ip->always==0))
			continue;
		if (strlen(fname)+strlen(ip->file)+2 > sizeof(iname))
			continue;
		strcpy(iname, ip->file);
		strcat(iname, "/");
		strcat(iname, fname);
		if ((fd = open(iname, 0)) >= 0)
			break;
	}
	if ( Mflag>1 || (!angled&&Mflag==1) ) {
		write(1,objname,strlen(objname));
		write(1,iname,strlen(iname));
		write(1,"\n",1);
	}
	if (fd >= 0) {
		if (++incdepth > 10)
			error(FATAL, "#include too deeply nested");
		setsource((char*)newstring((uchar*)iname, strlen(iname), 0), fd, NULL);
		genline();
	} else {
		trp->tp = trp->bp+2;
		error(ERROR, "Could not find include file %r", trp);
	}
	return;
syntax:
	error(ERROR, "Syntax error in #include");
	return;
}

/*
 * Generate a line directive for cursource
 */
void
genline(void)
{
	static Token ta = { UNCLASS };
	static Tokenrow tr = { &ta, &ta, &ta+1, 1 };
	uchar *p;

	ta.t = p = (uchar*)outp;
	strcpy((char*)p, "#line ");
	p += sizeof("#line ")-1;
	p = (uchar*)outnum((char*)p, cursource->line);
	*p++ = ' '; *p++ = '"';
	if (cursource->filename[0]!='/' && wd[0]) {
		strcpy((char*)p, wd);
		p += strlen(wd);
		*p++ = '/';
	}
	strcpy((char*)p, cursource->filename);
	p += strlen((char*)p);
	*p++ = '"'; *p++ = '\n';
	ta.len = (char*)p-outp;
	outp = (char*)p;
	tr.tp = tr.bp;
	puttokens(&tr);
}

void
setobjname(char *f)
{
	int n = strlen(f);
	objname = (char*)domalloc(n+5);
	strcpy(objname,f);
	if(objname[n-2]=='.'){
		strcpy(objname+n-1,"$O: ");
	}else{
		strcpy(objname+n,"$O: ");
	}
}
