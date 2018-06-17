/* $Id$ */
/*
 * (c) copyright 1983 by the Vrije Universiteit, Amsterdam, The Netherlands.
 *
 *          This product is part of the Amsterdam Compiler Kit.
 *
 * Permission to use, sell, duplicate or disclose this software must be
 * obtained in writing. Requests for such permissions may be sent to
 *
 *      Dr. Andrew S. Tanenbaum
 *      Wiskundig Seminarium
 *      Vrije Universiteit
 *      Postbox 7161
 *      1007 MC Amsterdam
 *      The Netherlands
 *
 */

#include "pc.h"

void _rf(struct file* f)
{

	_curfil = f;
	if ((f->flags & 0377) != MAGIC)
		_trp(EBADF);
	if (f->flags & WRBIT)
		_trp(EREADF);
	if ((f->flags & WINDOW) == 0)
		_incpt(f);
}
