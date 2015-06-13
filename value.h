/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * value.h
 *
 */

/*
 * Tvalue data-types
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
 * Tvalue data-type qualifiers
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
struct clause;
struct path;

typedef union {
	struct tvalue  *tval;
	unsigned char   ident;
	bool            boolean;
	int             number;
	const char     *atom;
	String         *string;
	struct clause  *clause;
	struct Tuple   *tuple;
	struct List    *list;
	struct Select  *select;
	struct path    *path;
	struct PathID  *pathid;
} Value;

struct tvalue {
	TYPE    t;
	Value   v;
};

struct tvaluelist {
	struct tvalue     *head;
	struct tvaluelist *tail;
};

struct Tuple {
	int           arity : 8;
	struct tvalue members[];
};
typedef struct Tuple Tuple;

struct List {
	struct tvalue *head;
	struct List   *tail;
};
typedef struct List List;

struct tvalue *tvalue(TYPE type, Value val);
void           tvalue_pp(struct tvalue *tval);
void           tvalues_pp(struct tvalue *tval, int size);

struct tvalue *tuple(int arity);
struct tvalue *list(struct tvalue *);
struct tvalue *atom(const char *);
struct tvalue *number(const char *);
List   *list_cons(List *list, struct tvalue *e);
