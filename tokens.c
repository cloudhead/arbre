/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * tokens.c
 *
 */
#include "tokens.h"

const char *TOKEN_STRINGS[] = {
   [T_EOF]         = "EOF",
   [T_COMMENT]     = "comment",
   [T_ILLEGAL]     = "ILLEGAL",
   [T_INDENT]      = "indent",
   [T_DEDENT]      = "dedent",

   [T_SPACE]       = "space",
   [T_LF]          = "'\\n'",

   [T_UNDER]       = "_",
   [T_IDENT]       = "ident",
   [T_INT]         = "int",
   [T_FLOAT]       = "float",
   [T_CHAR]        = "char",
   [T_STRING]      = "string",
   [T_ATOM]        = "atom",

   [T_PLUS]        = "'+'",
   [T_MINUS]       = "'-'",
   [T_STAR]        = "'*'",
   [T_SLASH]       = "'/'",
   [T_PERCENT]     = "'%'",

   [T_AND]         = "'&'",
   [T_OR]          = "'|'",
   [T_XOR]         = "'^'",

   [T_DEFINE]      = ":=",
   [T_EQ]          = "=",

   [T_LAND]        = "LAND",
   [T_LOR]         = "LOR",

   [T_LARROW]      = "'<-'",
   [T_RARROW]      = "'->'",
   [T_LRARROW]     = "'<->'",
   [T_LEQARROW]    = "'<='",
   [T_REQARROW]    = "'=>'",
   [T_LREQARROW]   = "'<=>'",
   [T_LDARROW]     = "'<<'",
   [T_RDARROW]     = "'>>'",

   [T_LEQ]         = "'=='",
   [T_LNOT_EQ]     = "'/='",
   [T_LT]          = "'<'",
   [T_GT]          = "'>'",

   [T_ELLIPSIS]    = "'...'",
   [T_BANG]        = "'!'",
   [T_QUESTION]    = "'?'",
   [T_PIPE]        = "'|'",

   [T_LPAREN]      = "'('",
   [T_RPAREN]      = "')'",

   [T_LBRACK]      = "'['",
   [T_RBRACK]      = "']'",

   [T_LBRACE]      = "'}'",
   [T_RBRACE]      = "'{'",

   [T_COMMA]       = "','",
   [T_PERIOD]      = "'.'",

   [T_SEMICOLON]   = "';'",
   [T_COLON]       = "':'"
};
