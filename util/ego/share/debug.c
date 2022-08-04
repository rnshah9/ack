/* $Id$ */
/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/*  S H A R E D   F I L E
 *
 *  D E B U G . C
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <em_spec.h>
#include "types.h"
#include "def.h"
#include "debug.h"
#include "global.h"



int		linecount;	/* # lines in this file */
bool verbose_flag = FALSE;  /* generate verbose output ? */

/* VARARGS1 */
void error(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	fprintf(stderr,"error on line %u",linecount);
	if (filename != (char *) 0) {
		fprintf(stderr," file %s",filename);
	}
	fprintf(stderr,": ");
	vfprintf(stderr,s,ap);
	fprintf(stderr,"\n");
	abort();
	exit(-1);
}

#ifdef TRACE
/* VARARGS1 */
void OUTTRACE(const char *s, int n)
{
	fprintf(stderr,"> ");
	fprintf(stderr,s,n);
	fprintf(stderr,"\n");
}
#endif

#ifdef VERBOSE
/* VARARGS1 */
void OUTVERBOSE(const char *s, int n1, int n2)
{
	if (verbose_flag) {
		fprintf(stderr,"optimization: ");
		fprintf(stderr,s,n1,n2);
		fprintf(stderr,"\n");
	}
}
#endif



#ifdef DEBUG
/* Valid Address */

void VA(short *a) {
	if (a == (short *) 0)  error("VA: 0 argument");
	if ( ((intptr_t) a & 01) == 01) {
		/* MACHINE DEPENDENT TEST */
		error("VA: odd argument");
	}
}


/* Valid Instruction code */

void VI(short i) {
	if (i > ps_last) error("VI: illegal instr: %d", i);
}


/* Valid Line */

void VL(line_p l) {
	byte instr, optype;

	VA((short *) l);
	instr = l->l_instr;
	VI(instr);
	optype = TYPE(l);
	if (optype < OP_FIRST || optype > OP_LAST) {
		error("VL: illegal optype: %d", optype);
	}
}



/* Valid Data block */

void VD(dblock_p d) {
	byte pseudo;

	VA((short *) d);
	pseudo = d->d_pseudo;
	if (pseudo < D_FIRST || pseudo > D_LAST) {
		error("VD: illegal pseudo: %d",pseudo);
	}
}


/* Valid Object */

void VO(obj_p o) {
	offset off;

	VA((short *) o);
	off = o->o_off;
	if (off < 0 || off > 10000) {
		error("VO: unlikely offset: %d", off);
	}
}



/* Valid Proc */

void VP(proc_p p) {
	proc_id pid;
	int nrlabs;

	VA((short *) p);
	pid = p->p_id;
	if (pid <0 || pid > 1000) {
		error("VP: unlikely proc_id: %d", (int) pid);
	}
	nrlabs = p->p_nrlabels;
	if (nrlabs < 0 || nrlabs > 500) {
		error("VP: unlikely p_nrlabels: %d", nrlabs);
	}
}
#endif
