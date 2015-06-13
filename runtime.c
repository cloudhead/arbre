/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * runtime.c
 *
 *   Runtime support for stack, modules, paths and processes
 *
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "arbre.h"
#include "op.h"
#include "runtime.h"
#include "assert.h"

Stack *stack(void)
{
    Stack *s = malloc(sizeof(*s));
           s->size = 0;
           s->capacity = sizeof(Frame) + sizeof(TValue);
           s->base = malloc(s->capacity);
           s->frame = NULL;
           s->depth = 0;
    return s;
}

void stack_correct(Stack *s, Frame *old)
{
    for (Frame *f = s->frame; f != NULL; f = f->prev) {
        if (f->prev) {
            ptrdiff_t diff = (ptrdiff_t)f->prev - (ptrdiff_t)old;
            f->prev = (Frame *)(diff + (ptrdiff_t)s->base);
        }
    }
}

/*
 * Push the given frame on the stack
 */
void stack_push(Stack *s, Clause *c)
{
    int nsize = s->size + sizeof(Frame)
                        + sizeof(TValue) * c->nlocals;

    Frame *oldbase = NULL,
          *prev = s->frame,
          *f;

    if (s->capacity < nsize) {
        oldbase = s->base;

        s->capacity = (s->capacity) * 2;

        if (s->capacity < nsize)
            s->capacity = nsize;

        s->base = realloc(s->base, s->capacity);
        memset((Frame *)((ptrdiff_t)s->base + s->size + sizeof(Frame)), 0,
               s->capacity - s->size - sizeof(Frame));
    }

    s->frame  = (Frame *)((ptrdiff_t)s->base + s->size);
    s->size   = nsize;

    f         = s->frame;
    f->prev   = prev;
    f->pc     = c->code;
    f->clause = c;

    if (s->depth > 0 && oldbase)
        stack_correct(s, oldbase);

    s->depth  ++;

    assert(s->size <= s->capacity);
}

/*
 * Pop the current frame from the stack
 */
Frame *stack_pop(Stack *s)
{
    Frame *f = s->frame, *old;

    s->size -= sizeof(*f) + sizeof(TValue) * f->clause->nlocals;

    assert(s->size >= 0);

    s->depth --;
    s->frame = f->prev;

    if (s->capacity - s->size * 2 > STACK_MAXDIFF) {
        old = s->base;
        s->capacity = s->size * 2;
        s->base = realloc(s->base, s->capacity);
        if (s->base != old)
            stack_correct(s, old);
    }
    return f;
}

/*
 * Module allocator
 */
Module *module(const char *name, unsigned pathc)
{
    Module *m = malloc(sizeof(*m));
            m->name = name;
            m->paths = pathc ? malloc(sizeof(Path *) * pathc) : NULL;
            m->pathc = pathc;
    return  m;
}

/*
 * SymList allocator/initializer
 */
ModuleList *modulelist(Module *head)
{
    ModuleList *list = malloc(sizeof(*list));
    list->head = head;
    list->tail = NULL;
    return list;
}

/*
 * Prepend Module `m` to ModuleList `list`
 */
void module_prepend(ModuleList *list, Module *m)
{
    ModuleList *head;

    if (list->head) {
        head = modulelist(m);
        head->tail = list;
    } else { /* empty */
        list->head = m;
    }
}

Path *module_path(Module *m, const char *path)
{
    /* TODO: Optimize with hash table */
    for (int i = 0; i < m->pathc; i++) {
        if (! strcmp(m->paths[i]->name, path)) {
            return m->paths[i];
        }
    }
    return NULL;
}

Path *path(const char *name, int nclauses)
{
    Path  *p = malloc(sizeof(*p));
           p->name = name;
           p->nclauses = nclauses;
           p->clauses = calloc(nclauses, sizeof(Clause));
    return p;
}

Clause *clause(TValue pattern, int nlocals, int clen)
{
    Clause *c = malloc(sizeof(*c));
            c->pattern = pattern;
            c->nlocals = nlocals;
            c->constants = malloc(sizeof(TValue) * clen);
            c->constantsn = clen;
            c->pc = -1;
    return  c;
}

TValue *select_(int nclauses)
{
    assert(nclauses <= 255);

    Select *s = malloc(sizeof(*s) + sizeof(TValue *) * nclauses);
            s->nclauses = nclauses;

    return tvalue(TYPE_SELECT, (Value){ .select = s });
}

Process *process(Module *m, Path *path)
{
    Process *p = malloc(sizeof(*p));
    p->stack = stack();
    p->module = m;
    p->path = path;
    p->credits = 0;
    p->flags = 0;
    return p;
}

