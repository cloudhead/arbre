/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * tokens.h
 *
 */
extern const char *TOKEN_STRINGS[];

typedef enum {
	/* Special */
	T_EOF,
	T_COMMENT,        // -- I'm a comment
	T_ILLEGAL,

	/* Indentation tokens */
	T_INDENT,
	T_DEDENT,

	/* Whitespace */
	T_SPACE,          // ' '
	T_LF,             // '\n'

	T_UNDER,
	T_AT,

	/* Operands */
	T_IDENT,          // Fnord
	T_ATOM,           // fnord
	T_INT,            // 12345
	T_FLOAT,          // 123.45
	T_CHAR,           // 'a'
	T_STRING,         // "fnord"

	/* Operators */
	T_PLUS,           // +
	T_MINUS,          // -
	T_STAR,           // *
	T_SLASH,          // /
	T_DIV,            // ÷
	T_BSLASH,         // '\'
	T_PERCENT,        // %

	T_AND,            // &
	T_OR,             // |
	T_XOR,            // ^

	/* Definition/declaration */
	T_DEFINE,         // :=

	/* Logical operators */
	T_LAND,           // &&
	T_LOR,            // ||
	T_PIPE,           // |
	T_QUESTION,       // ?
	T_LEQ,
	T_LNOT_EQ,

	/* Arrows */
	T_LARROW,         // <-
	T_RARROW,         // ->
	T_LRARROW,        // <->
	T_LEQARROW,       // <=
	T_REQARROW,       // =>
	T_LREQARROW,      // <=>
	T_LDARROW,        // <<
	T_RDARROW,        // >>

	/* Comparison operators */
	T_EQ,             // =
	T_NOT_EQ,         // ≠
	T_LT,             // <
	T_GT,             // >

	/* Other operators */
	T_ELLIPSIS,       // ...
	T_BANG,           // !

	/* Delimiters */
	T_LPAREN,         // (
	T_RPAREN,         // )

	T_LBRACK,         // [
	T_RBRACK,         // ]

	T_LBRACE,         // {
	T_RBRACE,         // }

	T_COMMA,          // ,
	T_PERIOD,         // .

	T_SEMICOLON,      // ;
	T_COLON           // :
} TOKEN;

