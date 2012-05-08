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

#define DEBUG

#ifdef DEBUG
#define debug(...) (fprintf(stderr, __VA_ARGS__))
#else
#define debug(...) ;
#endif

Module *vm_module  (VM *vm, const char *name);
TValue *vm_execute (VM *vm, Process *proc);

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

/* TODO: Paths should be per-module */
uint8_t *vm_readpath(VM *vm, uint8_t *b)
{
    char *name;

    /* Path attributes */
    uint8_t attrs   = *b ++;
    uint8_t namelen = *b ++;

    /* TODO: check attributes instead */
    /* TODO: clean this up.. maybe with strdup */
    if (namelen > 0) {
        name = malloc(namelen + 1);
        memcpy(name, b, namelen);
        name[namelen] = '\0';
        b += namelen;
    }

    /* Size of constants table */
    uint8_t ksize = *b ++;

    Path *p = path(name, ksize);
    Value v;

    debug("%s (%d):\n\n", name, attrs);

    for (int i = 0; i < ksize; i++) {
        TYPE t = *b ++;

        debug("\tk%d = ", i);

        switch (t) {
            case TYPE_BIN:
            case TYPE_TUPLE:
            case TYPE_ATOM:
                v.atom = (char *)b;
                b += strlen(v.atom) + 1;
                debug("%s", v.atom);
                break;
            case TYPE_STRING:
                assert(0);
                break;
            case TYPE_NUMBER:
                v.number = (int)*b;
                b += sizeof(int);
                debug("%d", v.number);
                break;
            default:
                assert(0);
                break;
        }
        debug("\n");

        p->constants[i].t = t;
        p->constants[i].v = v;

    }
    debug("\n");

    p->codelen = *((unsigned long*)b);
    b += sizeof(p->codelen);

    p->code = (Instruction *)b;

    /* Skip all code */
    b += p->codelen * sizeof(Instruction);

    vm->paths[vm->pathc++] = p;

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

void vm_call(VM *vm, Process *proc, Module *m, Path *p, TValue *arg)
{
    Frame *f = frame(255);

    m = m ? m : proc->module;
    p = p ? p : proc->path;

    // value passed to the function is `arg`
    // `arg` needs to be destructured/pattern-matched against function pattern
    // this is the set of locals that end up in the function.
    // this is known in advance by generator, but doesn't have to be..
    // pattern matcher can return list of locals + size.
    //
    // arg = (42, foo)
    // pat = (X, Y)
    //
    // iterate over elements in parrallel, bind variables, error if there's a
    // mismatch

    //f->locals = args;
    f->module = m;
    f->path   = p;
    f->pc     = 0;
    f->prev   = NULL;

    stack_push(proc->stack, f);

    debug("%s/%s:\n", m->name, p->name);
}

Process *vm_spawn(VM *vm, Module *m, Path *p)
{
    Process *proc = process(m, p);

    vm->procs[vm->nprocs++] = proc;

    return proc;
}

Process *vm_select(VM *vm)
{
    Process *proc;

    /* TODO: Don't start from 0, start from last proc */

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
    return vm_select(vm);
}

#define RK(x) (ISK(x) ? K[INDEXK(x)] : R[x])
TValue *vm_execute(VM *vm, Process *proc)
{
    vm->proc = proc;

    int A, B, C;

    Module *m;
    Path   *p;

    Stack *s = proc->stack;
    Frame *f;

    Instruction i;

    TValue *R;
    TValue *K;

reentry:

    f = *(s->frame);    /* Current stack-frame */
    p = f->path;        /* Current path */
    m = f->module;      /* Current module */
    R = f->locals;      /* Registry (local vars) */
    K = p->constants;   /* Constants */

    while ((i = p->code[f->pc++])) {
        #ifdef DEBUG
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
                R[A] = K[B];
                break;
            case OP_ADD:
                // TODO: Check operands
                R[A].v.number = RK(B(i)).v.number +
                                RK(C(i)).v.number;
                break;
            case OP_JUMP:
                f->pc += J(i);
                break;
            case OP_MATCH:
                // TODO: Implement
                assert(0);
                break;
            case OP_TUPLE:
                // TODO: Implement
                assert(0);
                break;
            case OP_SETTUPLE:
                // TODO: Implement
                assert(0);
                break;
            case OP_PATH:
                R[A].v.uri.module = RK(B(i)).v.atom;
                R[A].v.uri.path   = RK(C(i)).v.atom;
                break;
            case OP_TAILCALL:
                (*(s->frame))->pc = 0;
                goto reentry;
            case OP_CALL: {
                C = C(i); /* TODO: Pass argument to function */

                const char *module = RK(B(i)).v.uri.module;
                const char *path   = RK(B(i)).v.uri.path;

                /* TODO: Don't lookup if path is static */

                if (! (m = vm_module(vm, module)))
                    error(1, 0, "module `%s` not found", module);

                if (! (p = module_path(m, path)))
                    error(1, 0, "path `%s` not found in `%s` module", path, module);

                TValue arg = RK(C);

                vm_call(vm, proc, m, p, &arg); /* Create & push stack call-frame */
                (*s->frame)->result = A;       /* Set return-value register */

                goto reentry;
            }
            case OP_RETURN: {
                Frame *old = stack_pop(s);

                /* We reached the top of the stack,
                 * exit loop. */
                if (s->size == 0)
                    return &R[A];

                (*s->frame)->locals[old->result] = RK(A);

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

    vm_call(vm, proc, NULL, NULL, NULL);

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
