/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * scanner.h
 *
 */
typedef struct {
    Source   *source;  /* Source file pointer */
    char     *src;     /* Effectively `source->data` */
    size_t    len;     /* Length of `src` */
    size_t    pos;     /* Current position in `src` */
    size_t    rpos;    /* Current reading position in `src` */
    size_t    line;    /* Current line number in `src` */
    size_t    linepos; /* Position of current line in `src` */
    char      ch;      /* Current character */

    struct {           /* Stack of indentation levels */
        int *stack;    /* Sack array */
        int *sp;       /* Points to top of the stack */
        int  size;     /* Size of stack */
    } lvls;

    int       lvl;     /* Current indentation level (in spaces) */
    int       dedent;  /* Amount of pending dedents */
    int       lf;      /* `true` if the previous token was a T_LF */
} Scanner;

Scanner *scanner(Source *source);
void     scanner_free(Scanner *);
Token   *scan(Scanner *);
