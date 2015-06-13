/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * tree.h
 *
 */

struct Tree {
    struct node  *root;
    SymTable     *symbols;
    SymTable     *tsymbols;
    SymTable     *psymbols;
};

typedef     struct Tree      Tree;

Tree       *tree(void);
void        tree_free(Tree *);

Sym        *tree_lookup(Tree *t, char *k);

void        enterscope(Tree *t);
void        exitscope(Tree *t);

void        pp_tree(Tree *);

struct ClauseEntry {
    struct node    *node;

    /* Constants */
    SymTable       *ktable;
    struct tvalue **kheader;
    unsigned        kindex;

    /* Locals */
    int            nlocals;
    uint8_t        nreg;

    /* Code */
    uint32_t       *code;
    unsigned long   codesize;
    unsigned long   pc;
};

typedef struct ClauseEntry ClauseEntry;

struct PathEntry {
    char          *name;
    struct node   *node;
    uint8_t        index;

    /* Clauses */
    int            nclauses;
    ClauseEntry    *clause;
    ClauseEntry   **clauses;
};

typedef unsigned Register;

struct Variable {
    char          *name;
    struct node   *def;
    struct Type   *type;
    Register       reg : 8;
};

typedef struct Variable Variable;
typedef struct PathEntry PathEntry;

Variable     *var(char *, Register);
PathEntry    *pathentry(char *, struct node *, uint8_t);
ClauseEntry  *clauseentry(struct node *, uint8_t);
