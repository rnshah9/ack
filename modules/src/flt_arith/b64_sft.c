/*
  (c) copyright 1988 by the Vrije Universiteit, Amsterdam, The Netherlands.
  See the copyright notice in the ACK home directory, in the file "Copyright".
*/

/* $Header$ */

#include "misc.h"

b64_sft(e,n)
	register struct _mantissa *e;
	register int n;
{
	if (n > 0) {
		if (n > 63) {
			e->flt_l_32 = 0;
			e->flt_h_32 = 0;
			return;
		}
		if (n >= 32) {
			e->flt_l_32 = e->flt_h_32;
			e->flt_h_32 = 0;
			n -= 32;
		}
		if (n > 0) {
			e->flt_l_32 = (e->flt_l_32 >> 1) & 0x7FFFFFFF;
			e->flt_l_32 >>= (n - 1);
			if (e->flt_h_32 != 0) {
				e->flt_l_32 |= (e->flt_h_32 << (32 - n)) & 0xFFFFFFFF;
				e->flt_h_32 = (e->flt_h_32 >> 1) & 0x7FFFFFFF;
				e->flt_h_32 >>= (n - 1);
			}
		}
		return;
	}
	n = -n;
	if (n > 0) {
		if (n > 63) {
			e->flt_l_32 = 0;
			e->flt_h_32 = 0;
			return;
		}
		if (n >= 32) {
			e->flt_h_32 = e->flt_l_32;
			e->flt_l_32 = 0;
			n -= 32;
		}
		if (n > 0) {
			e->flt_h_32 = (e->flt_h_32 << n) & 0xFFFFFFFF;
			if (e->flt_l_32 != 0) {
				long l = (e->flt_l_32 >> 1) & 0x7FFFFFFF;
				e->flt_h_32 |= (l >> (31 - n));
				e->flt_l_32 = (e->flt_l_32 << n) & 0xFFFFFFFF;
			}
		}
	}
}

b64_lsft(e)
	register struct _mantissa *e;
{
	/*	shift left 1 bit */
	e->flt_h_32 = (e->flt_h_32 << 1) & 0xFFFFFFFF;
	if (e->flt_l_32 & 0x80000000L) e->flt_h_32 |= 1;
	e->flt_l_32 = (e->flt_l_32 << 1) & 0xFFFFFFFF;
}

b64_rsft(e)
	register struct _mantissa *e;
{
	/*	shift right 1 bit */
	e->flt_l_32 = (e->flt_l_32 >> 1) & 0x7FFFFFFF;
	if (e->flt_h_32 & 1) e->flt_l_32 |= 0x80000000L;
	e->flt_h_32 = (e->flt_h_32 >> 1) & 0x7FFFFFFF;
}
