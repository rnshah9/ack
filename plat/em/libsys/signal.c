/* $Id$ */
#include <signal.h>

extern void _setsig(int (*)(int));

static sighandler_t vector[16] = {
      SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL,
      SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL, SIG_DFL
} ;

static const char mapvec[] = {
	0,              /* EARRAY */
	0,              /* ERANGE */
	0,              /* ESET */
	0,              /* EIOVFL */
	SIGFPE,         /* EFOVFL */
	SIGFPE,         /* EFUNDFL */
	0,              /* EIDIVZ */
	SIGFPE,         /* EFDIVZ */
	0,              /* EIUND, already ignored */
	SIGFPE,         /* EFUND */
	0,              /* ECONV */
	0,              /* 11 */
	0,              /* 12 */
	0,              /* 13 */
	0,              /* 14 */
	0,              /* 15 */
	SIGSEGV,        /* ESTACK */
	SIGSEGV,        /* EHEAP */
	0,              /* EILLINS */
	0,              /* EODDZ */
	0,              /* ECASE */
	SIGSEGV,        /* EBADMEM */
	SIGBUS,         /* EBADPTR */
	0,              /* EBADPC */
	0,              /* EBADLAE */
	SIGSYS,         /* EBADMON */
	0,              /* EBADLIN */
	0,              /* EBADGTO */
} ;

#define VECBASE 128

static          firsttime       = 1 ;
extern int      sigtrp(int mapval, int sig) ;
static int      catchtrp(int trapno) ;
static int      procesig(int signo) ;

sighandler_t signal(int sig, sighandler_t func) {
	register index, i ;
	sighandler_t  prev ;

	index= sig-1 ;
	if ( index<0 || index>=(sizeof vector/sizeof vector[0]) ) {
		return (sighandler_t) -1 ;
	}
	if ( firsttime ) {
		firsttime= 0 ;
		_setsig(catchtrp) ;
	}
	prev= vector[index] ;
	if ( prev!=func ) {
		register int mapval ;
		vector[index]= func ;
		if ( func==SIG_IGN ) {
			mapval= -3;
		} else if ( func==SIG_DFL ) {
			mapval= -2;
		} else {
			mapval=VECBASE+sig;
		}
		if ( sigtrp(mapval,sig)== -1 ) return (sighandler_t) -1;

	}
	return prev ;
}

static int catchtrp(int trapno) {
	if ( trapno>VECBASE &&
	     trapno<=VECBASE + (sizeof vector/sizeof vector[0]) ) {
		return procesig(trapno-VECBASE) ;
	}
	if ( trapno>=0 && trapno< (sizeof mapvec/sizeof mapvec[0]) &&
	     mapvec[trapno] ) {
		return procesig(mapvec[trapno]) ;
	}
	return 0 ; /* Failed to handle the trap */
}

static int procesig(int sig) {
	register index ;
	sighandler_t  trf ;

	index= sig-1 ;
	trf= vector[index] ;
	if ( trf==SIG_IGN ) return 1 ;
	if ( sig!=SIGILL && sig!=SIGTRAP ) vector[index]= SIG_IGN ;
	if ( trf==SIG_DFL ) return 0 ;
	(*trf)(sig) ;
	return 1 ;
}
