/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * node.h
 *
 *   AST node & struct nodelist related declarations
 *
 */

/*
 * Node operation types
 */
typedef enum {
    /* Implicit */
    OBLOCK,

    /* Stateful */
    ODECL, OBIND, OMATCH,
    OMODULE, OSELECT, OPATH, OMPATH,
    OSPAWN,

    /* Operands */
    OIDENT, OTYPE, OSTRING, OCHAR,
    ONUMBER, OTUPLE, OLIST, OMAP,
    OATOM, OCLAUSE,

    /* Operators */
    OSELECTOR, OACCESS, OAPPLY, OSEND,
    ORANGE, OADD, OSUB, OWAIT, OPIPE,
    OCONS,

    /* Comparison */
    OLT, OGT, OEQ
} OP;

typedef enum {
    CMP_GT,
    CMP_LT
} CMP;

typedef enum {
    MODULE_ROOT = 1,
    MODULE_CURRENT,
    MODULE_NAMED
} MODULE;

typedef enum {
    PATH_MSG,
    PATH_PRIV,
    PATH_PUB
} PATH;

extern TYPE OP_TYPES[];

/*
 * # `struct node` and `struct nodelist` data-type definitions
 *
 *   Nodes represent abstract operations in syntax trees (see: `Tree`).
 *   Nodelists are linked-lists of nodes.
 *
 *   Nodes have "common" attributes, which is shared amongst all node types,
 *   and type-specific attributes which aren't shared.
 *
 *   Supposing we have three nodes: `N1`, `N2`, `N3`, the nodelist
 *   would look like this:
 *
 *      nodelist      .--> nodelist      .--> nodelist <------.
 *      .head -> N1   |    .head -> N2   |   .head -> N3      |
 *      .tail --------'    .tail --------'   .tail -> NULL    |
 *      .end -------------------------------------------------'
 *
 *   Additionally, the `prev` attribute points to the previous nodelist.
 *
 *   An empty nodelist looks like this:
 *
 *       nodelist <-----.
 *       .head -> NULL  |
 *       .tail -> NULL  |
 *       .end ----------'
 *
 */
struct node {
                              /* Common attributes */
    struct Sym    *sym;       /* Pointer to symbol table entry */
    struct source *source;    /* struct source file this node belongs to */
    size_t         pos;       /* Position of this node in the source file */
    char          *src;       /* Raw syntax */
    OP             op;        /* Operation (node type) */
    TYPE           type;      /* The node's type, after semantic analysis */
    union SymEntry entry;     /* Entry in symbol tables */

    union {                   /* Attributes specific to a node OP type */
        char  *string;
        char  *number;
        char  *atom;
        char   chr;

        /* TODO: Refactor this into single struct */
        struct { struct node  *lval;  struct node  *rval; } access;
        struct { struct node  *lval;  struct node  *rval; } apply;
        struct { struct node  *lval;  struct node  *rval; } send;
        struct { struct node  *lval;  struct node  *rval; } type;
        struct { struct node  *lval;  struct node  *rval; } range;
        struct { struct node  *lval;  struct node  *rval; } pipe;
        struct { struct node  *lval;  struct node  *rval; } add;
        struct { struct node  *lval;  struct node  *rval; } sub;
        struct { struct node  *lval;  struct node  *rval; } match;
        struct { struct node  *lval;  struct node  *rval; } bind;
        struct { struct node  *lval;  struct node  *rval; } cmp;
        struct { struct node  *lval;  struct node  *rval; } cons;
        struct { struct node  *lval;  struct node  *rval; } binop;

        struct {
            struct node  *proc;
        } wait;

        struct {
            struct node  *apply;
        } spawn;

        struct {
            PATH          type;
            struct node  *name;
            struct node  *clause;
        } path;

        struct {
            struct node     *lval;
            struct node     *rval;
            struct nodelist *guards;
            int             nguards;
        } clause;

        struct {
            PATH          type;
            struct node  *clause;
        } mpath;

        struct {
            struct node  *module;
            struct node  *args;
            struct node  *alias;
        } decl;

        struct {
            struct node  *path;
            MODULE        type;
        } module;

        struct {
            unsigned        nclauses;
            struct nodelist *clauses;
            struct node     *arg;
        } select;

        struct {
            struct node  *decl;
            struct node  *def;
        } ident;

        struct {
            unsigned          arity;
            struct nodelist  *members;
        } tuple;

        struct {
            unsigned         length;
            struct nodelist *items;
        } list;

        struct { struct nodelist *items; } map;

        struct {
            struct node      *parent;
            struct nodelist  *body;
        } block;
    } o;
};

struct nodelist {
    struct node     *head;
    struct nodelist *tail;
    struct nodelist *end;
    struct nodelist *prev;
};

void        pp_node(struct node *n);
void        pp_nodel(struct node *n, int lvl);
void        append(struct nodelist *list, struct node *n);

struct node  *node(Token *, OP op);
void          node_free(struct node *);

struct nodelist   *nodelist(struct node *head);
void               nodelist_free(struct nodelist *);

int         nodetos(struct node *t, char *buff);
