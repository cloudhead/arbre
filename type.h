/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * type.h
 *
 * TODO: Is this even relevant?
 *
 */
struct Type {
    char         *name;
    Node         *node;
    unsigned int  id;
};

typedef  struct Type  Type;

Type   *type      (char *name, Node *n, int id);
Type   *nil       (void);
TYPE    typeid    (Node *n);

int     typeidtos (int t, char *buff);
int     typetos   (struct Type *t, char *buff);
void    typeid_pp (unsigned int id);
