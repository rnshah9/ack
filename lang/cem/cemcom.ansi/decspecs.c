/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */
/*	D E C L A R A T I O N   S P E C I F I E R   C H E C K I N G	*/

#include	"nofloat.h"
#include	"assert.h"
#include	"Lpars.h"
#include	"decspecs.h"
#include	"arith.h"
#include	"type.h"
#include	"level.h"
#include	"def.h"
#include	"noRoption.h"

extern char options[];
extern int level;
extern char *symbol2str();
extern char *type2str();
extern char *qual2str();
extern struct type *qualifier_type();

struct decspecs null_decspecs;

do_decspecs(ds)
	register struct decspecs *ds;
{
	/*	The provisional decspecs ds as obtained from the program
		is turned into a legal consistent decspecs.
	*/
	register struct type *tp = ds->ds_type;
	
	ASSERT(level != L_FORMAL1);
	
	/*
	if (ds->ds_notypegiven && !ds->ds_sc_given)
		strict("data definition lacking type or storage class");
	*/

	if (	level == L_GLOBAL &&
		(ds->ds_sc == AUTO || ds->ds_sc == REGISTER)
	)	{
		warning("no global %s variable allowed",
			symbol2str(ds->ds_sc));
		ds->ds_sc = GLOBAL;
	}

	if (level == L_FORMAL2)	{
		if (ds->ds_sc_given &&
		    ds->ds_sc != REGISTER){
			error("%s formal illegal", symbol2str(ds->ds_sc));
			ds->ds_sc = FORMAL;
		}
	}

	/*	Since type qualifiers may be associated with types by means
		of typedefs, we have to perform same basic tests down here.
	*/
	if (tp != (struct type *)0) {
		if ((ds->ds_typequal & TQ_VOLATILE) && (tp->tp_typequal & TQ_VOLATILE))
			error("indirect repeated type qualifier");
		if ((ds->ds_typequal & TQ_CONST) && (tp->tp_typequal & TQ_CONST))
			error("indirect repeated type qualifier");
		ds->ds_typequal |= tp->tp_typequal;
	}

	/*	The tests concerning types require a full knowledge of the
		type and will have to be postponed to declare_idf.
	*/

	/* some adjustments as described in RM 8.2 */
	if (tp == 0) {
		ds->ds_notypegiven = 1;
		tp = int_type;
	}
	switch (ds->ds_size)	{
	case SHORT:
		if (tp == int_type)
			tp = short_type;
		else
			error("short with illegal type");
		break;
	case LONG:
		if (tp == int_type)
			tp = long_type;
		else
#ifndef NOFLOAT
		if (tp == double_type)
			tp = lngdbl_type;
		else
#endif NOFLOAT
			error("long with illegal type");
		break;
	}
	if (ds->ds_unsigned == UNSIGNED) {
		switch (tp->tp_fund)	{
		case CHAR:
#ifndef NOROPTION
			if (options['R'])
				warning("unsigned char not allowed");
#endif
			tp = uchar_type;
			break;
		case SHORT:
#ifndef NOROPTION
			if (options['R'])
				warning("unsigned short not allowed");
#endif
			tp = ushort_type;
			break;
		case INT:
			tp = uint_type;
			break;
		case LONG:
#ifndef NOROPTION
			if (options['R'])
				warning("unsigned long not allowed");
#endif
			tp = ulong_type;
			break;
		default:
			error("unsigned with illegal type");
			break;
		}
	}
	if (ds->ds_unsigned == SIGNED) {
		switch (tp->tp_fund) {
		case CHAR:
			tp = char_type;
			break;
		case SHORT:
			tp = short_type;
			break;
		case INT:
			tp = int_type;
			break;
		case LONG:
			tp = long_type;
			break;
		default:
			error("signed with illegal type");
			break;
		}
	}

	ds->ds_type = qualifier_type(tp, ds->ds_typequal);
}

/*	Make tp into a qualified type. This is not as trivial as it
	may seem. If tp is a fundamental type the qualified type is
	either existent or will be generated.
	In case of a complex type the top of the type list will be
	replaced by a qualified version.
*/
struct type *
qualifier_type(tp, typequal)
	register struct type *tp;
	int typequal;
{
	register struct type *dtp = tp;
	register int fund = tp->tp_fund;

	while (dtp && dtp->tp_typequal != typequal)
		dtp = dtp->next;

	if (!dtp) {
		dtp = create_type(fund);
		dtp->tp_unsigned = tp->tp_unsigned;
		dtp->tp_align = tp->tp_align;
		dtp->tp_typequal = typequal;
		dtp->tp_size = tp->tp_size;
		switch (fund) {
		case POINTER:
		case ARRAY:
		case FUNCTION:
		case STRUCT:
		case UNION:
		case ENUM:
			dtp->tp_idf = tp->tp_idf;
			dtp->tp_sdef = tp->tp_sdef;
			dtp->tp_up = tp->tp_up;
			dtp->tp_field = tp->tp_field;
			dtp->tp_pointer = tp->tp_pointer;
			dtp->tp_array = tp->tp_array;
			dtp->tp_function = tp->tp_function;
			break;
		default:
			break;
		}
		dtp->next = tp->next; /* don't know head or tail */
		tp->next = dtp;
	}
	return(dtp);
}

