/*
  (c) copyright 1989 by the Vrije Universiteit, Amsterdam, The Netherlands.
  See the copyright notice in the ACK home directory, in the file "Copyright".
*/

/* $Header$ */

#include "misc.h"

flt_nrm(e)
	register flt_arith *e;
{
	if ((e->m1 | e->m2) == 0L)
		return;

	/* if top word is zero mov low word	*/
	/* to top word, adjust exponent value	*/
	if (e->m1 == 0L)	{
		e->m1 = e->m2;
		e->m2 = 0L;
		e->flt_exp -= 32;
	}
	if ((e->m1 & 0x80000000) == 0) {
		long l = 0x40000000;
		int cnt = -1;

		while (! (l & e->m1)) {
			l >>= 1;
			cnt--;
		}
		e->flt_exp += cnt;
		b64_sft(&(e->flt_mantissa), cnt);
	}
}
