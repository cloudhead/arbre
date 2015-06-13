/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * runtime.h
 *
 */
#define  STACK_MAXDIFF  4096

struct clause {
    struct path    *path;
    struct tvalue   pattern;
    Instruction    *code;
    unsigned long   codelen; /* TODO: Rename to ncode */
    int            nlocals;
    struct tvalue  *constants;
    int             constantsn; /* TODO: Rename to nconstants */
    int             pc;
};

struct Select {
    int            nclauses : 8;
    struct clause  *clauses[];
};
typedef struct Select Select;

struct path {
    const char     *name;
    struct module  *module;

    /* Clauses */
    int            nclauses;
    struct clause  *clause;
    struct clause **clauses;
};

struct module {
    const char   *name;
    struct path **paths;
    unsigned      pathc;
};

struct modulelist {
    struct module     *head;
    struct modulelist *tail;
};

struct frame {
    struct frame    *prev;
    Instruction     *pc;
    struct clause   *clause;
    uint8_t          result;
    struct tvalue    locals[];
};

struct stack {
    struct frame   *base;  /* Base of the stack */
    struct frame   *frame; /* struct frame pointer */
    size_t          size;
    size_t          capacity;
    int             depth;
};

#define PROC_WAITING 1
#define PROC_READY   1 << 1

typedef struct {
    struct stack   *stack;
    struct path    *path;
    struct module  *module;
    struct clause  *clause;
    uint64_t        pc;
    uint16_t        credits;
    uint8_t         flags;
} Process;

struct module     *module          (const char *name, unsigned pathc);
struct path       *module_path     (struct module *m, const char *path);
void               module_prepend  (struct modulelist *list, struct module *m);
struct modulelist *modulelist      (struct module *head);

struct stack   *stack           (void);
void            stack_push      (struct stack *s, struct clause *c);
struct frame   *stack_pop       (struct stack *s);
void            stack_pp        (struct stack *s);

struct path       *path            (const char *name, int nclauses);
Process           *process         (struct module *m, struct path *path);
struct frame      *frame           (struct tvalue *locals, int nlocals);
void               frame_pp        (struct frame *);

struct clause     *clause          (struct tvalue pattern, int nlocals, int clen);
struct tvalue     *select_         (int nclauses);
