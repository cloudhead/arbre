/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 * (c) 1994-2011 Lua.org, PUC-Rio
 *
 * op.h
 *
 */
typedef struct {
    unsigned short   length;
    TYPE             type;
    char             value[];
} Constant;

extern const char         *OPCODE_STRINGS[];
extern const unsigned char OPCODE_MODES[];

typedef uint32_t OpArg;                 /* Instruction argument */
typedef uint32_t Instruction;           /* Byte-code instruction */
enum             IMode {ABC, AD, AJ};   /* Instruction mode/format */

/*
 * Size and position of opcode arguments
 */
#define OPSIZE        8
#define OPPOS_A       OPSIZE
#define OPPOS_C       OPPOS_A + OPSIZE
#define OPPOS_B       OPPOS_C + OPSIZE
#define OPPOS_D       OPPOS_C


/* Limits for opcode arguments */
#define OPMAX_A        0xff
#define OPMAX_C        0xff
#define OPMAX_B        0xff
#define OPMAX_D        0xffff
#define OPMAX_J        (OPMAX_D >> 1)  /* `J` is signed */

/*
 * Instruction field macros
 */
#define iOP(i)   ((OpCode)((i) & 0xff))
#define iA(i)    ((OpArg)((i) >> OPPOS_A) & 0xff)
#define iB(i)    ((OpArg)((i) >> OPPOS_B) & 0xff)
#define iC(i)    ((OpArg)((i) >> OPPOS_C) & 0xff)
#define iD(i)    ((OpArg)((i) >> OPPOS_D))
#define iJ(i)    ((ptrdiff_t)(iD(i) - OPMAX_J))

/*
 * Instruction creation macros
 */
#define iABC(o,a,b,c) (((Instruction)(o))              \
                     | ((Instruction)(a) << OPPOS_A)   \
                     | ((Instruction)(b) << OPPOS_B)   \
                     | ((Instruction)(c) << OPPOS_C))

#define iAD(o,a,d)    (((Instruction)(o))              \
                     | ((Instruction)(a)  << OPPOS_A)  \
                     | ((Instruction)(d)  << OPPOS_D))

#define iAJ(o,a,j)   iAD(o, a, ((int32_t)(j) + OPMAX_J))

/*
 * RK operands
 */

/* If this bit is set, the operand is a constant, else
 * a register. */
#define BITRK           (1 << (OPSIZE - 1))

/* Test if the RK bit is set */
#define ISK(x)          ((x) & BITRK)

/* Get the constant index from an RK operand */
#define INDEXK(r)       ((int)(r) & ~BITRK)
#define MAXINDEXRK      (BITRK - 1)

/* Create an RK operand from a constant index */
#define RKASK(x)        ((x) | BITRK)

#define NO_REG          OPMAX_A
#define NO_JMP          (~(int32_t)0)

/*
 * op-codes
 */
typedef enum {
    OP_INVALID,
    OP_MOVE,
    OP_LOADK,
    OP_ADD,
    OP_SUB,
    OP_GT,
    OP_EQ,
    OP_JUMP,
    OP_RETURN,
    OP_MATCH,
    OP_TUPLE,
    OP_SETTUPLE,
    OP_LIST,
    OP_CONS,
    OP_CALL,
    OP_TAILCALL,
    OP_SEND,
    OP_LAMBDA,
    OP_PATH
} OpCode;

/*
 * op-code modes
 */
enum OpArgMask {
    OPARG__,  /* argument is not used */
    OPARG_U,  /* argument is used */
    OPARG_R,  /* argument is a register or a jump offset */
    OPARG_K   /* argument is a constant or register/constant (RK) */
};

#define OPMODE(m)  ((enum IMode)(OPCODE_MODES[m] & 3))
#define BMODE(m)   ((enum OpArgMask)((OPCODE_MODES[m] >> 4) & 3))
#define CMODE(m)   ((enum OpArgMask)((OPCODE_MODES[m] >> 2) & 3))
#define AMODE(m)   (OPCODE_MODES[m] & (1 << 6))
#define TMODE(m)   (OPCODE_MODES[m] & (1 << 7))

void  oparg_pp   (int arg, int mode, int amode);
void  op_pp      (Instruction i);
