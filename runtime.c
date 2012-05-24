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

#include "arbre.h"
#include "op.h"
#include "runtime.h"

void frame_pp(Frame *f);

Frame *frame(TValue *locals, int nlocals)
{
    Frame *f = malloc(sizeof(*f));
           f->locals  =  locals;
           f->nlocals = nlocals;
    return f;
}

void frame_pp(Frame *f)
{
    for (int i = 0; i < f->nlocals; i++) {
        printf("R[%d]: ", i);
        tvalue_pp(f->locals + i);
        putchar('\n');
    }
}

Stack *stack(void)
{
    Stack *s = malloc(sizeof(*s));
           s->size = 0;
           s->frames = NULL;
           s->frame = NULL;
    return s;
}

void stack_pp(Stack *s)
{
    for (int i = s->size - 1; i >= 0; i--) {
        frame_pp(s->frames[i]);
        putchar('\n');
    }
}

/*
 * Push the given frame on the stack
 */
void stack_push(Stack *s, Frame *f)
{
    s->size ++;
    s->frames = realloc(s->frames, s->size * sizeof(Frame*));
    s->frame  = s->frames + s->size - 1;
  *(s->frame) = f;
}

/*
 * Pop the current frame from the stack
 */
Frame *stack_pop(Stack *s)
{
    Frame *f = *(s->frame);

    s->size --;
    s->frames = realloc(s->frames, s->size * sizeof(Frame*));
    s->frame  = s->frames + s->size - 1;

    return f;
}

/*
 * Module allocator
 */
Module *module(const char *name, Path *paths[], unsigned pathc)
{
    Module *m = malloc(sizeof(*m));
            m->name = name;
            m->paths = paths;
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

Process *process(Module *m, Path *path)
{
    Process *p = malloc(sizeof(*p));
    p->stack = stack();
    p->module = m;
    p->path = path;
    p->credits = 0;
    return p;
}

