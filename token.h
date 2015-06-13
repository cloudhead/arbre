/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * token.h
 *
 *   Token data-type
 *
 */
typedef struct {
    TOKEN          tok;      /* Token type (see tokens.h for list of tokens types) */
    struct source *source;   /* struct source file the token belongs to */
    size_t         pos;      /* Position of the token in the source file */
    char          *src;      /* Scanned string literal for this token */
} Token;

char   *tokentos(Token *t, char *str, bool);
Token  *token(TOKEN tok, struct source *source, size_t pos, char *src);
