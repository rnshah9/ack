/* $Id$ */

/* Driver program for the global optimizer. It might even become the global
   optimizer itself one day ...
*/

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include "em_path.h"
#include "system.h"
#include "print.h"

enum
{
	NONE = 0,
	IC,
	CF,
	IL,
	CS,
	SR,
	UD,
	LV,
	RA,
	SP,
	BO,
	CJ,
	CA
};

static const struct
{
	const char* name;
	bool needsdescr;

} phase_data[] = {
	{},
	{ "ic" },
	{ "cf" },
	{ "il" },
	{ "cs", true },
	{ "sr" },
	{ "ud", true },
	{ "lv" },
	{ "ra" },
	{ "sp" },
	{ "bo" },
	{ "cj" },
	{ "ca" },
};

#define MAXUPHASES 64 /* max # of phases to be run */
#define MAXARGS 1024 /* mar # of args */
#define NTEMPS 4 /* # of temporary files; not tunable */

static char* tmpbase;
static char ddump[PATH_MAX]; /* data label dump file */
static char pdump[PATH_MAX]; /* procedure name dump file */
static char tmpbufs[NTEMPS * 2][PATH_MAX];

static int O2phases[] = { /* Passes for -O2 */
	CJ, BO, SP, 0
};

static int O3phases[] = { /* Passes for -O3 */
	CS, SR, CJ, BO, SP, UD, LV, RA, 0
};

static int O4phases[] = { /* Passes for -O4 */
	IL, CF, CS, SR, CJ, BO, SP, UD, LV, RA, 0
};

static int* Ophase = &O2phases[0]; /* default : -O2 */

static int nuphases; /* # of phases specified by user */
static int uphases[MAXUPHASES + 1]; /* phases to be run */

static int nfiles = NTEMPS * 2 + 1; /* leave space for tempfilenames */
static char* phargs[MAXARGS + 1];

static int keeptemps = 0;

static char** phase_args;
static int nphase_args;

static const char* descr_file;
static const char* opt_dir;
static const char* prog_name;

static int v_flag;

static void
cleanup()
{
	/*	Cleanup temporaries */

	if (!keeptemps)
	{
		register int i;

		for (i = NTEMPS * 2; i > 0; i--)
		{
			const char* f = phargs[i];
			if (f != 0 && *f != '\0' && *f != '-')
				(void)unlink(f);
		}
		if (ddump[0] != '\0')
			(void)unlink(ddump);
		if (pdump[0] != '\0')
			(void)unlink(pdump);
	}

	(void)unlink(tmpbase);
}

/*VARARGS1*/
static void fatal(const char *s, ...)
{
	/*	A fatal error occurred; exit gracefully */

	va_list ap;
	va_start(ap, s);

	fprint(STDERR, "%s: ", prog_name);
	doprnt(STDERR, s, ap);
	fprint(STDERR, "\n");
	cleanup();
	sys_stop(S_EXIT);
	/*NOTREACHED*/
}

static void
    add_file(s) char* s;
{
	/*	Add an input file to the list */

	if (nfiles >= MAXARGS)
		fatal("too many files");
	phargs[nfiles++] = s;
}

static void
    add_uphase(p) int p;
{
	/*	Add an optimizer phase to the list of phases to run */

	if (nuphases >= MAXUPHASES)
		fatal("too many phases");
	uphases[nuphases++] = p;
}

static void catch ()
{
	/*	Catch interrupts and exit gracefully */

	cleanup();
	sys_stop(S_EXIT);
}

static void
old_infiles()
{
	/*	Remove old input files unless we have to keep them around. */

	register int i;

	if (phargs[1] == pdump || keeptemps)
		return;

	for (i = 1; i <= NTEMPS; i++)
		(void)unlink(phargs[i]);
}

static void
get_infiles()
{
	/*	Make output temps from previous phase input temps of next phase. */

	register int i;
	char** dst = &phargs[1];
	char** src = &phargs[NTEMPS + 1];

	for (i = 1; i <= NTEMPS; i++)
	{
		*dst++ = *src++;
	}
}

static void
new_outfiles()
{
	static int tmpindex = 0;
	static int Bindex = 0;
	static char dig1 = '1';
	static char dig2 = '0';
	register int i;
	char** dst = &phargs[NTEMPS + 1];

	if (!Bindex)
		Bindex = strlen(tmpbufs[0]) - 2;

	for (i = 1; i <= NTEMPS; i++)
	{
		*dst = tmpbufs[tmpindex];
		(*dst)[Bindex - 1] = dig2;
		(*dst)[Bindex] = dig1;
		tmpindex++;
		dst++;
	}
	if (tmpindex >= 2 * NTEMPS)
		tmpindex = 0;
	if (++dig1 > '9')
	{
		++dig2;
		dig1 = '0';
	}
}

static void
    run_phase(phase) int phase;
{
	/*	Run one phase of the global optimizer; special cases are
	IC and CA.
  */
	static int flags_added;
	register int argc;
	register int i;
	char buf[256];
	int pid, status;

	/* Skip this phase if it requires a descr file and one hasn't been
	 * provided. */

	if (phase_data[phase].needsdescr && !descr_file)
		return;

	phargs[0] = buf;
	(void)strcpy(buf, opt_dir);
	(void)strcat(buf, "/");
	(void)strcat(buf, phase_data[phase].name);

	switch (phase)
	{
		case IC:
			/* always first */
			phargs[1] = pdump;
			phargs[2] = ddump;
			for (i = 3; i <= NTEMPS; i++)
				phargs[i] = "-";
			new_outfiles();
			argc = nfiles;
			phargs[argc] = 0;
			break;

		case CA:
			/* always last */
			old_infiles();
			get_infiles();
			phargs[NTEMPS + 1] = pdump;
			phargs[NTEMPS + 2] = ddump;
			for (i = NTEMPS + 3; i <= 2 * NTEMPS; i++)
				phargs[i] = "-";
			argc = 2 * NTEMPS + 1;
			phargs[argc] = 0;
			break;

		default:
		{
			old_infiles();
			get_infiles();
			new_outfiles();

			argc = 2 * NTEMPS + 1;
			if (descr_file)
			{
				phargs[argc++] = "-M";
				phargs[argc++] = (char*) descr_file;
			}

			for (i=0; i<nphase_args; i++)
				phargs[argc++] = phase_args[i];

			phargs[argc] = NULL;
			break;
		}
	}

	if (v_flag)
	{
		register int i = 0;

		while (phargs[i])
		{
			fprint(STDERR, "%s ", phargs[i]);
			i++;
		}
		fprint(STDERR, "\n");
	}

	status = sys_system(phargs[0], (const char* const*) phargs);
	if ((status & 0177) != 0)
	{
		fatal("%s got a unix signal", phargs[0]);
	}
	if (((status >> 8) & 0377) != 0)
	{
		cleanup();
		sys_stop(S_EXIT);
	}
}

int main(int argc, char* argv[])
{
	int opt;
	int i;

	if (signal(SIGINT, catch) == SIG_IGN)
		(void)signal(SIGINT, SIG_IGN);
	prog_name = argv[0];

	nphase_args = 0;
	phase_args = &argv[1];

	opterr = 0;
	for (;;)
	{
		int opt = getopt(argc, argv, "M:P:O:vt");
		if (opt == -1)
			break;

		switch (opt)
		{
			case 'M':
				descr_file = optarg;
				break;

			case 'P':
				opt_dir = optarg;
				break;

			case 'O':
			{
				int o = atoi(optarg);
				if (o <= 2)
					Ophase = &O2phases[0];
				else if (o == 3)
					Ophase = &O3phases[0];
				else
					Ophase = &O4phases[0];
				break;
			}

			case 't':
				keeptemps = 1;
				goto addopt;

			case 'v':
				v_flag = 1;
				goto addopt;

			case '?':
			addopt:
				phase_args[nphase_args++] = argv[optind - 1];
				break;
		}
	}

	for (i = optind; i < argc; i++)
		add_file(argv[i]);

	phase_args[nphase_args] = 0;
	if (nuphases)
		Ophase = uphases;

	if (nfiles == 2 * NTEMPS + 1)
	{
		/* 2*NTEMPS+1 was the starting value; nothing to do */
		sys_stop(S_END);
	}

	if (!opt_dir)
	{
		fatal("no correct -P flag given");
	}

	tmpbase = sys_maketempfile("ego", "");

	strcpy(ddump, tmpbase);
	strcpy(pdump, tmpbase);
	strcpy(tmpbufs[0], tmpbase);

	if (keeptemps)
	{
		(void)strcpy(ddump, ".");
		(void)strcpy(pdump, ".");
		(void)strcpy(tmpbufs[0], ".");
	}
	(void)strcat(ddump, "dd");
	(void)strcat(pdump, "pd");

	(void)strcat(tmpbufs[0], "A.BB");
	for (i=1; i<(2 * NTEMPS); i++)
		(void)strcpy(tmpbufs[i], tmpbufs[0]);

	i = strlen(tmpbufs[0]) - 4;
	tmpbufs[0][i] = 'p';
	tmpbufs[NTEMPS + 0][i] = 'p';
	tmpbufs[1][i] = 'd';
	tmpbufs[NTEMPS + 1][i] = 'd';
	tmpbufs[2][i] = 'l';
	tmpbufs[NTEMPS + 2][i] = 'l';
	tmpbufs[3][i] = 'b';
	tmpbufs[NTEMPS + 3][i] = 'b';

	run_phase(IC);
	run_phase(CF);
	while (*Ophase)
	{
		run_phase(*Ophase++);
	}
	run_phase(CA);
	cleanup();
	sys_stop(S_END);
	/*NOTREACHED*/
}
