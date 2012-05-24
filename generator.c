/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * generator.c
 *
 *     code generator
 *
 * +  generator
 * -  generator_free
 *
 * -> generate
 *
 */
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

#include "arbre.h"
#include "op.h"
#include "runtime.h"
#include "vm.h"
#include "generator.h"
#include "error.h"

size_t strnlen(const char *, size_t);
char  *strndup(const char *, size_t);

#define REPORT_GEN
#include "report.h"
#undef  REPORT_GEN

static int    gen_block   (Generator *, Node *);
static int    gen_match   (Generator *, Node *);
static int    gen_bind    (Generator *, Node *);
static int    gen_node    (Generator *, Node *);
static int    gen_ident   (Generator *, Node *);
static int    gen_tuple   (Generator *, Node *);
static int    gen_add     (Generator *, Node *);
static int    gen_num     (Generator *, Node *);
static int    gen_atom    (Generator *, Node *);
static int    gen_path    (Generator *, Node *);
static int    gen_select  (Generator *, Node *);
static int    gen_apply   (Generator *, Node *);
static int    gen_access  (Generator *, Node *);
static int    gen         (Generator *, Instruction);

static void dump_path(PathEntry *p, FILE *out);
static void gen_locals(Generator *g, Node *n);

int (*OP_GENERATORS[])(Generator*, Node*) = {
    [OBLOCK]    =  gen_block,  [ODECL]     =  NULL,
    [OMATCH]    =  gen_match,  [OBIND]     =  gen_bind,
    [OMODULE]   =  NULL,       [OSELECT]   =  gen_select,
    [OWAIT]     =  NULL,       [OIDENT]    =  gen_ident,
    [OTYPE]     =  NULL,       [OADD]      =  gen_add,
    [OPATH]     =  gen_path,   [OMPATH]    =  NULL,
    [OSTRING]   =  NULL,       [OATOM]     =  gen_atom,
    [OCHAR]     =  NULL,       [ONUMBER]   =  gen_num,
    [OTUPLE]    =  gen_tuple,  [OLIST]     =  NULL,
    [OACCESS]   =  gen_access, [OAPPLY]    =  gen_apply,
    [OSEND]     =  NULL,       [ORANGE]    =  NULL,
    [OCLAUSE]   =  NULL,       [OPIPE]     =  NULL
};

static int define(Generator *g, char *ident, int reg)
{
    symtab_insert(g->tree->symbols, ident, symbol(ident, var(ident, reg)));

    return reg;
}

/*
 * Variable entry allocator
 */
Variable *var(char *name, Register reg)
{
    Variable *v = malloc(sizeof(*v));
              v->name = name;
              v->reg  = reg;
              v->type = NULL;
    return v;
}

Module *source_module(Source *src)
{
    char *sep  = strrchr(src->path, '.'),
         *name = strndup(src->path, sep - src->path);

    return module(name, NULL, 0);
}

Generator *generator(Tree *tree, Source *source)
{
    Generator *g = malloc(sizeof(*g));

    g->tree      = tree;
    g->source    = source;
    g->module    = source_module(source);
    g->slot      = 1;
    g->path      = NULL;
    g->paths     = calloc(256, sizeof(PathEntry*));
    g->pathsn    = 0;

    return g;
}

void generate(Generator *g, FILE *out)
{
    printf("generating module '%s'..\n", g->module->name);

    gen_block(g, g->tree->root);

    if (out == NULL) return;

    /* Write magic number */
    fputc(167, out);

    /* Write compiler version */
    int version = 0xffffff;
    fwrite(&version, 3, 1, out);

    /* Write path entry count */
    fwrite(&g->pathsn, 4, 1, out);

    for (int i = 0; i < g->pathsn; i++) {
        dump_path(g->paths[i], out);
    }
}

static int gen_constant(Generator *g, const char *src, TValue *tval)
{
    Sym *k = symtab_lookup(g->path->clause->ktable, src);

    if (k)
        return k->e.tval->v.number;

    int      index  = g->path->clause->kindex++;
    Value    v      = (Value){ .number = index };
    TValue  *indexv = tvalue(-1, v);

    g->path->clause->kheader[index] = tval;
    symtab_insert(g->path->clause->ktable, src, tvsymbol(src, indexv));

    return index;
}

static unsigned nextreg(Generator *g)
{
    // TODO: Check limits
    return g->path->clause->nreg++;
}

static int gen_atom(Generator *g, Node *n)
{
    Value v = (Value){ .atom = n->src };
    TValue *tval = tvalue(TYPE_ATOM, v);

    return RKASK(gen_constant(g, n->src, tval));
}

static int gen_block(Generator *g, Node *n)
{
    NodeList *ns  = n->o.block.body;
    int       reg = 0;

    while (ns) {
        reg = gen_node(g, ns->head);
        ns  = ns->tail;
    }

    return reg;
}

static int gen_access(Generator *g, Node *n)
{
    Node *lval = n->o.access.lval,
         *rval = n->o.access.rval;

    int reg = nextreg(g),
        rk = gen_node(g, rval);

    switch (lval->o.module.type) {
        case MODULE_CURRENT: {
            /* TODO: Factor this somehow */
            Value v = (Value){ .atom = g->module->name };
            int current = gen_constant(g, g->module->name, tvalue(TYPE_ATOM, v));

            switch (rval->op) {
                case OATOM: {
                    /* TODO: Use module index here when possible */
                    gen(g, iABC(OP_PATH, reg, RKASK(current), RKASK(rk)));
                    break;
                }
                case OIDENT:
                    break;
                default:
                    assert(0);
                    break;
            }
            break;
        }
        case MODULE_ROOT:
        case MODULE_NAMED:
        default:
            assert(0);
            break;
    }
    return reg;
}

static int gen_apply(Generator *g, Node *n)
{
    int lval = gen_node(g, n->o.apply.lval),
        rval = gen_node(g, n->o.apply.rval);

    int rr = nextreg(g);

    if (n->src == g->path->name || !strcmp(n->src, g->path->name)) { /* Tail-call */
        gen(g, iAD(OP_TAILCALL, rr, rval));
    } else {
        gen(g, iABC(OP_CALL, rr, lval, rval));
    }

    return rr;
}

static int gen_clause(Generator *g, Node *n, bool path)
{
    int reg = -1;
    int rega = 0;

    g->path->clause = clauseentry(n, g->path->nclauses);
    g->path->clauses[g->path->nclauses ++] = g->path->clause;

    enterscope(g->tree);
    gen_locals(g, n->o.clause.lval);
    reg = gen_block(g, n->o.clause.rval);
    exitscope(g->tree);

    /* Don't return from top-level */
    if (g->tree->symbols->parent || path) {
        if (ISK(reg)) {
            rega = nextreg(g);
            gen(g, iAD(OP_LOADK, rega, reg));
            reg = rega;
        }
        gen(g, iABC(OP_RETURN, reg, 0, 0));
    }
    gen(g, 0); /* Terminator */

    return reg;
}

static int gen_ident(Generator *g, Node *n)
{
    Sym *ident = tree_lookup(g->tree, n->src);

    if (ident) {
        return ident->e.var->reg;
    } else {
        assert(0);
    }
}

static int gen_select(Generator *g, Node *n)
{
    return 0;
}

static int gen_add(Generator *g, Node *n)
{
    int lval = gen_node(g, n->o.add.lval),
        rval = gen_node(g, n->o.add.rval);

    int reg = nextreg(g);

    gen(g, iABC(OP_ADD, reg, lval, rval));

    return reg;
}

static void gen_locals(Generator *g, Node *n)
{
    switch (n->op) {
        case OTUPLE:
            for (NodeList *ns = n->o.tuple.members ; ns ; ns = ns->tail) {
                gen_locals(g, ns->head);
            }
            break;
        case OIDENT: {
            Sym  *ident = tree_lookup(g->tree, n->src);

            if (! ident) {
                g->path->clause->nlocals ++;
                define(g, n->src, nextreg(g));
            }
            break;
        }
        case ONUMBER:
        case OATOM:
        case OSTRING:
            gen_node(g, n);
            break;
        default:
            // Ignore
            break;
    }
}

static int gen_path(Generator *g, Node *n)
{
    char *name = n->o.path.name->o.atom;

    Sym *k = symtab_lookup(g->tree->psymbols, name);

    if (k) {
        nreportf(REPORT_ERROR, n, "path '%s' already defined.", name);
        exit(1);
    }

    g->path = g->paths[g->pathsn] = pathentry(name, n, g->pathsn);

    symtab_insert(g->tree->psymbols, name, psymbol(name, g->path));

    g->pathsn ++;

    printf("%s/%s:\n", g->module->name, name);

    return gen_clause(g, n->o.path.clause, true);
}

static int gen_num(Generator *g, Node *n)
{
    int number = atoi(n->src);

    Value v = (Value){ .number = number };

    TValue *tval = tvalue(TYPE_NUMBER, v);

    // TODO: Think about inlining small numbers

    return RKASK(gen_constant(g, n->src, tval));
}

static int gen_tuple(Generator *g, Node *n)
{
    NodeList *ns;

    unsigned reg = nextreg(g);

    gen(g, iABC(OP_TUPLE, reg, n->o.tuple.arity, 0));

    ns = n->o.tuple.members;
    for (int i = 0; i < n->o.tuple.arity; i++) {
        gen(g, iABC(OP_SETTUPLE, reg, i, gen_node(g, ns->head)));
        ns = ns->tail;
    }
    return reg;
}

static int gen_bind(Generator *g, Node *n)
{
    Node *lval = n->o.match.lval,
         *rval = n->o.match.rval;

    int lreg, rreg;
    Sym *rident;

    switch (rval->op) {
        case OIDENT:
            rident = tree_lookup(g->tree, rval->src);
            break;
        case OTUPLE:
            rreg = gen_tuple(g, rval);
            break;
        default:
            rreg = gen_node(g, rval);
            break;
    }

    if (lval->op == OIDENT) {
        Sym  *ident = tree_lookup(g->tree, lval->src);

        g->path->clause->nlocals ++;

        if (ident) {
            nreportf(REPORT_ERROR, n, ERR_REDEFINITION, lval->src);
        } else { /* Create variable binding */
            lreg = define(g, lval->src, nextreg(g));
            if (ISK(rreg)) {
                gen(g, iAD(OP_LOADK, lreg, rreg));
            } else {
                gen(g, iABC(OP_MOVE, lreg, rreg, 0));
            }
        }
    } else {
        // TODO
    }
    return 0;
}

static int gen_match(Generator *g, Node *n)
{
    Node *lval = n->o.match.lval,
         *rval = n->o.match.rval;

    int larg = gen_node(g, lval),
        rarg = gen_node(g, rval);

    gen(g, iABC(OP_MATCH, 0, larg, rarg));
    gen(g, iAJ(OP_JUMP, 0, 0));

    // TODO: gen error in case of bad-match

    return 0;
}

static int gen_node(Generator *g, Node *n)
{
    return OP_GENERATORS[n->op](g, n);
}

static void dump_atom(Node *n, FILE *out)
{
    fputc(strlen(n->o.atom), out);
    fwrite(n->o.atom, strlen(n->o.atom) + 1, 1, out);
}

static void dump_number(Node *n, FILE *out)
{
    int i = atoi(n->o.number);
    fwrite(&i, sizeof(int), 1, out);
}

static void dump_node(Node *n, FILE *out)
{
    NodeList *ns;

    fputc(OP_TYPES[n->op], out);

    switch (n->op) {
        case OTUPLE:
            fputc(n->o.tuple.arity, out);
            for (ns = n->o.tuple.members ; ns ; ns = ns->tail) {
                dump_node(ns->head, out);
            }
            break;
        case OIDENT:
            /* The type is all that matters (TYPE_ANY) */
            break;
        case OATOM:
            dump_atom(n, out);
            break;
        case ONUMBER:
            dump_number(n, out);
            break;
        default:
            assert(0);
            break;
    }
}

static void dump_pattern(Node *pattern, FILE *out)
{
    dump_node(pattern, out);
}

static void dump_clause(ClauseEntry *c, FILE *out)
{
    TValue *tval;

    /* Write clause pattern */
    dump_pattern(c->node->o.clause.lval, out);

    /* Write local variable count */
    fputc(c->nreg, out);

    /* Write table entry count */
    fputc(c->kindex, out);

    /* Write header */
    for (int i = 0; i < c->kindex; i++) {
        tval = c->kheader[i];

        /* Write constant type */
        fputc(tval->t, out);

        /* Write constant value */
        switch (tval->t) {
            case TYPE_BIN:
            case TYPE_TUPLE:
            case TYPE_STRING:
                assert(0);
                break;
            case TYPE_ATOM:
                fwrite(tval->v.atom, strlen(tval->v.atom) + 1, 1, out);
                break;
            case TYPE_NUMBER:
                fwrite(&tval->v, sizeof(int), 1, out);
                break;
            default:
                break;
        }
    }
    /* Write byte-code length */
    fwrite(&c->pc, sizeof(c->pc), 1, out);

    /* Write byte-code */
    fwrite(c->code, sizeof(Instruction), c->pc , out);
}

static void dump_path(PathEntry *p, FILE *out)
{
    /* Write path attributes */
    fputc(0xff, out);

    if (false) { /* TODO: Anonymous path */
        fputc(0, out);
    } else {
        /* Write path name */
        uint8_t len = (uint8_t)strnlen(p->name, UINT8_MAX);
        fputc(len, out);
        fwrite(p->name, len, 1, out);
    }

    /* Write clause entry count */
    fputc(p->nclauses, out);

    for (int i = 0; i < p->nclauses; i++) {
        dump_clause(p->clauses[i], out);
    }
}

static int gen(Generator *g, Instruction i)
{
    ClauseEntry *clause = g->path->clause;

    if (clause->pc == clause->codesize - 1) {
        clause->codesize += 4096;
        clause->code      = realloc(clause->code, clause->codesize);
    }
    clause->code[clause->pc] = i;

    if (i)
        printf("%3lu:\t", clause->pc), op_pp(i), putchar('\n');

    return clause->pc++;
}
