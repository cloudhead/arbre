/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * source.h
 *
 */
typedef struct {
    const char  *path;
    char        *data;
    size_t       size;
    int          lines;
    int          line;
    int          col;
    char       **lineps;
    int          lineps_size;
} Source;

Source *source(const char *);
void    source_free(Source *);
void    source_addline(Source *, size_t);
void    source_seek(Source *s, size_t pos);
void    source_rewind(Source *s);
