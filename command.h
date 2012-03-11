/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * command.h
 */
typedef enum {
    CMD_BUILD = 1,
    CMD_RUN,
    CMD_TEST
} CommandType;

struct Command {
    CommandType  type;
    int          options;
    char       **inputs;
    int          inputc;
    int          argv;
    char       **argc;
    int        (*f)(struct Command *);
    FILE        *fp;
};

typedef  struct Command  Command;

typedef enum {
    CMDOPT_SYNTAX = 1,
    CMDOPT_PRE    = 1 << 1,
    CMDOPT_AST    = 1 << 2,
    CMDOPT_V      = 1 << 3
} CommandOption;

Command  *command(int argv, char *argc[]);
int       command_exec(Command *);
void      command_free(Command *);
