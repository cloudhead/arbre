/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * reduce.h
 *
 */
typedef struct {
    int _;
} Reducer;

static struct nodelist *reduce_nodelist(Reducer *r, struct nodelist *ns);
static struct node *reduce_path(Reducer *r, struct node *n);
static struct node *reduce_list(Reducer *r, struct node *n);
static struct node *reduce_block(Reducer *r, struct node *n);
static struct node *reduce_bind(Reducer *r, struct node *n);
static struct node *reduce_apply(Reducer *r, struct node *n);
static struct node *reduce_select(Reducer *r, struct node *n);
static struct node *reduce_pipe(Reducer *r, struct node *n);

struct node *(*REDUCERS[])(Reducer *, struct node *) = {
    [OBLOCK]    =  reduce_block,  [ODECL]     =  NULL,
    [OMATCH]    =  NULL,          [OBIND]     =  reduce_bind,
    [OMODULE]   =  NULL,          [OSELECT]   =  reduce_select,
    [OWAIT]     =  NULL,          [OIDENT]    =  NULL,
    [OTYPE]     =  NULL,          [OADD]      =  NULL,
    [OPATH]     =  reduce_path,   [OMPATH]    =  NULL,
    [OSTRING]   =  NULL,          [OATOM]     =  NULL,
    [OCHAR]     =  NULL,          [ONUMBER]   =  NULL,
    [OTUPLE]    =  NULL,          [OLIST]     =  reduce_list,
    [OACCESS]   =  NULL,          [OAPPLY]    =  reduce_apply,
    [OSEND]     =  NULL,          [ORANGE]    =  NULL,
    [OCLAUSE]   =  NULL,          [OPIPE]     =  reduce_pipe,
    [OSUB]      =  NULL,          [OLT]       =  NULL,
    [OGT]       =  NULL
};

#define node_access(l, r) (binop(OACCESS, l, r))

static struct node *anode(OP op)
{
    struct node *n   = calloc(1, sizeof(*n));
    n->op     = op;
    return n;
}

static struct node *node_apply(struct node *lval, struct node *rval)
{
    struct node *n = anode(OAPPLY);
    n->o.apply.lval = lval;
    n->o.apply.rval = rval;
    return n;
}

static struct node *binop(OP op, struct node *lval, struct node *rval)
{
    struct node *n = anode(op);
    n->o.binop.lval = lval;
    n->o.binop.rval = rval;
    return n;
}

static struct node *node_atom(const char *name)
{
    struct node  *n = anode(OATOM);
           n->o.atom = (char *)name;
    return n;
}

static struct node *ntuple(int arity, ...)
{
    va_list ap;

    struct node  *n = anode(OTUPLE), *e;
           n->o.tuple.arity = arity;
           n->o.tuple.members = nodelist(NULL);

    va_start(ap, arity);

    for (int i = 0; i < arity; i++) {
        e = va_arg(ap, struct node *);
        append(n->o.tuple.members, e);
    }
    va_end(ap);

    return n;
}

static struct node *aapply(const char *module, const char *path, struct node *rval)
{
    return node_apply(node_access(node_atom(module), node_atom(path)), rval);
}

static struct node *cons(struct node *lval, struct node *rval)
{
    struct node *n = anode(OCONS);
    n->o.cons.lval = lval;
    n->o.cons.rval = rval;
    return n;
}

static struct node *reduce_pipe(Reducer *r, struct node *n)
{
    struct node *t = ntuple(2, n->o.pipe.lval, n->o.pipe.rval);
    return aapply("io", "pipe", t);
}

static struct node *reduce_list(Reducer *r, struct node *n)
{
    if (n->o.list.length == 0)
        return cons(NULL, NULL);

    struct nodelist *ns = n->o.list.items;
    struct nodelist *end = ns->end;

    struct node *new = NULL;

    while (end) {
        new = cons(end->head, new);
        end = end->prev;
    }

    return new;
}

static void reduce_node(Reducer *r, struct node **n)
{
    struct node *new;

    if (*n == NULL)
        return;

    if (REDUCERS[(*n)->op]) {
        new = REDUCERS[(*n)->op](r, *n);
        if (new != *n) {
            // TODO: Free *n
        }
        *n = new;
    }
}

static struct node *reduce_bind(Reducer *r, struct node *n)
{
    reduce_node(r, &n->o.bind.lval);
    reduce_node(r, &n->o.bind.rval);
    return n;
}

static struct node *reduce_apply(Reducer *r, struct node *n)
{
    reduce_node(r, &n->o.apply.lval);
    reduce_node(r, &n->o.apply.rval);
    return n;
}

static struct node *reduce_select(Reducer *r, struct node *n)
{
    reduce_node(r, &n->o.select.arg);
    reduce_nodelist(r, n->o.select.clauses);
    return n;
}

static struct node *reduce_clause(Reducer *r, struct node *n)
{
    reduce_block(r, n->o.clause.rval);
    return n;
}

static struct node *reduce_path(Reducer *r, struct node *n)
{
    reduce_clause(r, n->o.path.clause);
    return n;
}

static struct node *reduce_block(Reducer *r, struct node *n)
{
    struct nodelist *ns = n->o.block.body;
    reduce_nodelist(r, ns);
    return n;
}

static struct nodelist *reduce_nodelist(Reducer *r, struct nodelist *ns)
{
    while (ns) {
        reduce_node(r, &ns->head);
        ns = ns->tail;
    }
    return ns;
}

void reduce(Tree *tree)
{
    Reducer *r = malloc(sizeof(*r));

    reduce_block(r, tree->root);

    free(r);
}
