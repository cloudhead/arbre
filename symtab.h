/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * symtab.h
 *
 * Symbol-table definition
 *
 */
union SymEntry {
    struct Variable  *var;
    struct Type      *type;
    struct PathEntry *path;
    struct tvalue    *tval;
};

enum SymType {
    SYM_VAR,
    SYM_TYPE,
    SYM_PATH,
    SYM_TVAL
};

struct Sym {
    const char       *name;
    enum  SymType     type;
    union SymEntry    e;
};

struct SymList {
    struct Sym      *head;
    struct SymList  *tail;
};

typedef  struct Sym      Sym;
typedef  struct SymList  SymList;

struct SymTable {
    size_t           size;
    struct SymTable *parent;
    struct SymList  *data[];
};
typedef  struct SymTable  SymTable;

SymTable *symtab(size_t size);
void      symtab_free(SymTable *t);
Sym      *symtab_lookup(SymTable *t, const char *k);
void      symtab_insert(SymTable *t, const char *k, Sym *s);
void      symtab_pp(SymTable *t);

Sym      *symbol(const char *k, struct Variable *v);
Sym      *psymbol(const char *k, struct PathEntry *p);
Sym      *tvsymbol(const char *k, struct tvalue *tv);
SymList  *symlist(Sym *head);
void      sym_prepend(SymList *list, Sym *s);

