/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * value.c
 *
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "value.h"

void tuple_pp (Value v);

const char *TYPE_STRINGS[] = {
    [TYPE_INVALID] = "INVALID",
    [TYPE_NONE] = "_",
    [TYPE_ANY] = "any",
    [TYPE_VAR] = "var",
    [TYPE_ATOM] = "atom",
    [TYPE_BIN] = "bin",
    [TYPE_TUPLE] = "tuple",
    [TYPE_STRING] = "string",
    [TYPE_NUMBER] = "number",
    [TYPE_LIST] = "list",
    [TYPE_PATH] = "path"
};

TValue *tvalue(TYPE type, Value val)
{
    TValue *tval = malloc(sizeof(*tval));

    tval->v = val;
    tval->t = type;

    return tval;
}

void tvalue_pp(TValue *tval)
{
    if (tval == NULL) {
        printf("()");
        return;
    }

    TYPE t = tval->t;
    Value v = tval->v;

    switch (t & TYPE_MASK) {
        case TYPE_TUPLE:
            tuple_pp(v);
            break;
        case TYPE_BIN:
            printf("<bin>");
            break;
        case TYPE_ATOM:
            printf("%s", v.atom);
            break;
        case TYPE_STRING:
            printf("<string>");
            break;
        case TYPE_NUMBER:
            printf("%d", v.number);
            break;
        case TYPE_LIST: {
            List *l = v.list;
            printf("[");
            while (l->head) {
                tvalue_pp(l->head);
                if ((l = l->tail)->head)
                    printf(", ");
            }
            printf("]");
            break;
        }
        default:
            printf(t & Q_RANGE ? "<%s..>" : "<%s>", TYPE_STRINGS[t]);
            break;
    }
}

void tuple_pp(Value v)
{
    putchar('(');
    for (int i = 0; i < v.tuple->arity; i++) {
        tvalue_pp(&v.tuple->members[i]);

        if (i < v.tuple->arity - 1)
            printf(", ");
    }
    putchar(')');
}

void tvalues_pp(TValue *tval, int size)
{
    putchar('[');
    for (int i = 0; i < size; i++) {
        tvalue_pp(tval + i);
        if (i < size - 1)
            putchar(',');
    }
    putchar(']');
}

TValue *tuple(int arity)
{
    assert(arity <= 255);

    Tuple *t = malloc(sizeof(*t) + sizeof(TValue) * arity);
           t->arity = arity;

    return tvalue(TYPE_TUPLE, (Value){ .tuple = t });
}

TValue *list(TValue *head)
{
    List *l = malloc(sizeof(*l));
          l->head = head;
          l->tail = NULL;

    return tvalue(TYPE_LIST, (Value){ .list = l });
}

List *list_cons(List *list, TValue *head)
{
    List *l = malloc(sizeof(*l));
          l->head = head;
          l->tail = list;

    return l;
}

TValue *atom(const char *name)
{
    assert(name);

    return tvalue(TYPE_ATOM, (Value){ .atom = name });
}

TValue *number(const char *src)
{
    int number = atoi(src);

    Value v = (Value){ .number = number };

    return tvalue(TYPE_NUMBER, v);
}
