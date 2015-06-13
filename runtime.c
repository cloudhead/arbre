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

struct stack *stack(void)
{
    struct stack *s = malloc(sizeof(*s));

    s->size = 0;
    s->capacity = sizeof(struct frame) + sizeof(struct tvalue);
    s->base = malloc(s->capacity);
    s->frame = NULL;
    s->depth = 0;

    return s;
}

void stack_correct(struct stack *s, struct frame *old)
{
    for (struct frame *f = s->frame; f != NULL; f = f->prev) {
        if (f->prev) {
            ptrdiff_t diff = (ptrdiff_t)f->prev - (ptrdiff_t)old;
            f->prev = (struct frame *)(diff + (ptrdiff_t)s->base);
        }
    }
}

/*
 * Push the given frame on the stack
 */
void stack_push(struct stack *s, struct clause *c)
{
    int nsize = s->size + sizeof(struct frame)
                        + sizeof(struct tvalue) * c->nlocals;

    struct frame *oldbase = NULL,
                 *prev = s->frame,
                 *f;

    if (s->capacity < nsize) {
        oldbase = s->base;

        s->capacity = (s->capacity) * 2;

        if (s->capacity < nsize)
            s->capacity = nsize;

        s->base = realloc(s->base, s->capacity);
        memset((struct frame *)((ptrdiff_t)s->base + s->size + sizeof(struct frame)), 0,
               s->capacity - s->size - sizeof(struct frame));
    }

    s->frame  = (struct frame *)((ptrdiff_t)s->base + s->size);
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
struct frame *stack_pop(struct stack *s)
{
    struct frame *f = s->frame, *old;

    s->size -= sizeof(*f) + sizeof(struct tvalue) * f->clause->nlocals;

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
struct module *module(const char *name, unsigned pathc)
{
    struct module *m = malloc(sizeof(*m));

    m->name = name;
    m->paths = pathc ? malloc(sizeof(struct path *) * pathc) : NULL;
    m->pathc = pathc;

    return m;
}

/*
 * SymList allocator/initializer
 */
struct modulelist *modulelist(struct module *head)
{
    struct modulelist *list = malloc(sizeof(*list));
    list->head = head;
    list->tail = NULL;
    return list;
}

/*
 * Prepend module `m` to `list`
 */
void module_prepend(struct modulelist *list, struct module *m)
{
    struct modulelist *head;

    if (list->head) {
        head = modulelist(m);
        head->tail = list;
    } else { /* empty */
        list->head = m;
    }
}

struct path *module_path(struct module *m, const char *path)
{
    /* TODO: Optimize with hash table */
    for (int i = 0; i < m->pathc; i++) {
        if (! strcmp(m->paths[i]->name, path)) {
            return m->paths[i];
        }
    }
    return NULL;
}

struct path *path(const char *name, int nclauses)
{
    struct path *p = malloc(sizeof(*p));

    p->name = name;
    p->nclauses = nclauses;
    p->clauses = calloc(nclauses, sizeof(struct clause));

    return p;
}

struct clause *clause(struct tvalue pattern, int nlocals, int clen)
{
    struct clause *c = malloc(sizeof(*c));

    c->pattern = pattern;
    c->nlocals = nlocals;
    c->constants = malloc(sizeof(struct tvalue) * clen);
    c->constantsn = clen;
    c->pc = -1;

    return c;
}

struct tvalue *select_(int nclauses)
{
    assert(nclauses <= 255);

    Select *s = malloc(sizeof(*s) + sizeof(struct tvalue *) * nclauses);
            s->nclauses = nclauses;

    return tvalue(TYPE_SELECT, (Value){ .select = s });
}

Process *process(struct module *m, struct path *path)
{
    Process *p = malloc(sizeof(*p));
    p->stack = stack();
    p->module = m;
    p->path = path;
    p->credits = 0;
    p->flags = 0;
    return p;
}

