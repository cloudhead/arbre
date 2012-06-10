/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * value.h
 *
 */

/*
 * TValue data-types
 */
typedef enum {
    TYPE_INVALID,
    TYPE_NONE,
    TYPE_ANY,
    TYPE_VAR,
    TYPE_ATOM,
    TYPE_BIN,
    TYPE_TUPLE,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_LIST,
    TYPE_PATH,
    TYPE_PATHID,
    TYPE_SELECT,
    TYPE_CLAUSE
} TYPE;

/*
 * Assuming only `Y` is bound
 *
 * `_`     -- TYPE_NONE
 * `X`     -- TYPE_ANY
 * `Y`     -- TYPE_VAR
 * `XS..`  -- TYPE_ANY | QUAL_RANGE
 */

/*
 * TValue data-type qualifiers
 */
typedef enum {
    Q_NONE     = 0,
    Q_RANGE    = 1 << 4 /* Bind ident 0001 0000 */
} QUAL;

#define  TYPE_QUAL_MASK  0xf0
#define  TYPE_MASK       0x0f

typedef struct {
    int  length;
    char value[];
} String;

struct PathID {
    const char *module;
    const char *path;
};

struct Select;
struct Clause;
struct Path;

typedef union {
    struct TValue  *tval;
    unsigned char   ident;
    bool            boolean;
    int             number;
    const char     *atom;
    String         *string;
    struct Clause  *clause;
    struct Tuple   *tuple;
    struct List    *list;
    struct Select  *select;
    struct Path    *path;
    struct PathID  *pathid;
} Value;

struct TValue {
    TYPE    t;
    Value   v;
};

struct TValueList {
    struct TValue     *head;
    struct TValueList *tail;
};

typedef struct TValueList TValueList;
typedef struct TValue     TValue;

struct Tuple {
    int     arity : 8;
    TValue  members[];
};
typedef struct Tuple Tuple;

struct List {
    TValue        *head;
    struct List   *tail;
};
typedef struct List List;

TValue *tvalue(TYPE type, Value val);
void    tvalue_pp(TValue *tval);
void    tvalues_pp(TValue *tval, int size);

TValue *tuple(int arity);
TValue *list(TValue *);
TValue *atom(const char *);
TValue *number(const char *);
List   *list_cons(List *list, TValue *e);
