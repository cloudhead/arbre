/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * util.c
 *
 *   collection of utility functions
 *
 * TODO: Document functions
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "color.h"

#define ESCAPE_BUFF 1024

int isascii(int c);

/*
 * Escape a string, revealing all non-ascii or
 * non-printable characters.
 */
char *escape(char *str)
{
    if (str == NULL) return "";
    static char esc[ESCAPE_BUFF];
    escape_n(str, esc, strlen(str));
    return esc;
}

int escape_n(char *str, char *esc, int len)
{
    int  e = 0;

    for (int i = 0; i < ESCAPE_BUFF && i < len; i ++) {
        switch (str[i]) {
            case '\n':
                esc[e++] = '\\';
                esc[e++] = 'n';
                break;
            case '\t':
                esc[e++] = '\\';
                esc[e++] = 't';
                break;
            default:
                if (isascii(str[i])) {
                    esc[e++] = str[i];
                } else {
                    sprintf(esc + e, "#%3d", (unsigned char)str[i]), e += 4;
                }
        }
    }
    esc[e] = '\0';

    return e;
}

char *escape_r(char *str, char **saveptr)
{
    int  len = strlen(str), newlen;
    char *esc = *saveptr;

    esc    = malloc(len * 4);
    newlen = escape_n(str, esc, len);
    esc    = realloc(esc, newlen);

    return esc;
}

/*
 * Print `str` with escape code `esc`, if
 * STDOUT is a tty.
 */
void ttyprint(char *esc, const char *str)
{
    int tty = isatty(STDOUT_FILENO);

    tty && printf("%s", esc);
           printf("%s", str);
    tty && printf("%s", COLOR_RESET);
}
