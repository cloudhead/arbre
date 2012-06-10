/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * op.c
 *
 *     op-codes
 *
 */
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>

#include "value.h"
#include "op.h"

const char *OPCODE_STRINGS[] = {
    [OP_MOVE]     = "move",
    [OP_LOADK]    = "loadk",
    [OP_JUMP]     = "jump",
    [OP_RETURN]   = "return",
    [OP_ADD]      = "add",
    [OP_SUB]      = "sub",
    [OP_GT]       = "gt",
    [OP_MATCH]    = "match",
    [OP_TUPLE]    = "tuple",
    [OP_SETTUPLE] = "settuple",
    [OP_LIST]     = "list",
    [OP_CONS]     = "cons",
    [OP_CALL]     = "call",
    [OP_TAILCALL] = "tcall",
    [OP_LAMBDA]   = "lambda",
    [OP_PATH]     = "path"
};

#define MODE(t, a, b, c, m) (((t) << 7) | ((a) << 6) | ((b) << 4) | ((c) << 2) | (m))

const unsigned char OPCODE_MODES[] = {
    /* opcode            T   A     B       C      mode */
    [OP_MOVE]     = MODE(0,  1, OPARG_R,       0, ABC),
    [OP_LOADK]    = MODE(0,  1, OPARG_K,       0, AD ),
    [OP_JUMP]     = MODE(0,  0, OPARG_U,       0, AJ ), // NOTE: Lua uses OPARG_R for B
    [OP_RETURN]   = MODE(0,  1,       0,       0, ABC),
    [OP_ADD]      = MODE(0,  1, OPARG_K, OPARG_K, ABC),
    [OP_SUB]      = MODE(0,  1, OPARG_K, OPARG_K, ABC),
    [OP_GT]       = MODE(1,  0, OPARG_K, OPARG_K, ABC),
    [OP_MATCH]    = MODE(1,  1, OPARG_K, OPARG_K, ABC),
    [OP_TUPLE]    = MODE(0,  1, OPARG_U,       0, ABC),
    [OP_SETTUPLE] = MODE(0,  1, OPARG_U, OPARG_K, ABC),
    [OP_LIST]     = MODE(0,  1, OPARG__, OPARG__, ABC),
    [OP_CONS]     = MODE(0,  1, OPARG_R, OPARG_K, ABC),
    [OP_CALL]     = MODE(0,  1, OPARG_K, OPARG_K, ABC),
    [OP_TAILCALL] = MODE(0,  1, OPARG_U, OPARG_R, ABC), // TODO: Don't use C
    [OP_SEND]     = MODE(0,  0, OPARG_R, OPARG_K, ABC),
    [OP_LAMBDA]   = MODE(0,  1, OPARG_U,       0, AD ),
    [OP_PATH]     = MODE(0,  1, OPARG_K, OPARG_K, ABC)
};

#undef MODE

void op_pp(Instruction i)
{
    OpCode o = OP(i);

    int a = A(i),
        b = B(i),
        c = C(i),
        d = D(i),
        j = J(i);

    printf("%-9s\t", OPCODE_STRINGS[o]);

    oparg_pp(a, -1, AMODE(o));
    putchar('\t');

    switch (OPMODE(o)) {
        case ABC:
            oparg_pp(b, BMODE(o), 0);
            putchar('\t');
            oparg_pp(c, CMODE(o), 0);
            break;
        case AD:
            oparg_pp(d, BMODE(o), 0);
            putchar('\t');
            break;
        case AJ:
            oparg_pp(j, BMODE(o), 0);
            putchar('\t');
            break;
    }
}

void oparg_pp(int arg, int mode, int amode)
{
    if (mode == -1 && !amode) {
        printf(" %-4d", arg);
        return;
    }
    if (mode) {
        if (mode == OPARG_U) {
            printf(" %-4d", arg);
        } else {
            if (ISK(arg) && mode == OPARG_K) {
                printf("k%-4d", (INDEXK(arg)));
            } else {
                printf("r%-4d", arg);
            }
        }
    }
}

