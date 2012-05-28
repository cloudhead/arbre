/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * parser.h
 *
 */
typedef struct {
    Tree       *tree;      /* AST */
    Node       *root;      /* tree->root */
    Node       *block;     /* Current block */
    Token      *token;     /* Current token */
    TOKEN       tok;       /* token->tok */
    size_t      pos;       /* token->pos */
    char       *src;       /* token->src */
    int         errors;    /* Error count */
    Scanner    *scanner;
    void      (*ontoken)(Token *);
} Parser;

Parser *parser(Source *src);
Tree   *parse(Parser *p);
void    pparser(Parser *p);
