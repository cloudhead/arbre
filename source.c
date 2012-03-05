/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * source.c
 *
 *   abstraction over a raw source file
 *
 * TODO: Document functions
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>

#include "source.h"

static char *readfile(const char *, size_t *);

Source *source(const char *path)
{
    Source *src = malloc(sizeof(*src));

    src->path        = path;
    src->data        = readfile(path, &src->size);
    src->lines       = 0;
    src->lineps      = malloc(1024 * sizeof(char *));
    src->lineps_size = 1024;

    source_rewind(src);

    return src;
}

void source_free(Source *src)
{
    free(src->data);
    free(src->lineps);
    free(src);
}

void source_seek(Source *s, size_t pos)
{
    for (s->line = 0; s->line + 1 < s->lines; s->line++) {
        if (s->lineps[s->line + 1] > s->data + pos) break;
    }
    for (s->col = 0; s->lineps[s->line][s->col] != '\n'; s->col++) {
        if (s->lineps[s->line] + s->col == s->data + pos) break;
    }
}

void source_rewind(Source *s)
{
    s->line = 0;
    s->col  = 0;
}

void source_addline(Source *s, size_t lpos)
{
    s->lineps[s->lines ++] = s->data + lpos;

    if (s->lines >= s->lineps_size) {
        s->lineps = realloc(s->lineps, s->lineps_size * 2);
    }
}

static char *readfile(const char *path, size_t *length)
{
    FILE   *fp;
    char   *buffer;
    size_t  size;

    if (! (fp = fopen(path, "r")))
        error(1, errno, "error reading %s\n", path);

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buffer = malloc(size + 1);

    fread(buffer, size, 1, fp);
    fclose(fp);

    *length = size;

    buffer[size] = '\0';

    return buffer;
}
