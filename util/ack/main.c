/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ack.h"
#include "list.h"
#include "trans.h"
#include <local.h>
#include "data.h"
#include <signal.h>

#ifndef NORCSID
static char rcs_id[] = "$Id$";
static char rcs_ack[] = RCS_ACK;
#endif

static int arg_count;

static char* srcvar(void);
static char* getsuffix(void);
static void varinit(void);
static void vieuwargs(int, char**);
static void firstarg(char*);
static int process(char*);
static int startrf(trf*);
static void block(trf*);
static int mayprep(void);
static void scanneeds(void);
static void setneeds(const char*, int);
static void noodstop(int);

int main(int argc, char** argv)
{
	register list_elem* elem;
	register char* frontend;
	register int* n_sig;
	register trf* phase;

	progname = argv[0];
	varinit();
	vieuwargs(argc, argv);
	if ((frontend = getenv("ACKFE")))
	{
		setlist(frontend);
	}
	else
	{
		setlist(FRONTENDS);
	}
	if (callname)
	{
		if (!machine)
			machine = callname;
	}
	if (!machine && !(machine = getenv("ACKM")))
	{
#ifdef ACKM
		machine = ACKM; /* The default machine */
#else
		fuerror("No machine specified");
#endif
	}
	setlist(machine);
	/* Find the linker, needed for argument building */
	scanlist(l_first(tr_list), elem)
	{
		if (t_cont(*elem)->t_linker)
		{
			linker = t_cont(*elem);
		}
	}
	transini();
	scanneeds();
	if (n_error && !k_flag)
		exit(n_error);

	if (signal(SIGINT, noodstop) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	scanlist(l_first(arguments), elem)
	{
		arg_count++;
	}

	scanlist(l_first(arguments), elem)
	{
		if (!process(l_content(*elem)) && !k_flag)
			exit(1);
	}
	orig.p_path = (char*)0;
	if (!rts)
		rts = ".";
	setsvar(keeps(RTS), rts);
	if (linker)
		getmapflags(linker);

	scanlist(l_first(tr_list), elem)
	{
		phase = t_cont(*elem);
		if (phase->t_combine && phase->t_do)
		{
			if (phase->t_blocked)
			{
#ifdef DEBUG
				if (debug)
				{
					vprint("phase %s is blocked\n", phase->t_name);
				}
#endif
				disc_inputs(phase);
				continue;
			}
			orig.p_keep = YES;
			orig.p_keeps = NO;
			orig.p_path = phase->t_origname;
			if (p_basename)
				throws(p_basename);
			if (orig.p_path)
			{
				p_basename = keeps(ack_basename(orig.p_path));
			}
			else
			{
				p_basename = 0;
			}
			if (!startrf(phase) && !k_flag)
				exit(1);
		}
	}

	if (n_error)
		exit(n_error);

	exit(0);
}

static char* srcvar(void)
{
	return orig.p_path;
}

static char* getsuffix(void)
{
	return strrchr(orig.p_path, SUFCHAR);
}

static void varinit(void)
{
	/* initialize the string variables */
	register char* envstr;
	extern char* em_dir;

	if ((envstr = getenv("ACKDIR")) != NULL)
	{
		em_dir = keeps(envstr);
	}
	setsvar(keeps(HOME), em_dir);
	setpvar(keeps(SRC), srcvar);
	setpvar(keeps(SUFFIX), getsuffix);
}

/************************* flag processing ***********************/

void vieuwargs(int argc, char** argv)
{
	register char* argp;
	register int nextarg;
	register int eaten;
	int hide;

	firstarg(argv[0]);

	nextarg = 1;

	while (nextarg < argc)
	{
		argp = argv[nextarg];
		nextarg++;
		if (argp[0] != '-' || argp[1] == 'l')
		{
			/* Not a flag, or a library */
			l_add(&arguments, argp);
			continue;
		}

		/* Flags */
		hide = NO; /* Do not hide this flags to the phases */
		eaten = 0; /* Did not 'eat' tail of flag yet */
		switch (argp[1])
		{
			case 'm':
				if (machine)
					fuerror("Two machines?");
				machine = &argp[2];
				if (*machine == '\0')
				{
					fuerror("-m needs machine name");
				}
				eaten = 1;
				break;
			case 'o':
				if (nextarg >= argc)
				{
					fuerror("-o can't be the last flag");
				}
				if (outfile)
					fuerror("Two results?");
				outfile = argv[nextarg++];
				hide = YES;
				break;
			case 'O':
				Optlevel = atoi(&argp[2]);
				if (!Optlevel)
					Optlevel = 1;
				Optlist = &argp[2];
				eaten = 1;
				break;
			case 'v':
				if (argp[2])
				{
					v_flag += atoi(&argp[2]);
					eaten = 1;
				}
				else
				{
					v_flag++;
				}
#ifdef DEBUG
				if (v_flag >= 3)
					debug = v_flag - 2;
#endif
				break;
			case 'c':
				if (stopsuffix)
					fuerror("Two -c flags");
				stopsuffix = &argp[2];
				eaten = 1;
				if (*stopsuffix && *stopsuffix != SUFCHAR)
				{
					fuerror("-c flag has invalid tail");
				}
				break;
			case 'k':
				k_flag++;
				break;
			case 't':
				t_flag++;
				break;
			case 'R':
				eaten = 1;
				break;
			case 'r':
				if (argp[2] != SUFCHAR)
				{
					error("-r must be followed by %c", SUFCHAR);
				}
				l_add(&tail_list, &argp[2]);
				eaten = 1;
				break;
			case '.':
				if (rts)
				{
					if (strcmp(rts, &argp[1]) != 0)
						fuerror("Two run-time systems?");
				}
				else
				{
					rts = &argp[1];
					l_add(&head_list, rts);
					l_add(&tail_list, rts);
				}
				eaten = 1;
				break;
			case 0:
				nill_flag++;
				eaten++;
				hide = YES;
				break;
			case 'w':
				w_flag++;
				break;
			default: /* The flag is not recognized,
			   put it on the list for the sub-processes
			*/
#ifdef DEBUG
				if (debug)
				{
					vprint("Flag %s: phase dependent\n", argp);
				}
#endif
				l_add(&flags, keeps(argp));
				eaten = 1;
				hide = YES;
		}
		if (!hide)
		{
			register char* tokeep;
			tokeep = keeps(argp);
			if (argp[1] == 'R')
			{
				l_add(&R_list, tokeep);
			}
			else
			{
				*tokeep |= NO_SCAN;
			}
			l_add(&flags, tokeep);
		}
		if (argp[2] && !eaten)
		{
			werror("Unexpected characters at end of %s", argp);
		}
	}
	return;
}

static void firstarg(char* argp)
{
	register char* name;

	name = strrchr(argp, '/');
	if (!name)
		name = strrchr(argp, '\\');
	if (name && *(name + 1))
	{
		name++;
	}
	else
	{
		name = argp;
	}
	callname = name;
}

/************************* argument processing ***********************/

static int process(char* arg)
{
	/* Process files & library arguments */
	trf* phase;
	register trf* tmp;

#ifdef DEBUG
	if (debug)
		vprint("Processing %s\n", arg);
#endif
	p_suffix = strrchr(arg, SUFCHAR);
	orig.p_keep = YES; /* Don't throw away the original ! */
	orig.p_keeps = NO;
	orig.p_path = arg;
	if (arg[0] == '-' || !p_suffix)
	{
		if (linker)
			add_input(&orig, linker);
		return 1;
	}
	if (p_basename)
		throws(p_basename);
	p_basename = keeps(ack_basename(arg));
	/* Try to find a path through the transformations */
	switch (getpath(&phase))
	{
		case F_NOPATH:
			error("Cannot produce the desired file from %s", arg);
			if (linker)
				add_input(&orig, linker);
			return 1;
		case F_NOMATCH:
			if (stopsuffix)
				werror("Unknown suffix in %s", arg);
			if (linker)
				add_input(&orig, linker);
			return 1;
		case F_TRANSFORM:
			break;
	}
	if (!phase)
		return 1;
	for (tmp = phase; tmp; tmp = tmp->t_next)
		if (!tmp->t_visited)
		{
			/* The flags are set up once.
		   At the first time each phase is in a list.
		   The program name and flags may already be touched
		   by vieuwargs.
		*/
			tmp->t_visited = YES;
			if (tmp->t_priority < 0)
				werror("Using phase %s (negative priority)", tmp->t_name);
			if (!rts && tmp->t_rts)
				rts = tmp->t_rts;
			if (tmp->t_needed)
			{
				add_head(tmp->t_needed);
				add_tail(tmp->t_needed);
			}
		}
	if (phase->t_combine)
	{
		add_input(&orig, phase);
		return 1;
	}
	in = orig;
	return startrf(phase);
}

static int startrf(trf* first)
{
	/* Start the transformations at the indicated phase */
	register trf* phase;

	phase = first;
	for (;;)
	{
		int do_preprocess = 0;
		int only_prep = 0;

		switch (phase->t_prep)
		{
			/* BEWARE, sign extension */
			case NO:
				break;
			default:
				if (!mayprep())
					break;
			case YES:
				do_preprocess = 1;
				break;
		}
		if (cpp_trafo && stopsuffix && strcmp(cpp_trafo->t_out, stopsuffix) == 0)
		{
			/* user explicitly asked for preprocessing */
			do_preprocess = 1;
			only_prep = 1;
		}

		if (do_preprocess && !transform(cpp_trafo))
		{
			n_error++;
#ifdef DEBUG
			vprint("Pre-processor failed\n");
#endif
			return 0;
		}
		if (only_prep)
		{
			break;
		}
		if (!transform(phase))
		{
			n_error++;
			block(phase->t_next);
#ifdef DEBUG
			if (debug)
			{
				if (!orig.p_path)
				{
					vprint("phase %s failed\n", phase->t_name);
				}
				else
				{
					vprint("phase %s for %s failed\n", phase->t_name, orig.p_path);
				}
			}
#endif
			return 0;
		}
		first = NO;
		phase = phase->t_next;
		if (!phase)
		{
#ifdef DEBUG
			if (debug)
				vprint("Transformation sequence complete for %s\n", orig.p_path);
#endif
			/* No more work on this file */
			if (!in.p_keep)
			{
				fatal("attempt to discard the result file");
			}
			if (in.p_keeps)
				throws(in.p_path);
			in.p_keep = NO;
			in.p_keeps = NO;
			in.p_path = (char*)0;
			return 1;
		}
		if (phase->t_combine)
		{
			add_input(&in, phase);
			break;
		}
	}
	return 1;
}

static void block(trf* first)
{
	/* One of the input files of this phase could not be produced,
	   block all combiners taking their input from this one.
	*/
	register trf* phase;
	for (phase = first; phase; phase = phase->t_next)
	{
		if (phase->t_combine)
			phase->t_blocked = YES;
	}
}

static int mayprep(void)
{
	int file;
	char fc;
	file = open(in.p_path, 0);
	if (file < 0)
		return 0;
	if (read(file, &fc, 1) != 1)
		fc = 0;
	close(file);
	return fc == '#';
}

static void scanneeds(void)
{
	register list_elem* elem;
	scanlist(l_first(head_list), elem)
	{
		setneeds(l_content(*elem), 0);
	}
	l_clear(&head_list);
	scanlist(l_first(tail_list), elem)
	{
		setneeds(l_content(*elem), 1);
	}
	l_clear(&tail_list);
}

static void setneeds(const char* suffix, int tail)
{
	trf* phase;

	p_suffix = suffix;
	switch (getpath(&phase))
	{
		case F_TRANSFORM:
			for (; phase; phase = phase->t_next)
			{
				if (phase->t_needed)
				{
					if (tail)
						add_tail(phase->t_needed);
					else
						add_head(phase->t_needed);
				}
			}
			break;
		case F_NOMATCH:
			werror("\"%s\": unrecognized suffix", suffix);
			break;
		case F_NOPATH:
			werror("sorry, cannot produce the desired file(s) from %s files", suffix);
			break;
	}
}

static void noodstop(int sig)
{
	quit(-3);
}
