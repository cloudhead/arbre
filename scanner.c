/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * scanner.c
 *
 *   raw source file scanner.
 *
 * + scanner
 * - scanner_free
 *
 * -> scan
 *
 * usage:
 *
 *   Source  *src = source("fnord.arb");
 *   Scanner *s   = scanner(src);
 *   Token   *t;
 *
 *   while ((t = scan(s)).tok != T_EOF) {
 *       ... do something with token ...
 *   }
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "arbre.h"
#include "scanner.h"

/* I'm not sure why we have to declare these,
 * but clang complains otherwise. */
char *strndup(char const *, unsigned long);
int   isascii(int c);

static void next(Scanner *);
static void pushlvl(Scanner *, int);
static int  poplvl(Scanner *);

/*
 * Scanner allocator
 */
Scanner *scanner(Source *source)
{
    Scanner *s = malloc(sizeof(*s));

    s->src    = source->data;
    s->source = source;
    s->len    = source->size;
    s->pos    = s->rpos = -1;
    s->ch     = T_ILLEGAL;
    s->lvl    = 0;
    s->dedent = 0;
    s->lf     = 0;
    s->line   = 0;
    s->linepos= 0;

    s->lvls.size  = 0;
    s->lvls.stack = NULL;
    s->lvls.sp    = NULL;

    source_addline(s->source, 0);
    pushlvl(s, 0);
    next(s);

    return s;
}

/*
 * Scanner de-allocator
 */
void scanner_free(Scanner *s)
{
    free(s->lvls.stack);
    free(s);
}

/*
 * Read the next character into s->ch.
 * Set s->ch to -1 if we've reached the EOF.
 */
static void next(Scanner *s)
{
    s->rpos ++;

    if (s->rpos <= s->len) {
        char c = s->src[s->rpos];

        if (c == '\0') {
            /* TODO: Handle error */
        } else if (c >= 0x80) {
            /* TODO: Handle unicode */
        } else {
            s->ch = c;
        }
    } else {
        s->ch = -1; /* EOF */
    }
}

/*
 * Consume whitespace. '\n' is treated differently,
 * due to the indentation-sensitive grammar.
 */
static void whitespace(Scanner *s)
{
    while (s->ch != '\n' && isspace(s->ch)) {
        next(s);
    }
}

/*
 * Consume an identifier, such as: Fnord
 */
static void scan_ident(Scanner *s)
{
    while (isalnum(s->ch)) {
        next(s);
    }
}

/*
 * Consume an atom, such as: fnord
 */
static void scan_atom(Scanner *s)
{
    while (islower(s->ch) || s->ch == '_' || s->ch == '.') {
        next(s);
    }
}

/*
 * Consume a number, such as: 42
 */
static void scan_number(Scanner *s)
{
    while (isdigit(s->ch)) {
        next(s);
    }
}

/*
 * Consume a string, such as: "fnord, fnord, fnord"
 */
static void scan_string(Scanner *s)
{
    while (s->ch != '"') {
        if (s->ch == '\n' || s->ch < 0) {
            /* TODO: Handle error */
        }
        next(s);
    }
    next(s); /* Consume closing '"' */
}

/*
 * Consume a comment, such as: -- fnord comment
 */
static void scan_comment(Scanner *s)
{
    while (s->ch != '\n') {
        next(s);
    }
}

/*
 * Consume a character, such as: 'z'
 */
static void scan_char(Scanner *s)
{
    if (s->ch == '\'') {
        next(s);
    } else if (isascii(s->ch)) {
        next(s);
        if (s->ch == '\'') {
            next(s);
        } else {
            /* TODO: Handle error */
        }
    } else {
        /* TODO: Handle error */
    }
}

/*
 * Consume indentation. Return `true` if there's an indent.
 */
static int scan_indent(Scanner *s)
{
    size_t pos, indent;

    /* calculate current indentation */
    pos = s->rpos, whitespace(s), indent = s->rpos - pos;

    /* indent, dedent or don't change indentation */
    if (indent > s->lvl && s->ch != '\n') {
        pushlvl(s, (s->lvl = indent));
        return true;
    } else {
        /* Only dedent if the current indentation
         * is smaller than the previous indentation. */
        while ((s->lvl > 0) && (indent <= *(s->lvls.sp - 1))) {
            poplvl(s);
            s->lvl = *(s->lvls.sp);
            s->dedent ++;
        }
    }
    return false;
}


/*
 * Push the current indentation level to the stack
 */
static void pushlvl(Scanner *s, int lvl)
{
    s->lvls.size ++;
    s->lvls.stack = realloc(s->lvls.stack, s->lvls.size * sizeof(lvl));
    s->lvls.sp    = s->lvls.stack + s->lvls.size - 1;
  *(s->lvls.sp)   = lvl;
}

/*
 * Pop the current indentation from the stack
 */
static int poplvl(Scanner *s)
{
    int lvl = *(s->lvls.sp);

    s->lvls.size --;
    s->lvls.stack = realloc(s->lvls.stack, s->lvls.size * sizeof(lvl));
    s->lvls.sp    = s->lvls.stack + s->lvls.size - 1;

    return lvl;
}

static void newline(Scanner *s)
{
    s->lf      = false;
    s->line    ++;
    s->linepos = s->rpos;

    source_addline(s->source, s->linepos);

    s->source->col = 0;
    s->source->line ++;
}

/*
 * Scan the source for the next token, and return it.
 * We treat '\n' specially, as it could precede a change
 * in indentation.
 * Because it is possible for the level of indentation to
 * fall from N to 0 after a single T_LF token, and we want
 * to return a T_DEDENT for *each* of these levels,
 * we have to keep track of dedents which haven't been returned
 * by the scanner yet, and return them on next call to `scan` in [1].
 * This is done via the `dedent` property of the scanner.
 */
Token *scan(Scanner *s)
{
    TOKEN   tok = T_ILLEGAL;
    size_t  pos = s->pos = s->rpos;
    char    c;

    /* if the last token a T_LF, scan for indentation changes */
    if (s->lf) {
        newline(s);
        if (scan_indent(s)) {
            return token(T_INDENT, s->source, pos, NULL);
        }
    }

    /* 1. We have pending dedents, return a T_DEDENT, and decrement
     *    the dedent counter. */
    if (s->dedent) {
        s->dedent --;
        return token(T_DEDENT, s->source, pos, NULL);
    }
    whitespace(s);

    c   = s->ch;
    pos = s->rpos;

    if (c == -1) {
        tok = T_EOF;
    } else if (isupper(c)) {
        scan_ident(s);
        tok = T_IDENT;
    } else if (islower(c)) {
        scan_atom(s);
        tok = T_ATOM;
    } else if (isdigit(c)) {
        scan_number(s);
        tok = T_INT;
    } else {
        next(s); /* We advance here, but keep matching on `c` */

        switch (c) {
            case  '@' :  tok = T_AT;                        break;
            case  '_' :  tok = T_UNDER;                     break;
            case  '?' :  tok = T_QUESTION;                  break;
            case  '&' :  tok = T_AND;                       break;
            case  '|' :  tok = T_PIPE;                      break;
            case  '(' :  tok = T_LPAREN;                    break;
            case  ')' :  tok = T_RPAREN;                    break;
            case  '[' :  tok = T_LBRACK;                    break;
            case  ']' :  tok = T_RBRACK;                    break;
            case  '{' :  tok = T_LBRACE;                    break;
            case  '}' :  tok = T_RBRACE;                    break;
            case  ';' :  tok = T_SEMICOLON;                 break;
            case  ',' :  tok = T_COMMA;                     break;
            case  '"' :  tok = T_STRING;   scan_string(s);  break;
            case  '`' :  tok = T_ATOM;     scan_atom(s);    break;
            case '\'' :  tok = T_CHAR;     scan_char(s);    break;
            case '\n' :  tok = T_LF;       s->lf = true;    break;
            case  ' ' :  tok = T_SPACE;                     break;
            case  '+' :  tok = T_PLUS;                      break;
            case  '/' :  tok = T_SLASH;                     break;
            case '\\' :  tok = T_BSLASH;                    break;
            case  '.' :
                if (s->ch == '.') {
                    tok = T_ELLIPSIS;
                    next(s);
                } else {
                    tok = T_PERIOD;
                }
                break;
            case  ':' :
                switch (s->ch) {
                    case '=':  tok = T_DEFINE;  next(s);  break;
                    default :  tok = T_COLON;
                }
                break;
            case '<':
                switch (s->ch) {
                    case  '-':  tok = T_LARROW;  next(s);  break;
                    case  '=':  tok = T_LDARROW; next(s);  break;
                    default  :  tok = T_LT;
                }
                break;
            case '>':
                if (s->ch == '>') {
                    next(s);
                    tok = T_RDARROW;
                } else {
                    tok = T_GT;
                }
                break;
            case '-':
                if (isdigit(s->ch)) {
                    tok = T_INT;
                    next(s);
                    scan_number(s);
                } else {
                    switch (s->ch) {
                        case  '>':  tok = T_RARROW;  next(s);         break;
                        case  '-':  tok = T_COMMENT; scan_comment(s); break;
                        default  :  tok = T_MINUS;
                    }
                }
                break;
            case '=':
                switch (s->ch) {
                    case  '>':  tok = T_REQARROW;  next(s);  break;
                    default  :  tok = T_EQ;
                }
                break;
        }
    }
    s->source->col = pos - s->linepos;
    s->pos         = pos;

    return token(tok, s->source, pos, strndup(s->src + pos, s->rpos - pos));
}

