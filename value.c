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

#include "value.h"

TValue *tvalue(TYPE type, Value val)
{
    TValue *tval = malloc(sizeof(*tval));

    tval->v = val;
    tval->t = type;

    return tval;
}

