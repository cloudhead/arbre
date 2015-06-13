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
	struct source  *source;
	PathEntry      *path;
	PathEntry     **paths;
	struct module  *module;
	struct node    *block; /* Current block */
	int             env; /* Index of root module */
	int             head;
	char           *out;
	unsigned        pathsn; /* TODO: Rename to `npaths` */
	uint8_t         slot;
} Generator;

Generator *generator (Tree *tree, struct source *source);
void       generate  (Generator *g, FILE *fp);

