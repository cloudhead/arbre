/*
 * arbre
 * hash.h
 */
#define  FNV_PRIME  16777619
#define  FNV_BASIS  2166136261

unsigned long hash(const char *input, unsigned long len);

