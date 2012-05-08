/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * node.h
 *
 *   AST Node & NodeList related declarations
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
    ORANGE, OADD, OWAIT, OPIPE
} OP;

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

/*
 * # `Node` and `NodeList` data-type definitions
 *
 *   `Node`s represent abstract operations in syntax trees (see: `Tree`).
 *   `NodeList`s are linked-lists of `Node`s.
 *
 *   `Node`s have "common" attributes, which is shared amongst all node types,
 *   and type-specific attributes which aren't shared.
 *
 *   Supposing we have three nodes: `N1`, `N2`, `N3`, the NodeList
 *   would look like this:
 *
 *      NodeList      .--> NodeList      .--> NodeList <------.
 *      .head -> N1   |    .head -> N2   |   .head -> N3      |
 *      .tail --------'    .tail --------'   .tail -> NULL    |
 *      .end -------------------------------------------------'
 *
 *   An empty NodeList looks like this:
 *
 *       NodeList <-----.
 *       .head -> NULL  |
 *       .tail -> NULL  |
 *       .end ----------'
 *
 */
struct Node {
                              /* Common attributes */
    struct Sym    *sym;       /* Pointer to symbol table entry */
    Source        *source;    /* Source file this node belongs to */
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
        struct { struct Node  *lval;  struct Node  *rval; } access;
        struct { struct Node  *lval;  struct Node  *rval; } apply;
        struct { struct Node  *lval;  struct Node  *rval; } send;
        struct { struct Node  *lval;  struct Node  *rval; } type;
        struct { struct Node  *lval;  struct Node  *rval; } range;
        struct { struct Node  *lval;  struct Node  *rval; } clause;
        struct { struct Node  *lval;  struct Node  *rval; } pipe;
        struct { struct Node  *lval;  struct Node  *rval; } add;
        struct { struct Node  *lval;  struct Node  *rval; } match;
        struct { struct Node  *lval;  struct Node  *rval; } bind;

        struct {
            struct Node  *proc;
        } wait;

        struct {
            struct Node  *apply;
        } spawn;

        struct {
            PATH          type;
            struct Node  *name;
            struct Node  *clause;
        } path;

        struct {
            PATH          type;
            struct Node  *clause;
        } mpath;

        struct {
            struct Node  *module;
            struct Node  *args;
            struct Node  *alias;
        } decl;

        struct {
            struct Node  *path;
            MODULE        type;
        } module;

        struct {
            struct NodeList *clauses;
        } select;

        struct {
            struct Node  *decl;
            struct Node  *def;
        } ident;

        struct {
            unsigned          arity;
            struct NodeList  *members;
        } tuple;

        struct {
            unsigned         length;
            struct NodeList *items;
        } list;

        struct { struct NodeList *items; } map;

        struct {
            struct NodeList  *body;
        } block;
    } o;
};

struct NodeList {
    struct Node     *head;
    struct NodeList *tail;
    struct NodeList *end;
};

typedef  struct Node      Node;
typedef  struct NodeList  NodeList;

void        pp_node(Node *n);
void        pp_nodel(Node *n, int lvl);
void        append(NodeList *list, Node *n);

Node       *node(Token *, OP op);
void        node_free(Node *);

NodeList   *nodelist(Node *head);
void        nodelist_free(NodeList *);

int         nodetos(Node *t, char *buff);
