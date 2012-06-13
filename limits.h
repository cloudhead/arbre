/*
 * arbre
 *
 * (c) 2011 Alexis Sellier.
 *
 * limits.h
 *
 */
# if defined(__linux__)
#   include  <linux/limits.h>
# else
#   include  <limits.h>
# endif

# if !defined(PATH_MAX)
#   define PATH_MAX 4096
# endif

