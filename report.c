/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * report.c
 *
 *   message reporting facilities
 *
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <unistd.h>
#include  <string.h>

char     *strdup(const char *);

#include  "arbre.h"
#include  "scanner.h"
#include  "parser.h"
#include  "color.h"
#include  "util.h"
#include  "report.h"

const char *REPORT_COLORS[] = { COLOR_RED,     COLOR_MAGENTA,   COLOR_GREY };
const char *REPORT_LABELS[] = { "error",       "warning",       "note" };

static void highlight(struct source *, struct node *);
static void vpheaderf(enum REPORT_TYPE, struct source *, const char *, va_list);
static void psrcline(struct source *, int);
static void fill(char, int);
static void carret(int);
static void tty(const char *);

/*
 * Print formatted to stderr
 */
int vreportf(const char *fmt, va_list ap)
{
    return vfprintf(stderr, fmt, ap);
}
int reportf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vreportf(fmt, ap);
    va_end(ap);
    return 0;
}

/*
 * Report based on a Token object
 */
void vtreportf(enum REPORT_TYPE type, Token *t, const char *fmt, va_list ap)
{
    source_seek(t->source, t->pos);
    vpheaderf(type, t->source, fmt, ap);
    psrcline(t->source, t->source->line);
    carret(t->source->col);
}

/*
 * Report based on a node object
 */
void vnreportf(enum REPORT_TYPE type, struct node *n, const char *fmt, va_list ap)
{
    struct source *s = n->source;

    source_seek(s, n->pos);
    vpheaderf(type, s, fmt, ap);

    /* Print source extract */
    psrcline(s, s->line);

    /* Highlight source */
    switch (n->op) {
        case OMATCH: case ODECL:
            highlight(s, n);
            break;
        default:
            carret(s->col);
            break;
    }
    source_rewind(n->source);
}
void nreportf(enum REPORT_TYPE type, struct node *n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vnreportf(type, n, fmt, ap);
    va_end(ap);
}

/*
 * Report based on a Parser object
 */
void vpreportf(enum REPORT_TYPE type, Parser *p, const char *fmt, va_list ap)
{
    vtreportf(type, p->token, fmt, ap);
}
void preportf(enum REPORT_TYPE type, Parser *p, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vpreportf(type, p, fmt, ap);
    va_end(ap);
}

/*
 * Print the a message header
 *
 *     <filename>:<line>:<column>: <message-type>: <message>
 */
static void vpheaderf(enum REPORT_TYPE type, struct source *s, const char *fmt, va_list ap)
{
    tty(COLOR_BOLD);

    /* Print source location */
    reportf("%s:%d:%d  ", s->path, s->line + 1, s->col + 1);

    /* Print message label */
    tty(REPORT_COLORS[type]);
    reportf("%s: ", REPORT_LABELS[type]);

    tty(COLOR_RESET);

    /* Print message */
    tty(COLOR_BOLD);
    vreportf(fmt, ap);
    tty(COLOR_RESET);
}

/*
 * Print a line of code from struct source `s`
 */
static void psrcline(struct source *s, int line)
{
    char *str = strdup(s->lineps[line]);
    reportf("\n%s\n", strtok(str, "\n"));
    free(str);
}

/*
 * Fill stderr with characters
 */
static void fill(char c, int count)
{
    for (int i = 0; i < count; i ++) {
        fputc(c, stderr);
    }
}

/*
 * Print to a tty
 */
static void tty(const char *s)
{
    if (isatty(STDERR_FILENO)) {
        fprintf(stderr, "%s", s);
    }
}

/*
 * Print the '^' carret
 */
static void carret(int col)
{
    tty(COLOR_BOLD_GREEN);
    fill(' ', col);
    fputc('^', stderr);
    fputc('\n', stderr);
    tty(COLOR_RESET);
}

/*
 * Highlight a binary node's source
 */
static void highlight(struct source *s, struct node *n)
{
    struct node *lval = n->o.match.lval,
         *rval = n->o.match.rval;

    source_seek(s, lval->pos);

    tty(COLOR_BOLD_GREEN);
    {
        fill(' ', s->col);
        fill('~', strlen(lval->src));
        fill(' ', n->pos - (lval->pos + strlen(lval->src)));
        fill('^', 1);
        fill(' ', rval->pos - n->pos - 1);
        fill('~', strlen(rval->src));
        fill('\n', 1);
    }
    tty(COLOR_RESET);
}

