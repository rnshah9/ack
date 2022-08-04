/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 * Author: Ceriel J.H. Jacobs
 */
#ifndef NODE_H_
#define NODE_H_

#include "em_arith.h"
#include "em_label.h"

/* N O D E   O F   A N   A B S T R A C T   P A R S E T R E E */



struct node {
	char nd_class;		/* kind of node */
#define Value	0		/* constant */
#define Arrsel  1		/* array selection */
#define Oper	2		/* binary operator */
#define Uoper	3		/* unary operator */
#define Arrow	4		/* ^ construction */
#define Call	5		/* cast or procedure - or function call */
#define Name	6		/* an identifier */
#define Set	7		/* a set constant */
#define Xset	8		/* a set */
#define Def	9		/* an identified name */
#define Stat	10		/* a statement */
#define Select	11		/* a '.' selection */
#define Link	12
				/* do NOT change the order or the numbers!!! */
	char nd_flags;		/* options */
#define ROPTION	1
#define AOPTION 2
	struct type *nd_type;	/* type of this node */
	struct token nd_token;
#define nd_set		nd_token.tk_data.tk_set
#define nd_def		nd_token.tk_data.tk_def
#define nd_LEFT		nd_token.tk_data.tk_left
#define nd_RIGHT	nd_token.tk_data.tk_right
#define nd_NEXT		nd_token.tk_data.tk_next
#define nd_symb		nd_token.tk_symb
#define nd_lineno	nd_token.tk_lineno
#define nd_IDF		nd_token.TOK_IDF
#define nd_SSTR		nd_token.TOK_SSTR
#define nd_STR		nd_token.TOK_STR
#define nd_SLE		nd_token.TOK_SLE
#define nd_INT		nd_token.TOK_INT
#define nd_REAL		nd_token.TOK_REAL
#define nd_RSTR		nd_token.TOK_RSTR
#define nd_RVAL		nd_token.TOK_RVAL
};

#define	new_node() ((struct node*) calloc(1, sizeof(struct node)))
#define	free_node(p) free(p)

extern struct node *dot2node(), *dot2leaf(), *getnode();

#define NULLNODE ((struct node *) 0)

#define HASSELECTORS	002
#define VARIABLE	004
#define VALUE		010

#define	IsCast(lnd)	((lnd)->nd_class == Def && is_type((lnd)->nd_def))
#define	IsProc(lnd)	((lnd)->nd_type->tp_fund == T_PROCEDURE)



struct node *getnode(int class);
struct node *dot2node(int class, struct node *left, struct node *right);
struct node *dot2leaf(int class);
void FreeNode(register struct node *nd);
int NodeCrash(register struct node* expp, label exit_label, int end_reached);
int PNodeCrash(struct node **expp, int flags);

#endif /* NODE_H_ */
