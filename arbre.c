/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * arbre.c
 *
 *   entry-point for CLI
 */
#include  <stdio.h>

#include  "arbre.h"
#include  "scanner.h"
#include  "parser.h"
#include  "op.h"
#include  "runtime.h"
#include  "vm.h"
#include  "generator.h"
#include  "command.h"

int main(int argv, char *argc[])
{
    Command *c = command(argv, argc);

    command_exec(c);
    command_free(c);

    return 0;
}
