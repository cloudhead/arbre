/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * io.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "arbre.h"
#include "io.h"

/*
 * Arbre-specific vsnprintf function
 *
 * It accepts sequences such as "#t" for printing Types,
 * and "#n" for printing Nodes, as well as some of the basic
 * printf sequences such as "%s" and "%d".
 */
int io_vsnprintf(char *s, size_t n, const char *fmt, va_list ap)
{
    int    index   = 0;  /* Current index in `s` */
    size_t rem     = n;  /* Remaining characters (n - index) */

    for (int i = 0; fmt[i] && rem > 0; i++) {
        if (fmt[i] == '#') {
            switch (fmt[++i]) {
                case 'n':
                    index += snprintf(s + index, rem, "%s", va_arg(ap, Node*)->src);
                    break;
                // TODO: Do we need this?
                // case 't':
                //     index += typetos(va_arg(ap, struct Type*), s + index);
                //     break;
                case '#':
                    s[++index] = '#';
                    break;
                default:
                    break;
            }
        } else if (fmt[i] == '%') {
            switch (fmt[++i]) {
                case 's':
                    index += snprintf(s + index, rem, "%s", va_arg(ap, char*));
                    break;
                case 'd':
                    index += snprintf(s + index, rem, "%d", va_arg(ap, int));
                    break;
                case '%':
                    s[++index] = '%';
                    break;
            }
        } else {
            s[index++] = fmt[i];
        }
        rem = n - index;
    }
    s[index] = '\0';
    return index;
}

