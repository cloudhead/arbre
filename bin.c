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

struct tvalue bin_readtuple(uint8_t **bp);

struct tvalue bin_readnode(uint8_t **bp)
{
    struct tvalue tv;

    TYPE t = *(*bp)++;

    switch (t) {
        case TYPE_TUPLE:
            tv = bin_readtuple(bp);
            break;
        case TYPE_NUMBER:
            tv = (struct tvalue){
                .t = TYPE_NUMBER,
                .v = (Value){
                    .number = *(int *)*bp
                }
            };
            *bp += sizeof(int);
            break;
        case TYPE_ATOM: {
            uint8_t len = *(*bp)++;

            tv = (struct tvalue){
                .t = TYPE_ATOM,
                .v = (Value){
                     // Alternatively `strndup(*bp, len)`
                    .atom = (const char *)*bp
                }
            };
            *bp += len;
            break;
        }
        case TYPE_ANY:
            tv = (struct tvalue){ .t = TYPE_ANY };
            break;
        default:
            assert(0);
    }
    return tv;
}

struct tvalue bin_readtuple(uint8_t **bp)
{
    uint8_t arity = *(*bp)++;

    Tuple *tup = malloc(sizeof(*tup) + sizeof(struct tvalue) * arity);
    tup->arity = arity;

    for (int i = 0; i < arity; i++) {
        tup->members[i] = bin_readnode(bp);
    }

    Value v = (Value){ .tuple = tup };
    struct tvalue *t = tvalue(TYPE_TUPLE, v);

    return *t;
}
