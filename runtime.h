/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * runtime.h
 *
 */
typedef struct {
    const char     *name;
    Instruction    *code;
    unsigned long   codelen; /* TODO: Rename to ncode */
    TValue         *constants;
    int             constantsn; /* TODO: Rename to nconstants */
    int             pc;
} Path;

struct Module {
    const char  *name;
    Path       **paths;
    unsigned     pathc;
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
    TValue   *locals;
    int      nlocals;
} Frame;

typedef struct {
    Frame   **frames;
    Frame   **frame; /* Frame pointer */
    int       size;
} Stack;

typedef struct {
    Stack *stack;
    Path *path;
    Module *module;
    uint64_t pc;
} Process;

Module     *module          (const char *name, Path *paths[], unsigned pathc);
Path       *module_path     (Module *m, const char *path);
void        module_prepend  (ModuleList *list, Module *m);
ModuleList *modulelist      (Module *head);

Stack      *stack           (void);
void        stack_push      (Stack *s, Frame *f);
Frame      *stack_pop       (Stack *s);
void        stack_pp        (Stack *s);

Path       *path            (const char *name, int clen);
Process    *process         (Module *m, Path *path);
Frame      *frame           (TValue *locals, int nlocals);
void        frame_pp        (Frame *);

