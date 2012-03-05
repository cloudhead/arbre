/*
 * arbre
 * error.c
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "error.h"

void error(int status, int errnum, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fflush(stdout);
    fprintf(stderr, "%s: ", ARBRE_CMD);
    vfprintf(stderr, fmt, ap);

    if (errnum)
        fprintf(stderr, ": %s", strerror(errnum));

    fprintf(stderr, "\n");
    va_end(ap);

    if (status)
        exit(status);
}

