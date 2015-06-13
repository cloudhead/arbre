/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * parser.h
 *
 */
typedef struct {
	Tree            *tree;      /* AST */
	struct node     *root;      /* tree->root */
	struct node     *block;     /* Current block */
	Token           *token;     /* Current token */
	TOKEN            tok;       /* token->tok */
	size_t           pos;       /* token->pos */
	char            *src;       /* token->src */
	int              errors;    /* Error count */
	struct scanner  *scanner;
	void           (*ontoken)(Token *);
} Parser;

Parser *parser(struct source *src);
Tree   *parse(Parser *p);
void    pparser(Parser *p);
