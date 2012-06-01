/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * node.c
 *
 * TODO: Rename node-related identifiers to N*
 *
 */
#include  <stdlib.h>
#include  <stdio.h>

#include  "arbre.h"
#include  "color.h"
#include  "util.h"

static void opprint(OP);

const char *OP_STRINGS[] = {
    [OBLOCK]    =  "block",
    [ODECL]     =  "decl",
    [OMATCH]    =  "match",
    [OBIND]     =  "bind",
    [OMODULE]   =  "module",
    [OSELECT]   =  "select",
    [OCLAUSE]   =  "clause",
    [OWAIT]     =  "wait",
    [OIDENT]    =  "id",
    [OTYPE]     =  "type",
    [OADD]      =  "add",
    [OPATH]     =  "path",
    [OMPATH]    =  "mpath",
    [OPIPE]     =  "pipe",
    [OSTRING]   =  "str",
    [OATOM]     =  "atom",
    [OCHAR]     =  "char",
    [ONUMBER]   =  "num",
    [OTUPLE]    =  "tuple",
    [OLIST]     =  "list",
    [OACCESS]   =  "access",
    [OAPPLY]    =  "apply",
    [OSEND]     =  "send",
    [ORANGE]    =  "range",
    [OGT]       =  "gt",
    [OLT]       =  "lt",
    [OEQ]       =  "eq"
};

TYPE OP_TYPES[] = {
    [OSTRING]   =  TYPE_STRING,
    [OATOM]     =  TYPE_ATOM,
    [ONUMBER]   =  TYPE_NUMBER,
    [OTUPLE]    =  TYPE_TUPLE,
    [OLIST]     =  TYPE_LIST,
    [OIDENT]    =  TYPE_ANY
};

/*
 * Node allocator/initializer
 */
Node *node(Token *t, OP op)
{
    Node *n   = calloc(1, sizeof(*n));
    n->type   = -1;
    n->sym    = NULL;
    n->pos    = t->pos;
    n->source = t->source;
    n->src    = t->src;
    n->op     = op;
    return n;
}

/*
 * Node de-allocator
 */
void node_free(Node *n)
{
    free(n);
}

/*
 * NodeList allocator/initializer
 */
NodeList *nodelist(Node *head)
{
    NodeList *list = malloc(sizeof(*list));
    list->head = head;
    list->tail = NULL;
    list->end  = list;
    return list;
}

/*
 * Append Node `n` to NodeList `list`
 */
void append(NodeList *list, Node *n)
{
    NodeList *end;

    if (list->head) {
        end = nodelist(n);
        list->end->tail = end;
        list->end       = end;
    } else {
        list->head = n;
    }
}

/* PRINT FUNCTIONS */

/* TODO: Rename print functions to node_pp & node_lpp */
/* TODO: Contemplate visitor pattern for tree traversal */

/*
 * Print node
 */
void pp_node(Node *n)
{
    pp_nodel(n, 0);
}

/*
 * Print node at a given indentation level `lvl`
 */
void pp_nodel(Node *n, int lvl)
{
    if (! n) {
        printf("∅");
        return;
    }

    NodeList *ns;
    OP        op = n->op;

    if (lvl > 0)
        putchar('('), opprint(op), putchar(' ');

    if (op == OBLOCK) {
        ns = n->o.block.body;
        lvl ++;
        while (ns) {
            if (n->o.block.body->tail != NULL) {
                putchar('\n');
                for (int i = 0; i < lvl; i ++) putchar('\t');
            }
            pp_nodel(ns->head, lvl);
            ns = ns->tail;
        }
        putchar(')');
        if (--lvl == 0) putchar('\n');
    } else {
        switch (op) {
            case OACCESS: case OAPPLY: case ORANGE:
            case OSEND: case OPIPE: case OADD:
            case OGT: case OLT: case OEQ:
                pp_nodel(n->o.access.lval, lvl);
                putchar(' ');
                pp_nodel(n->o.access.rval, lvl);
                break;
            case OCLAUSE:
                pp_nodel(n->o.access.lval, lvl);
                putchar(' ');
                if (n->o.clause.guards) {
                    for (ns = n->o.clause.guards ; ns ; ns = ns->tail) {
                        pp_nodel(ns->head, lvl);
                    }
                } else {
                    printf("∅");
                }
                putchar(' ');
                pp_nodel(n->o.access.rval, lvl);
                break;
            case OWAIT:
                pp_nodel(n->o.wait.proc, lvl);
                break;
            case OSELECT:
                ns = n->o.select.clauses;
                lvl ++;
                while (ns) {
                    putchar('\n');
                    for (int i = 0; i < lvl; i ++) putchar('\t');
                    pp_nodel(ns->head, lvl);
                    ns = ns->tail;
                }
                if (--lvl == 0) putchar('\n');
                break;
            case ODECL:
                pp_nodel(n->o.decl.module, lvl);
                putchar(' ');
                pp_nodel(n->o.decl.args, lvl);
                putchar(' ');
                pp_nodel(n->o.decl.alias, lvl);
                break;
            case OMODULE:
                switch (n->o.module.type) {
                    case MODULE_CURRENT:
                        putchar('.');
                        break;
                    case MODULE_ROOT:
                        putchar('/');
                        break;
                    case MODULE_NAMED:
                        pp_nodel(n->o.module.path, lvl);
                        break;
                }
                break;
            case OPATH:
                pp_nodel(n->o.path.name, lvl);
                putchar(' ');
                pp_nodel(n->o.path.clause, lvl);
                break;
            case OMPATH:
                pp_nodel(n->o.mpath.clause, lvl);
                break;
            case OMATCH:
                pp_nodel(n->o.match.lval, lvl);
                putchar(' ');
                pp_nodel(n->o.match.rval, lvl);
                break;
            case OBIND:
                pp_nodel(n->o.bind.lval, lvl);
                putchar(' ');
                pp_nodel(n->o.bind.rval, lvl);
                break;
            case OLIST:
                if (n->o.list.length == 0) {
                    printf("∅");
                    break;
                }
                for (ns = n->o.list.items ; ns ; ns = ns->tail) {
                    pp_nodel(ns->head, lvl);
                }
                break;
            case OTUPLE:
                if (n->o.tuple.arity == 0) {
                    printf("∅");
                    break;
                }
                for (ns = n->o.tuple.members ; ns ; ns = ns->tail) {
                    pp_nodel(ns->head, lvl);
                    if (ns->tail) putchar(' ');
                }
                break;
            case OIDENT:
                printf("%s", n->src);
                if (n->type != -1) {
                    printf(" : %d", n->type);
                }
                break;
            case OSTRING: case ONUMBER: case OATOM:
                printf("%s", n->src);
                break;
            default: break;
        }
        putchar(')');
    }
}

/*
 * Print an OP
 */
static void opprint(OP op)
{
    ttyprint(COLOR_BOLD, OP_STRINGS[op]);
}

