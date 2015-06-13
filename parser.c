/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * parser.c
 *
 *   raw syntax parser
 *
 * + parser
 * - parser_free
 *
 * -> parse
 *
 *   Transforms a source file into an abstract syntax tree.
 *
 * TODO: Use `isnext` where appropriate
 *
 */
#include  <ctype.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <stdarg.h>

#include  "arbre.h"
#include  "scanner.h"
#include  "parser.h"
#include  "util.h"

#define REPORT_PARSER
#include  "report.h"
#undef  REPORT_PARSER

/* I'm not sure why we have to declare this,
 * but clang complains otherwise. */
char *strndup(char const *, unsigned long);

static  bool   expect(Parser *p, TOKEN t);
static  void   error(Parser *p, const char *fmt, ...);

static  struct node  *parse_expression(Parser *);
static  struct node  *parse_primary(Parser *);
static  struct node  *parse_access(Parser *);
static  struct node  *parse_apply(Parser *, struct node *);
static  struct node  *parse_number(Parser *);
static  struct node  *parse_module(Parser *);
static  struct node  *parse_bind(Parser *, struct node *);
static  struct node  *parse_match(Parser *p, struct node *lval);
static  struct node  *parse_send(Parser *, struct node *);
static  struct node  *parse_clause(Parser *);
static  struct node  *parse_spawn(Parser *);
static  struct node  *parse_select(Parser *p, struct node *arg);

/*
 * Parser allocator/initializer
 */
Parser *parser(struct source *src)
{
	Parser  *p = malloc(sizeof(*p));
	p->scanner = scanner(src);
	p->tree    = tree();
	p->root    = p->tree->root;
	p->token   = token(T_ILLEGAL, NULL, -1, NULL);
	p->ontoken = NULL;
	p->src     = NULL;
	p->tok     = p->token->tok;
	p->errors  = 0;
	p->block   = NULL;

	return p;
}

/*
 * Parser deallocator
 */
void parser_free(Parser *p)
{
	scanner_free(p->scanner);
	tree_free(p->tree);
	free(p);
}

/*
 * Print information about the parser's state.
 */
void pparser(Parser *p)
{
	printf("Parser{tok=%s", TOKEN_STRINGS[p->tok]);
	p->src && printf("  src=`%s`", escape(p->src));
	printf("  line=%lu", p->scanner->line + 1);
	printf("  pos=%lu}\n", p->pos);
}

/*
 * Set a node's `src` property automatically
 */
static void setsrc(Parser *p, struct node *n)
{
	n->src = strndup(p->scanner->src + n->pos, p->pos - n->pos);
}

/*
 * Advance parser to the next token, by calling `scan`.
 */
static TOKEN next(Parser *p)
{
	p->token = scan(p->scanner);
	p->tok   = p->token->tok;
	p->src   = p->token->src;
	p->pos   = p->token->pos;

	if (p->ontoken) p->ontoken(p->token);

	return p->tok;
}

/*
 * Advance parser to the beginning of the next line.
 * Useful when there is a parse error.
 */
static void nextline(Parser *p)
{
	while (p->tok != T_LF && p->tok != T_EOF)
		next(p);
	if (p->tok == T_LF)
		next(p);

	/* TODO: Handle this elsewhere */
	if (p->tok == T_EOF)
		exit(1);
}

/*
 * Emit a parse error, try to recover.
 */
static void error(Parser *p, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vpreportf(REPORT_ERROR, p, fmt, ap);
	va_end(ap);

	p->errors ++;

	nextline(p);
}

/*
 * Expect a token, raise an error if it's
 * not there.
 */
static bool expect(Parser *p, TOKEN t)
{
	if (p->tok != t) {
		error(p, "expected %s got `%s`", TOKEN_STRINGS[t], escape(p->src));
		return false;
	}
	next(p);
	return true;
}

/*
 * Try to consume a token, return false if it's
 * not there.
 */
static bool isnext(Parser *p, TOKEN t)
{
	if (p->tok != t) {
		return false;
	}
	next(p);
	return true;
}

/*
 * Expect an end-of-statement token.
 */
static void end(Parser *p)
{
	if (p->tok == T_SEMICOLON)
		next(p);
	if (p->tok == T_COMMENT)
		next(p);
}

/*
 * Block allocator/initializer
 */
static struct node *block(Parser *p)
{
	struct node  *b = node(p->token, OBLOCK);
	b->o.block.parent = p->block;
	b->o.block.body   = nodelist(NULL);
	return b;
}

/*
 * Accessor allocator/initializer
 */
static struct node *access(Parser *p, struct node *base, struct node *attr)
{
	struct node *n = node(p->token, OACCESS);
	n->o.access.lval = base;
	n->o.access.rval = attr;
	return n;
}

/*
 * Range allocator/initializer
 */
static struct node *range(Parser *p, struct node *from, struct node *to)
{
	struct node *n = node(p->token, ORANGE);
	n->o.range.lval = from;
	n->o.range.rval = to;
	return n;
}

/*
 * Parse a generic comma-delimited sequence.
 *
 * Store length in `*size`. The items
 * are parsed with the given `parse` function.
 *
 *     list  ::= open items? close
 *     items ::= (exp ',' items) | exp
 */
static struct nodelist *parse_seq(Parser *p,
	                              TOKEN open,
	                              TOKEN close,
	                              struct node *(parse)(Parser *),
	                              unsigned *size)
{
	struct nodelist *ns = nodelist(NULL);
	struct node     *n  = NULL;

	expect(p, open); /* Eat opening delimiter */

	int len = 0;

	if (p->tok == close) {
		next(p);
	} else {
		while ((n = parse(p))) {
			append(ns, n);
			len ++;

			if (p->tok == T_COMMA) {
				next(p);
			} else {
				expect(p, close); /* Eat closing delimiter */
				break;
			}
		}
	}
	size && (*size = len);

	return n ? ns : NULL;
}

/*
 * Parse tuple. This is a fixed-length
 * set of discrete values; each value
 * is an expression. Example:
 *
 *     ("fnord", 42, ('h', 'i'))
 */
static struct node *parse_tuple(Parser *p)
{
	unsigned len;
	struct node *n;
	struct nodelist *members = parse_seq(p, T_LPAREN,
	                                     T_RPAREN,
	                                     &parse_expression,
	                                     &len);
	if (len == 1) {
		n = members->head;
	} else {
		n = node(p->token, OTUPLE);
		setsrc(p, n);

		if (len > 1) {
			n->o.tuple.arity   = len;
			n->o.tuple.members = members;
		}
	}
	return n;
}

/*
 * Parse list expression. The difference between this
 * and a generic expression is that it may be a range
 * expression. Example:
 *
 *     9
 *     1..10
 *     x..
 */
static struct node *parse_list_exp(Parser *p)
{
	struct node *n = NULL, *to = NULL;

	if ((n = parse_expression(p))) {
		if (p->tok == T_ELLIPSIS) {
			next(p);
			if (p->tok != T_RBRACK && p->tok != T_COMMA) {
				to = parse_expression(p);
			}
			n = range(p, n, to);
		}
	}
	return n;
}

/*
 * Parse list. Example:
 *
 *     [1, 2, 3..9]
 *     [x..]
 */
static struct node *parse_list(Parser *p)
{
	unsigned len = 0;
	struct node *n = node(p->token, OLIST);

	if ((n->o.list.items = parse_seq(p, T_LBRACK, T_RBRACK, &parse_list_exp, &len))) {
		setsrc(p, n);
	}
	n->o.list.length = len;
	return n;
}

/*
 * Parse map expression. Example:
 *
 *     key: "value"
 */
static struct node *parse_map_exp(Parser *p)
{
	return parse_clause(p);
}

/*
 * Parse map. Example:
 *
 *     {one: 1, two: 2}
 */
static struct node *parse_map(Parser *p)
{
	struct node *n = node(p->token, OMAP);
	if ((n->o.map.items = parse_seq(p, T_LBRACE, T_RBRACE, &parse_map_exp, NULL))) {
		setsrc(p, n);
		return n;
	}
	return NULL;
}


/*
 * Parse identifier. Example:
 *
 *     fnord
 */
static struct node *parse_ident(Parser *p)
{
	struct node *n = node(p->token, OIDENT);
	n->o.ident.decl  = NULL;
	n->o.ident.def   = NULL;
	next(p);
	return n;
}

/*
 * Parse atom. Example:
 *
 *     'fnord
 */
static struct node *parse_atom(Parser *p)
{
	struct node *n = node(p->token, OATOM);
	n->o.atom = p->src;
	n->type = TYPE_ATOM;
	next(p);
	return n;
}

/*
 * Parse string. Example:
 *
 *     "fnord"
 */
static struct node *parse_string(Parser *p)
{
	struct node *n = node(p->token, OSTRING);
	n->o.string = p->src;
	n->type = TYPE_STRING;
	next(p);
	return n;
}

/*
 * Parse character. Example:
 *
 *     `y`
 */
static struct node *parse_char(Parser *p)
{
	struct node *n = node(p->token, OCHAR);
	n->o.chr = *(p->src);
	next(p);
	return n;
}

/*
 * Parse number. Example:
 *
 *     42
 */
static struct node *parse_number(Parser *p)
{
	struct node *n = node(p->token, ONUMBER);
	n->o.number = p->src;
	n->type = TYPE_NUMBER;
	next(p);
	return n;
}

/*
 * Parse a message wait. Example:
 *
 *     <-fnord
 */
static struct node *parse_wait(Parser *p)
{
	struct node *n = node(p->token, OWAIT), *e;
	int type;

	switch (p->tok) {
		case T_LARROW:
			next(p);
		default:
			type = 0;
	}
	e = parse_ident(p);

	n->o.wait.proc = e;

	return n;
}


/*
 * Parse lambda expression. Example:
 *
 *     \x : x + 1
 */
static struct node *parse_lambda(Parser *p)
{
	next(p); // '\'

	struct node *n = parse_clause(p);

	return n;
}

/*
 * Parse pipe. Example:
 *
 *     A -> B
 *     A -> (\X : X + 1) -> B
 */
static struct node *parse_pipe(Parser *p, struct node *lval)
{
	struct node *n;

	while (p->tok == T_RARROW || p->tok == T_RDARROW || p->tok == T_REQARROW) {
		next(p); // '->'
		n = node(p->token, OPIPE);
		n->o.pipe.lval = lval;
		lval = n;

		switch (p->tok) {
			case T_IDENT:  n->o.pipe.rval = parse_ident(p);  break;
			case T_BSLASH: n->o.pipe.rval = parse_lambda(p); break;
			case T_LPAREN: n->o.pipe.rval = parse_tuple(p);  break;
			default:                                         return n;
		}
	}
	return n;
}

/*
 * Parse addition. Example:
 *
 *     a + b
 */
static struct node *parse_add(Parser *p, struct node *lval)
{
	struct node *n = node(p->token, OADD);

	next(p); // '+'

	n->o.add.lval = lval;
	n->o.add.rval = parse_expression(p);

	return n;
}

/*
 * Parse subtraction. Example:
 *
 *     a - b
 */
static struct node *parse_sub(Parser *p, struct node *lval)
{
	struct node *n = node(p->token, OSUB);

	next(p); // '-'

	n->o.sub.lval = lval;
	n->o.sub.rval = parse_expression(p);

	return n;
}

/*
 * Parse lesser-than comparison. Example:
 *
 *     A < B
 */
static struct node *parse_lt(Parser *p, struct node *lval)
{
	struct node *n = node(p->token, OLT);

	next(p); // '<'

	n->o.cmp.lval = lval;
	n->o.cmp.rval = parse_expression(p);

	return n;
}

/*
 * Parse greater-than comparison. Example:
 *
 *     A > B
 */
static struct node *parse_gt(Parser *p, struct node *lval)
{
	struct node *n = node(p->token, OGT);

	next(p); // '>'

	n->o.cmp.lval = lval;
	n->o.cmp.rval = parse_expression(p);

	return n;
}

/*
 * Parse an expression. This can be almost
 * anything which returns a value.
 */
static struct node *parse_expression(Parser *p)
{
	struct node *n = NULL;

	switch (p->tok) { // Parse lval
		case T_IDENT:
		case T_PERIOD:   n = parse_access(p);       break;
		case T_ATOM:     n = parse_atom(p);         break;
		case T_BSLASH:   n = parse_lambda(p);       break;
		case T_LPAREN:   n = parse_tuple(p);        break;
		case T_LBRACK:   n = parse_list(p);         break;
		case T_LBRACE:   n = parse_map(p);          break;
		case T_STRING:   n = parse_string(p);       break;
		case T_CHAR:     n = parse_char(p);         break;
		case T_INT:      n = parse_number(p);       break;
		case T_LARROW:   n = parse_wait(p);         break;
		case T_PLUS:     n = parse_spawn(p);        goto question;
		case T_QUESTION: n = parse_select(p, NULL); break;
		default:         error(p, ERR_DEFAULT); return NULL;
	}

	switch (p->tok) { // Parse rval
		case T_QUESTION:   n = parse_select(p, n); break;
		case T_IDENT:  case T_LPAREN:
		case T_STRING: case T_INT:
		case T_LBRACK: case T_ATOM:       n = parse_apply(p, n);   break;
		case T_LARROW: case T_LDARROW:    n = parse_send(p, n);    break;
		case T_RARROW: case T_RDARROW:    n = parse_pipe(p, n);    break;
		case T_DEFINE:                    n = parse_bind(p, n);    break;
		case T_EQ:                        n = parse_match(p, n);   break;
		case T_PLUS:                      n = parse_add(p, n);     break;
		case T_MINUS:                     n = parse_sub(p, n);     break;
		case T_GT:                        n = parse_gt(p, n);      break;
		case T_LT:                        n = parse_lt(p, n);      break;
		default:                                                   break;
	}

question:

	switch (p->tok) {
		case T_QUESTION:   n = parse_select(p, n); break;
		default:                                   break;
	}
	return n;
}

static void parse_comments(Parser *p)
{
	while (p->tok == T_COMMENT) {
		next(p);

		if (p->tok == T_LF)
			next(p);
	}
}

/*
 * Parse a code block. It is essentially a
 * scoped list of primary expressions.
 *
 * Blocks with multiple statements use the
 * T_INDENT and T_DEDENT tokens to delimit
 * themselves. Inline blocks look for T_LF.
 */
static struct node *parse_block(Parser *p)
{
	struct node *n = block(p);

	p->block = n;

	if (p->tok == T_LF) { /* Multi-line block */
		next(p);

		parse_comments(p);

		expect(p, T_INDENT);

		while (p->tok != T_DEDENT) {
			if (p->tok == T_LF || p->tok == T_COMMENT) {
				next(p);
				continue;
			}
			append(n->o.block.body, parse_primary(p));
		}
		expect(p, T_DEDENT);
	} else {              /* Inline block */
		append(n->o.block.body, parse_primary(p));
	}
	p->block = n;

	return n;
}

/*
 * Parse property accessors. Example:
 *
 *     fnord/x/y
 *
 * The AST for the example above would be:
 *
 *     (/ fnord (/ x y))
 */
static struct node *parse_access(Parser *p)
{
	struct node *n = NULL, *a;

	switch (p->tok) {
		case T_IDENT:  n = parse_ident(p);   break;
		case T_PERIOD: n = parse_module(p);  break;
		case T_INT:    n = parse_number(p);  break;
		default:                             return NULL;
	}

	if (n == NULL)
		return NULL;

	if (p->tok == T_SLASH) {
		next(p);

		if ((a = parse_access(p))) {
			n = access(p, n, a);
		}
	}
	return n;
}

/*
 * Parse a pattern expression. These are almost
 * like generic expressions, except they cannot
 * contain operations or function calls, they must
 * be literal/static. Example:
 *
 *     ("hello", fnord)
 *     ('ok, data)
 */
static struct node *parse_pattern(Parser *p)
{
	struct node *n = NULL;

	switch (p->tok) {
		case T_UNDER   :
		case T_IDENT   : n = parse_ident(p);    break;
		case T_LPAREN  : n = parse_tuple(p);    break;
		case T_LBRACE  : n = parse_map(p);      break;
		case T_LBRACK  : n = parse_list(p);     break;
		case T_ATOM    : n = parse_atom(p);     break;
		case T_STRING  : n = parse_string(p);   break;
		case T_INT     : n = parse_number(p);   break;
		default        : error(p, "unrecognised pattern");
	}
	return n;
}

/*
 * Parse a variable definition.
 * Example:
 *
 *     John := (human, male, 29)
 */
static struct node *parse_bind(Parser *p, struct node *lval)
{
	struct node *n;

	n = node(p->token, OBIND);
	n->o.bind.lval = lval;

	switch (p->tok) {
		case T_DEFINE:
			next(p);
			n->o.bind.rval = parse_expression(p);
			break;
		default:
			break;
	}
	return n;
}

/*
 * Parse a match operation. Example:
 *
 *     42 = 42
 *     (ok, _) = (ok, 42)
 */
static struct node *parse_match(Parser *p, struct node *lval)
{
	struct node *n;

	n = node(p->token, OMATCH);
	n->o.match.lval = lval;

	switch (p->tok) {
		case T_EQ:
			next(p);
			n->o.match.rval = parse_expression(p);
			break;
		default:
			break;
	}
	return n;
}

/*
 * Parse module name. Example:
 *
 *     http
 *     .
 */
static struct node *parse_module(Parser *p)
{
	struct node *n = node(p->token, OMODULE);

	switch (p->tok) {
		case T_SLASH:
			next(p); // '/'
			n->o.module.type = MODULE_ROOT;
			n->o.module.path = NULL;
			break;
		case T_PERIOD:
			next(p); // '.'
			n->o.module.type = MODULE_CURRENT;
			n->o.module.path = NULL;
			break;
		case T_IDENT:
			next(p);
			n->o.module.type = MODULE_NAMED;
			n->o.module.path = parse_ident(p);
			break;
		default:
			return NULL;
	}
	return n;
}

/*
 * Parse clause. Example:
 *
 *     X : X + 1
 */
static struct node *parse_clause(Parser *p)
{
	struct node *n = node(p->token, OCLAUSE);

	if (p->tok == T_COLON) {
		next(p);
		n->o.clause.lval = node(p->token, OTUPLE);
		n->o.clause.lval->o.tuple.arity = 0;
	} else if ((n->o.bind.lval = parse_pattern(p))) {
		expect(p, T_COLON);
	} else {
		error(p, "expected clause pattern");
		return NULL;
	}
	n->o.bind.rval = parse_block(p);
	return n;
}

/*
 * Parse message path. Example:
 *
 *     + (ping) : pong
 */
static struct node *parse_msgpath(Parser *p)
{
	struct node *n = node(p->token, OMPATH);

	next(p); // Consume `+`

	n->o.mpath.type   = PATH_MSG;
	n->o.mpath.clause = parse_clause(p);

	return n;
}

/*
 * Parse spawn. Example:
 *
 *     + ./start (8080)
 */
static struct node *parse_spawn(Parser *p)
{
	struct node *n = node(p->token, OSPAWN);
	struct node *a = NULL;

	next(p); // Consume `+`

	switch (p->tok) {
		case T_PERIOD: case T_IDENT: case T_ATOM:
			a = parse_access(p); break;
		default:
			// Error
			error(p, "expected function call");
	}
	n->o.spawn.apply = parse_apply(p, a);

	return n;
}

/*
 * Parse path. Example:
 *
 *     ./ping : pong
 */
static struct node *parse_path(Parser *p)
{
	struct node  *n = NULL;
	PATH   type;

	switch (p->tok) {
		case T_PERIOD:
			next(p);
			expect(p, T_SLASH);
			type = PATH_PUB;
			break;
		case T_IDENT:
			type = PATH_PRIV;
			break;
		default:
			error(p, ERR_DEFAULT);
			return NULL;
	}
	n = node(p->token, OPATH);

	if (p->tok == T_IDENT) {
		n->o.path.name = parse_ident(p);
	} else {
		error(p, "expected ident");
	}
	n->o.path.type   = type;
	n->o.path.clause = node(p->token, OCLAUSE);

	if (p->tok == T_EQ) {
		next(p);
		n->o.path.clause->o.clause.lval = node(p->token, OTUPLE);
		n->o.path.clause->o.clause.lval->o.tuple.arity = 0;
	} else if ((n->o.path.clause->o.clause.lval = parse_pattern(p))) {
		expect(p, T_EQ);
	} else {
		error(p, "expected clause pattern");
		return NULL;
	}
	n->o.path.clause->o.clause.rval = parse_block(p);

	return n;
}

/*
 * Parse a message send. Example:
 *
 *     Fnord <- ("hi", 42)
 */
static struct node *parse_send(Parser *p, struct node *t)
{
	struct node *n = node(p->token, OSEND), *e;
	int    type;

	switch (p->tok) {
		case T_LARROW:
			next(p);
		default:
			type = 0;
	}
	e = parse_expression(p);

	n->o.send.lval = t;
	n->o.send.rval = e;

	return n;
}

static struct node *parse_guard(Parser *p)
{
	return parse_expression(p);
}

static struct nodelist *parse_guards(Parser *p, int *len)
{
	struct node     *n;
	struct nodelist *ns = nodelist(NULL);

	do {
		n = parse_guard(p);
		append(ns, n);
		(*len) ++;
	} while (isnext(p, T_COMMA));

	return ns;
}

/*
 * Parse select. Example:
 *
 *     A | B | (X : X + 1) | C
 */
static struct node *_parse_select(Parser *p, struct node *arg)
{
	struct nodelist *ns     = nodelist(NULL),
	                *guards = NULL;

	struct node *clause  = NULL,
	            *select  = node(p->token, OSELECT),
	            *pattern = NULL;

	select->o.select.arg = arg;

	// The first `|` is optional
	if (p->tok == T_PIPE) next(p);

	int len = 0;

	do {
		clause = node(p->token, OCLAUSE);
		clause->o.clause.nguards = 0;

		if (arg) {
			pattern = parse_pattern(p);

			if (isnext(p, T_AND))
				guards = parse_guards(p, &clause->o.clause.nguards);

		} else {
			guards = parse_guards(p, &clause->o.clause.nguards);
		}
		expect(p, T_COLON);

		clause->o.clause.guards = guards;
		clause->o.clause.lval   = pattern;
		clause->o.clause.rval   = parse_block(p);

		append(ns, clause);
		len ++;

		if (p->tok == T_LF)
			next(p);

		if (p->tok == T_INDENT)
			next(p);

	} while (isnext(p, T_PIPE));

	select->o.select.nclauses = len;
	select->o.select.clauses = ns;

	return select;
}
static struct node *parse_select(Parser *p, struct node *arg)
{
	struct node *n;

	expect(p, T_QUESTION);

	if (p->tok == T_LF) {
		next(p);
		expect(p, T_INDENT);
		n = _parse_select(p, arg);
		expect(p, T_DEDENT);
	} else {
		n = _parse_select(p, arg);
	}
	return n;
}

/*
 * Parse a function application. Example:
 *
 *     fnord (42)
 *     F "hello."
 */
static struct node *parse_apply(Parser *p, struct node *t)
{
	struct node *n = node(p->token, OAPPLY), *e = NULL;

	e = parse_expression(p);

	n->o.apply.lval = t;
	n->o.apply.rval = e;

	return n;
}

/*
 * Parse a primary expression.
 * These are expressions followed by a T_LF. Example:
 *
 *     Str := "fnord"
 *
 * TODO: Find a way to obsolete this.
 */
static struct node *parse_primary(Parser *p)
{
	struct node *node = NULL;

	if ((node = parse_expression(p))) {
		end(p);
	} else {
		nextline(p);
	}
	return node;
}

/*
 * Parse module declaration. Example:
 *
 *     +/http (/) @ web
 */
static struct node *parse_decl(Parser *p)
{
	struct node *n, *module, *args = NULL, *alias = NULL;

	module = access(p, parse_module(p),  parse_access(p));

	/* TODO: Support parrens around parameter */
	switch (p->tok) {
		case T_IDENT: case T_SLASH: case T_PERIOD:
			args = parse_module(p); // Module parameter
		default: break;
	}

	if (p->tok == T_AT) { // Module alias
		next(p); // '@'
		alias = parse_atom(p);
	}
	n = node(p->token, ODECL);
	n->o.decl.module = module;
	n->o.decl.args = args;
	n->o.decl.alias = alias;

	return n;
}

/*
 * Parse top-level expression. These
 * are usually things like paths, modules
 * and module attributes, which live outside
 * of functions.
 */
static struct node *parse_toplevel(Parser *p)
{
	struct node *node = NULL;

	switch (p->tok) {
		case T_PERIOD:
		case T_IDENT:
			node = parse_path(p);
			break;
		case T_PLUS:
			node = parse_msgpath(p);
			break;
		case T_SLASH:
			node = parse_decl(p);
			break;
		default:
			error(p, ERR_DEFAULT);
	}
	return node;
}


/*
 * Parse from `p->source` until we reach the
 * `T_EOF` end-of-file token.
 */
Tree *parse(Parser *p)
{
	struct node    *node;

	next(p);

	while (p->tok != T_EOF) {
		if (p->tok == T_LF || p->tok == T_COMMENT) {
			next(p); continue;
		}
		if ((node = parse_toplevel(p))) {
			append(p->root->o.block.body, node);
			next(p);
		}
	}
	return p->tree;
}

