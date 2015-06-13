/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * report.h
 *
 */
#define  ERR_DEFAULT             "unexpected expression"
#define  ERR_UNDEFINED           "'%s' is not defined"
#define  ERR_UNDECLARED_IDENT    "use of undeclared identifier '%s'"
#define  ERR_UNINITIALIZED_IDENT "'%s' is uninitialized"
#define  ERR_REDEFINITION        "redefinition of '%s'"
#define  ERR_TYPE_INIT           "initializing '#n : #t' with an expression of type '#t'"
#define  ERR_TUPLE_ARITY         "tuple arity mismatch: %d vs %d members"
#define  ERR_STREAM_RETURN       "returning '#n : #t' from a stream with return type '#t'"
#define  ERR_STREAM_SEND_TYPE    "sending '#n : #t' to stream of input type '#t'"

enum  REPORT_TYPE  { REPORT_ERROR,  REPORT_WARNING,  REPORT_NOTE };

void  nreportf(enum REPORT_TYPE, struct node *, const char *, ...);
void vnreportf(enum REPORT_TYPE, struct node *, const char *, va_list ap);

#ifdef REPORT_PARSER
void  preportf(enum REPORT_TYPE, Parser *, const char *, ...);
void vpreportf(enum REPORT_TYPE, Parser *, const char *, va_list ap);
#endif
