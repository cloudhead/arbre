/*
 * arbre
 * hash.c
 *
 * Fowler-Noll-Vo 32-bit hash function (FNV-1a)
 *
 */
#include  "hash.h"

unsigned long hash(const char *input, unsigned long len)
{
	unsigned long hash = FNV_BASIS;

	for (int i = 0; i < len; i ++) {
		hash  ^=  input[i];
		hash  *=  FNV_PRIME;
	}
	return hash;
}
