/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * generator.h
 *
 */
typedef struct {
    Tree           *tree;
    Source         *source;
    PathEntry      *path;
    PathEntry     **paths;
    Module         *module;
    int             env; /* Index of root module */
    int             head;
    char           *out;
    unsigned        pathsn; /* TODO: Rename to `npaths` */
    uint8_t         slot;
} Generator;

Generator *generator (Tree *tree, Source *source);
void       generate  (Generator *g, FILE *fp);

