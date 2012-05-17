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

#include "value.h"

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
        printf("(null)");
        return;
    }

    TYPE t = tval->t;
    Value v = tval->v;

    switch (t) {
        case TYPE_BIN:
        case TYPE_TUPLE:
        case TYPE_ATOM:
            printf("%s", v.atom);
            break;
        case TYPE_STRING:
            break;
        case TYPE_NUMBER:
            printf("%d", v.number);
            break;
        default:
            printf("<unknown>");
            break;
    }
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

