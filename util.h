/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * util.h
 *
 */
char *escape(char *str);
char *escape_r(char *str, char **saveptr);
int   escape_n(char *str, char *esc, int len);

void  ttyprint(char *esc, const char *str);
