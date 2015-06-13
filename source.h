/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * source.h
 *
 */
struct source {
    const char  *path;
    char        *data;
    size_t       size;
    int          lines;
    int          line;
    int          col;
    char       **lineps;
    int          lineps_size;
};

struct source *source(const char *);
void           source_free(struct source *);
void           source_addline(struct source *, size_t);
void           source_seek(struct source *s, size_t pos);
void           source_rewind(struct source *s);
