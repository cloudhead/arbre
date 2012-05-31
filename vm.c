/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * vm.c
 *
 *   arbre virtual-machine
 *
 * +  vm
 * -  vm_free
 *
 * -> vm_open, vm_run
 *
 * TODO: Document functions
 * TODO: Abstract module/path retrieval
 *
 */
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <assert.h>

#include "hash.h"
#include "value.h"
#include "op.h"
#include "runtime.h"
#include "vm.h"
#include "error.h"
#include "bin.h"

#define DEBUG

#ifdef DEBUG
#define debug(...) (fprintf(stderr, __VA_ARGS__))
#else
#define debug(...) ;
#endif

Module *vm_module  (VM *vm, const char *name);
TValue *vm_execute (VM *vm, Process *proc);

int match (TValue *pattern, TValue *v, TValue *local);

VM *vm(void)
{
    size_t msize = sizeof(ModuleList*) * 512;
    VM    *vm    = malloc(sizeof(*vm) + msize);

    vm->path  = NULL;
    vm->pathc = 0;
    vm->paths = calloc(OPMAX_B, sizeof(Path*));

    vm->nprocs = 0;
    vm->procs  = malloc(sizeof(Process*) * 1024);
    vm->proc   = NULL;

    memset(vm->modules, 0, msize);

    return vm;
}

void vm_free(VM *vm)
{
    // TODO: Implement
}

uint8_t *vm_readk(VM *vm, uint8_t *b, TValue *k)
{
    TYPE t = *b ++;
    Value v;

    switch (t) {
        case TYPE_BIN:
        case TYPE_ATOM:
            v.atom = (char *)b;
            b += strlen(v.atom) + 1;
            debug("%s", v.atom);
            break;
        case TYPE_STRING:
            assert(0);
            break;
        case TYPE_NUMBER:
            v.number = *(int *)b;
            b += sizeof(int);
            debug("%d", v.number);
            break;
        case TYPE_ANY:
            v.ident = *b ++;
            debug("<any>");
            break;
        case TYPE_IDENT:
            v.ident = *b ++;
            debug("r%d", v.ident);
            break;
        case TYPE_TUPLE: {
            uint8_t arity = *b ++;

            debug("<tuple>");

            v.tuple = malloc(sizeof(*v.tuple) + sizeof(TValue) * arity);
            v.tuple->arity = arity;

            for (int i = 0; i < arity; i++) {
                b = vm_readk(vm, b, v.tuple->members + i);
            }
            break;
        }
        default:
            assert(0);
            break;
    }
    k->t = t;
    k->v = v;

    return b;
}

/* TODO: Paths should be per-module */
uint8_t *vm_readclause(VM *vm, uint8_t *b, int index)
{
    /* Pattern */
    TValue pattern = bin_readnode(&b);

    /* Number of locals */
    uint8_t nlocals = *b ++;

    /* Size of constants table */
    uint8_t ksize = *b ++;

    debug("reading clause with %d constant(s) and %d local(s)..\n", ksize, nlocals);

    Clause *c = clause(pattern, nlocals, ksize);

    for (int i = 0; i < ksize; i++) {
        debug("\tk%d = ", i);
        b = vm_readk(vm, b, &c->constants[i]);
        debug("\n");
    }
    debug("\n");

    c->codelen = *((unsigned long*)b);
    b += sizeof(c->codelen);

    c->code = (Instruction *)b;

    /* Skip all code */
    b += c->codelen * sizeof(Instruction);

    Path *p = vm->paths[vm->pathc - 1];

    p->clauses[index] = c;

    return b;
}

uint8_t *vm_readpath(VM *vm, uint8_t *b)
{
    char *name;

    /* Path attributes */
    uint8_t attrs   = *b ++;
    uint8_t namelen = *b ++;

    assert(attrs);

    /* TODO: check attributes instead */
    /* TODO: clean this up.. maybe with strdup */
    if (namelen > 0) {
        name = malloc(namelen + 1);
        memcpy(name, b, namelen);
        name[namelen] = '\0';
        b += namelen;
    }

    uint8_t nclauses = *b ++;

    debug("reading path '%s'..\n", name);
    debug("reading %d clause(s)..\n", nclauses);

    Path *p = path(name, nclauses);

    vm->paths[vm->pathc++] = p;

    for (int i = 0; i < nclauses; i++) {
        b = vm_readclause(vm, b, i);
    }

    return b;
}

void vm_open(VM *vm, const char *name, uint8_t *buffer)
{
    unsigned char magic = *buffer;

    if (magic != 167) { /* TODO: Make this segment 2 bytes */
        error(1, 0, "file is not an arbre bin file");
    }
    buffer ++;

    struct version v = *((struct version *)buffer);
    buffer += sizeof(struct version); /* XXX: How portable is this? */

    debug("version %d.%d.%d\n", v.major, v.minor, v.patch);

    int pathsn = *((int*)buffer);
    buffer += sizeof(int);

    debug("reading %d paths..\n", pathsn);

    for (int i = 0; i < pathsn; i++) {
        buffer = vm_readpath(vm, buffer);
    }
    vm_load(vm, name, vm->paths);
}

void vm_load(VM *vm, const char *name, Path *paths[])
{
    Module *m = module(name, paths, vm->pathc);

    uint32_t key = hash(name, strlen(name)) % 512;

    if (vm->modules[key]) {
        module_prepend(vm->modules[key], m);
    } else {
        vm->modules[key] = modulelist(m);
    }
}

int match_atom(Value pattern, Value v, TValue *local)
{
    // TODO: If we keep a global store of atoms, we only
    // need a pointer comparison here.
    if (! strcmp(pattern.atom, v.atom))
        return 0;

    return -1;
}

int match_tuple(Value pattern, Value v, TValue *local)
{
    int m = 0, nmatches = 0;

    if (pattern.tuple->arity != v.tuple->arity)
        return -1;

    for (int i = 0; i < v.tuple->arity; i++) {
        m = match(pattern.tuple->members + i, v.tuple->members + i, local + nmatches);

        if (m == -1)
            return -1;

        nmatches += m;
    }
    return nmatches;
}

int match(TValue *pattern, TValue *v, TValue *local)
{
    assert(pattern);

    if (v == NULL) {
        if (pattern->t == TYPE_TUPLE &&
            pattern->v.tuple->arity == 0) {
            return 0;
        } else {
            return -1;
        }
    }

    if (pattern->t == TYPE_ANY) {
        *local = *v;
        return 1;
    }

    if (pattern->t == TYPE_IDENT) {
        assert(0);
    }

    if (pattern->t != v->t) {
        return -1;
    }

    switch (pattern->t) {
        case TYPE_TUPLE:
            return match_tuple(pattern->v, v->v, local);
        case TYPE_ATOM:
            return match_atom(pattern->v, v->v, local);
        case TYPE_NUMBER:
            if (pattern->v.number == v->v.number)
                return 0;
            break;
        default:
            assert(0);
    }
    return -1;
}

int vm_tailcall(VM *vm, Process *proc, /*Module *m, Path *p, */Clause *c, TValue *arg)
{
    Module *m = proc->module;
    Path *p = vm->path;

    Frame *frame = *(proc->stack->frame);

    size_t size = sizeof(TValue) * c->nlocals;
    TValue *local  = frame->locals;

    memset(frame->locals, 0, size);

    int nlocals = match(&c->pattern, arg, local);

    if (nlocals == -1)
        return -1;

    #ifdef DEBUG
        for (int i = 0; i < proc->stack->size; i++)
            printf("\t");

        printf("%s/%s (", m->name, p->name);
        if (arg) tvalue_pp(arg);
        printf("):\n");
    #endif

    if (! c)
        return -1;

    frame->clause = c;
    frame->pc     = 0;
    frame->prev   = NULL;

    return nlocals;
}

int vm_call(VM *vm, Process *proc, Module *m, Path *p, Clause *c, TValue *arg)
{
    /* TODO: Perform pattern-match */

    m = m ? m : proc->module;
    p = p ? p : proc->path;
    c = c ? c : p->clauses[0];

    vm->path = p;

    assert(p);
    assert(m);
    assert(c);

    size_t size = sizeof(TValue) * c->nlocals;
    TValue *locals = malloc(size),
           *local  = locals;

    memset(locals, 0, size);

    int nlocals = match(&c->pattern, arg, local);

    if (nlocals == -1) {
        return -1;
    }

    for (int i = 0; i < proc->stack->size; i++)
        debug("\t");

#ifdef DEBUG
    //debug("%s/%s:\n", m->name, p->name);
    printf("%s/%s (", m->name, p->name);
    if (arg) tvalue_pp(arg);
    printf("):\n");
#endif

    if (! c)
        return -1;

    Frame *f = frame(locals, c->nlocals);

    f->module = m;
    f->path   = p;
    f->clause = c;
    f->pc     = 0;
    f->prev   = NULL;

    stack_push(proc->stack, f);

    return nlocals;
}

Process *vm_spawn(VM *vm, Module *m, Path *p)
{
    Process *proc = process(m, p);

    proc->flags |= PROC_READY;

    vm->procs[vm->nprocs++] = proc;

    return proc;
}

Process *vm_select(VM *vm)
{
    Process *proc;

    /* TODO: Don't start from 0, start from last proc */

    for (;;) {
        for (int i = 0; i < vm->nprocs; i++) {
            if ((proc = vm->procs[i])) {
                if ((proc->credits > 0) && (proc->flags & PROC_READY)) {
                    return proc;
                }
            }
        }
        /* All proc credits exhausted, reset */
        /* TODO: We don't need this step if we start from the last proc,
         * and reset as we go */
        for (int i = 0; i < vm->nprocs; i++) {
            if ((proc = vm->procs[i])) {
                proc->credits = 16;
            }
        }
    }
    assert(0);
    return NULL;
}

#define RK(x) (ISK(x) ? K[INDEXK(x)] : R[x])
TValue *vm_execute(VM *vm, Process *proc)
{
    int A, B, C;

    Module *m;
    Path   *p;
    Clause *c;

    Stack *s;
    Frame *f;

    Instruction i;

    TValue *R;
    TValue *K;

reentry:

    vm->proc = proc;

    s = proc->stack;    /* Current stack */
    f = *(s->frame);    /* Current stack-frame */
    c = f->clause;      /* Current clause */
    p = f->path;        /* Current path */
    m = f->module;      /* Current module */
    R = f->locals;      /* Registry (local vars) */
    K = c->constants;   /* Constants */

    while ((i = c->code[f->pc++])) {
        #ifdef DEBUG
        for (int i = 0; i < proc->stack->size; i++)
            debug("\t");
        printf("%3lu:\t", f->pc); op_pp(i); putchar('\n');
        #endif

        proc->credits --;

        OpCode op = OP(i);

        A = A(i);

        switch (op) {
            case OP_MOVE:
                R[A] = R[B(i)];
                break;
            case OP_LOADK:
                B = INDEXK(D(i));
                assert(A < f->nlocals);
                R[A] = K[B];
                break;
            case OP_ADD: {
                assert(RK(B(i)).t == TYPE_NUMBER);
                assert(RK(C(i)).t == TYPE_NUMBER);

                R[A].t        = TYPE_NUMBER;
                R[A].v.number = RK(B(i)).v.number +
                                RK(C(i)).v.number;
                break;
            }
            case OP_SUB: {
                assert(RK(B(i)).t == TYPE_NUMBER);
                assert(RK(C(i)).t == TYPE_NUMBER);

                R[A].t        = TYPE_NUMBER;
                R[A].v.number = RK(B(i)).v.number -
                                RK(C(i)).v.number;
                break;
            }
            case OP_JUMP:
                f->pc += J(i);
                continue;
            case OP_MATCH: {
                TValue b = RK(B(i)),
                       c = RK(C(i));

                if (match(&b, &c, &R[A]) >= 0)
                    f->pc ++;

                break;
            }
            case OP_TUPLE:
                R[A] = *tuple(B(i));
                break;
            case OP_SETTUPLE:
                B = B(i);

                assert(R[A].t == TYPE_TUPLE);
                assert(B < R[A].v.tuple->arity);

                R[A].v.tuple->members[B] = RK(C(i));
                break;
            case OP_PATH:
                R[A].v.uri.module = RK(B(i)).v.atom;
                R[A].v.uri.path   = RK(C(i)).v.atom;
                break;
            case OP_TAILCALL: {
                int      matches = -1;
                TValue   arg     = R[C(i)];

                c = p->clauses[B(i)];

                if ((matches = vm_tailcall(vm, proc, c, &arg)) < 0) /* Replace stack call-frame */
                    error(1, 0, "no matches for %s/%s", m->name, p->name);
                goto reentry;
            }
            case OP_CALL: {
                int matches = -1;

                TValue callee = RK(B(i));

                C = C(i);

                TValue arg = RK(C);

                switch (callee.t) {
                    case 0:
                    case TYPE_PATH: {
                        const char *module = callee.v.uri.module;
                        const char *path   = callee.v.uri.path;

                        /* TODO: Don't lookup if path is static */

                        if (! (m = vm_module(vm, module)))
                            error(1, 0, "module `%s` not found", module);

                        if (! (p = module_path(m, path)))
                            error(1, 0, "path `%s` not found in `%s` module", path, module);

                        c = p->clauses[0];
                        matches = vm_call(vm, proc, m, p, c, &arg); /* Create & push stack call-frame */

                        break;
                    }
                    case TYPE_CLAUSE:
                        c = callee.v.clause;
                        break;
                    default:
                        assert(0);
                }
                if (matches < 0)
                    error(1, 0, "no matches for %s/%s", m->name, p->name);

                (*s->frame)->result = A;                    /* Set return-value register */

                goto reentry;
            }
            case OP_RETURN: {
                Frame *old = stack_pop(s);

                /* We reached the top of the stack,
                 * exit loop & return last register value. */
                if (s->size == 0)
                    return &R[A];

                (*s->frame)->locals[old->result] = RK(A);

                for (int i = 0; i < proc->stack->size; i++)
                    debug("\t");
                debug("%s/%s:\n", (*s->frame)->module->name,
                                  (*s->frame)->path->name);
                free(old);
                goto reentry;
            }
            default:
                assert(0);
                break;
        }
        if (proc->credits == 0) {
            Process *np;

            if ((np = vm_select(vm))) {
                return vm_execute(vm, np);
            } else {
                proc->credits = 16;
            }
        }
    }
    return NULL;
}

TValue *vm_run(VM *vm, const char *module, const char *path)
{
    Module *m = vm_module(vm, module);

    if (! m)
        error(2, 0, "couldn't find module '%s'", module);

    Path *p = module_path(m, path);

    if (! p)
        error(2, 0, "couldn't find path '%s/%s'", module, path);

    Process *proc = vm_spawn(vm, m, p);

    vm_call(vm, proc, NULL, NULL, NULL, NULL);

    return vm_execute(vm, proc);
}

Module *vm_module(VM *vm, const char *name)
{
    uint32_t key = hash(name, strlen(name)) % 512;
    ModuleList *ms  = vm->modules[key];

    while (ms && ms->head) {
        if (! strcmp(ms->head->name, name)) {
            return ms->head;
        }
        ms = ms->tail;
    }
    return NULL;
}
