/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * token.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "source.h"
#include "tokens.h"
#include "token.h"
#include "util.h"

/*
 * Print the string representation of a token
 * `t` into `str`, and return it.
 */
char *tokentos(Token *t, char *str, bool showsrc)
{
    const char *token = TOKEN_STRINGS[t->tok];

    if (t->src && showsrc) {
        sprintf(str, "<%s %s>", token, escape(t->src));
    } else {
        sprintf(str, "<%s>", token);
    }
    return str;
}

/*
 * Allocate and return a new Token.
 */
Token *token(TOKEN tok, struct source *source, size_t pos, char *src)
{
    Token *t   =  malloc(sizeof(*t));
    t->tok     =  tok;
    t->source  =  source;
    t->pos     =  pos;
    t->src     =  src;
    return t;
}

