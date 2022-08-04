/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
#ifndef NORCSID
static char rcsid[] = "$Id$";
#endif

#include <stdlib.h>
#include <stdio.h>
#include "param.h"
#include "hall.h"
#include "expr.h"
#include "extern.h"
#include "lookup.h"

char *filename;

int main(int argc, char **argv)
{
	extern int nerrors;
	extern int code_in_c;
	extern int tabledebug;
	extern int verbose;

	while (argc > 1 && argv[1][0] == '-')
	{
		switch (argv[1][1])
		{
			case 'c':
				code_in_c = 0;
				break;
			case 'd':
				tabledebug++;
				break;
			case 'v':
				verbose++;
				break;
			default:
				error("Unknown flag -%c", argv[1][1]);
		}
		argc--;
		argv++;
	}
	if (argc == 2)
	{
		if (freopen(argv[1], "r", stdin) == NULL)
		{
			error("Can't open %s", argv[1]);
			exit(-1);
		}
		filename = argv[1];
	}
	else if (argc == 1)
	{
		filename = "";
	}
	else
		error("Usage: %s [-c] [-d] [-v] [table]", argv[0]);
	initemhash();
	enterkeyw();
	initnodes();
	initio();
	yyparse();
	if (nerrors == 0)
	{
		finishio();
		if (verbose)
		{
			statistics();
			hallverbose();
		}
	}
	else
	{
		errorexit();
	}
	exit(nerrors == 0 ? 0 : -1);
}
