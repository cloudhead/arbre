/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * symtab.c
 *
 * Symbol table implementation
 *
 */
#include  <stdlib.h>
#include  <string.h>
#include  <stdio.h>

#include  "arbre.h"
#include  "hash.h"
#include  "color.h"
#include  "util.h"
#include  "type.h"

#define  append(...)  sym_append(__VA_ARGS__)

/*
 * SymTable allocator/initializer
 */
SymTable *symtab(size_t size)
{
    size_t dsize = sizeof(SymList*) * size;
    SymTable *t  = malloc(sizeof(*t) + dsize);

    memset(t->data, 0, dsize);

    t->parent = NULL;
    t->size   = size;

    return t;
}

/*
 * SymTable de-allocator
 */
void symtab_free(SymTable *t)
{
    free(t);
}

/*
 * Get value at key `k`
 */
Sym *symtab_lookup(SymTable *t, const char *k)
{
    uint32_t key = hash(k, strlen(k)) % t->size;
    SymList *ss  = t->data[key];

    while (ss && ss->head) {
        if (! strcmp(ss->head->name, k)) {
            return ss->head;
        }
        ss = ss->tail;
    }

    if (t->parent) {
        return symtab_lookup(t->parent, k);
    }
    return NULL;
}

/*
 * Set a value `v` for key `k` in table `t`.
 *
 * If there is already a value at that index,
 * append `v` to the SymList, else create a
 * new one.
 */
void symtab_insert(SymTable *t, const char *k, Sym *s)
{
    uint32_t key = hash(k, strlen(k)) % t->size;

    if (t->data[key]) {
        sym_prepend(t->data[key], s);
    } else {
        t->data[key] = symlist(s);
    }
}

/*
 * Sym allocator/initializer (Variable)
 */
Sym *symbol(const char *k, Variable *v)
{
    Sym   *s = malloc(sizeof(*s));
           s->name  = k;
           s->type  = SYM_VAR;
           s->e.var = v;
    return s;
}

/*
 * Sym allocator/initializer (PathEntry)
 */
Sym *psymbol(const char *k, PathEntry *p)
{
    Sym   *s = malloc(sizeof(*s));
           s->name   = k;
           s->type   = SYM_PATH;
           s->e.path = p;
    return s;
}

/*
 * Sym allocator/initializer (TValue)
 */
Sym *tvsymbol(const char *k, TValue *t)
{
    Sym   *s = malloc(sizeof(*s));
           s->name   = k;
           s->type   = SYM_TVAL;
           s->e.tval = t;
    return s;
}


/*
 * SymList allocator/initializer
 */
SymList *symlist(Sym *head)
{
    SymList *list = malloc(sizeof(*list));
    list->head = head;
    list->tail = NULL;
    return list;
}

/*
 * Prepend Sym `s` to SymList `list`
 */
void sym_prepend(SymList *list, Sym *s)
{
    SymList *head;

    if (list->head) {
        head = symlist(s);
        head->tail = list;
    } else { /* empty */
        list->head = s;
    }
}

/*
 * Print a symbol table
 */
void symtab_pp(SymTable *t)
{
    Sym     *s;
    SymList *ss;

    Variable *var;

    ttyprint(COLOR_BOLD, "symbols:\n");

    for (int i = 0; i < t->size; i++) {
        if (t->data[i]) {
            ss = t->data[i];
            while (ss) {
                s = ss->head;
                putchar('\t');
                ttyprint(COLOR_BOLD, s->name);
                putchar(':');

                if (s->type == SYM_VAR) {
                    var = s->e.var;
                    putchar(' ');
                    if (var->type) {
                        if (var->type->name) {
                            puts(var->type->name);
                        } else {
                            pp_node(var->type->node);
                        }
                    } else {
                        printf(" -");
                    }
                } else {
                    printf("...");
                }
                putchar('\n');
                ss = ss->tail;
            }
        }
    }
}
