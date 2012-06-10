/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * runtime.h
 *
 */
struct Clause {
    struct Path    *path;
    TValue          pattern;
    Instruction    *code;
    unsigned long   codelen; /* TODO: Rename to ncode */
    int            nlocals;
    TValue         *constants;
    int             constantsn; /* TODO: Rename to nconstants */
    int             pc;
};
typedef struct Clause Clause;

struct Select {
    int     nclauses : 8;
    Clause  *clauses[];
};
typedef struct Select Select;

struct Path {
    const char     *name;
    struct Module  *module;

    /* Clauses */
    int            nclauses;
    Clause         *clause;
    Clause        **clauses;
};
typedef struct Path Path;

struct Module {
    const char   *name;
    struct Path **paths;
    unsigned      pathc;
};

struct ModuleList {
    struct Module     *head;
    struct ModuleList *tail;
};

typedef struct Module     Module;
typedef struct ModuleList ModuleList;

typedef struct {
    void     *prev;
    uint64_t  pc;
    uint8_t   result;
    Module   *module;
    Path     *path;
    Clause   *clause;
    TValue   *locals;
    int      nlocals;
} Frame;

typedef struct {
    Frame   **frames;
    Frame   **frame; /* Frame pointer */
    int       size;
    int       capacity;
} Stack;

#define PROC_WAITING 1
#define PROC_READY   1 << 1

typedef struct {
    Stack    *stack;
    Path     *path;
    Module   *module;
    Clause   *clause;
    uint64_t  pc;
    uint16_t  credits;
    uint8_t   flags;
} Process;

Module     *module          (const char *name, unsigned pathc);
Path       *module_path     (Module *m, const char *path);
void        module_prepend  (ModuleList *list, Module *m);
ModuleList *modulelist      (Module *head);

Stack      *stack           (void);
void        stack_push      (Stack *s, Frame *f);
Frame      *stack_pop       (Stack *s);
void        stack_pp        (Stack *s);

Path       *path            (const char *name, int nclauses);
Process    *process         (Module *m, Path *path);
Frame      *frame           (TValue *locals, int nlocals);
void        frame_pp        (Frame *);

Clause     *clause          (TValue pattern, int nlocals, int clen);
TValue     *select          (int nclauses);
