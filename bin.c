/*
 * arbre
 *
 * (c) 2011 Alexis Sellier.
 *
 * bin.c
 *
 *   binary deserialization of arbre objects
 *
 */
#include  <unistd.h>
#include  <stdlib.h>
#include  <stdint.h>
#include  <assert.h>
#include  <stdbool.h>

#include  "value.h"

TValue bin_readtuple(uint8_t **bp);

TValue bin_readnode(uint8_t **bp)
{
    TValue tv;

    TYPE t = *(*bp)++;

    switch (t) {
        case TYPE_TUPLE:
            tv = bin_readtuple(bp);
            break;
        case TYPE_ATOM:
            assert(0);
            break;
        case TYPE_ANY:
            break;
        default:
            assert(0);
    }
    return tv;
}

TValue bin_readtuple(uint8_t **bp)
{
    uint8_t arity = *(*bp)++;

    Tuple *tup = malloc(sizeof(*tup) + sizeof(TValue) * arity);
    tup->arity = arity;

    for (int i = 0; i < arity; i++) {
        tup->members[i] = bin_readnode(bp);
    }

    Value v = (Value){ .tuple = tup };
    TValue *t = tvalue(TYPE_TUPLE, v);

    return *t;
}
