/*
 * arbre
 *
 * (c) 2011, Alexis Sellier
 *
 * command.c
 *
 * CLI commands
 *
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <inttypes.h>
#include  <assert.h>
#include  <errno.h>
#include  <sys/stat.h>

char *strdup(const char *);

#include  "arbre.h"
#include  "scanner.h"
#include  "parser.h"
#include  "op.h"
#include  "runtime.h"
#include  "vm.h"
#include  "generator.h"
#include  "command.h"
#include  "error.h"
#include  "reduce.h"

static int   command_build(Command *cmd);
static int   command_run(Command *cmd);
static int   command_test(Command *cmd);
static bool  command_parseopt(Command *cmd, char opt, char *arg);
static void  command_parselopt(Command *cmd, char *arg);

static uint8_t *freadbin(FILE *fp, const char *path);

struct {
    CommandOption  type;
    const char    *name;
} CMD_OPTIONS[] = {
    {CMDOPT_SYNTAX,  "syntax"},
    {CMDOPT_AST,     "ast"},
    {CMDOPT_PRE,     "pre"},
    {CMDOPT_V,       "verbose"},
    {0, NULL}
};

struct {
    CommandType  type;
    const char  *name;
    int        (*f)(Command *c);
} CMD_MAP[] = {
    {CMD_BUILD,    "build",   command_build},
    {CMD_RUN,      "run",     command_run},
    {CMD_TEST,     "test",    command_test},
 // {CMD_VERSION,  "version", command_version},
    {0, NULL, NULL}
};

const char *CMD_USAGE =
    "usage: arbre <command> [options]\n"
    "\n"
    "commands:\n"
    "    build      compile modules and dependencies\n"
    "    clean      remove .arb.bin files\n"
    "    run        compile and run\n"
    "\n"
    "options:\n"
    "    -v         verbose\n"
    "    --help     help\n"
    "    --version  print version and exit\n"
    "    --ast      print the AST\n"
    "    --pre      only run the pre-processor phase\n"
    "    --syntax   only run the syntax checking phase\n";

/*
 * Command allocator/initialzer
 */
Command *command(int argv, char *argc[])
{
    Command *c = malloc(sizeof(*c));
             c->type    = 0;
             c->options = 0;
             c->inputs  = malloc(sizeof(char*) * argv);
             c->inputc  = 0;
             c->argv    = argv;
             c->argc    = argc;
             c->output  = NULL;
             c->fp      = NULL;
             c->f       = NULL;
    return   c;
}

/*
 * Command de-allocator
 */
void command_free(Command *c)
{
    if (c->fp != NULL)
        fclose(c->fp);
    free(c->inputs);
    free(c);
}

/*
 * Execute a command.
 */
int command_exec(Command *cmd)
{
    /* No command passed, print usage */
    if (cmd->argv == 1) {
        puts(CMD_USAGE);
        exit(0);
    }

    /* Retrieve command type */
    for (int i = 0; CMD_MAP[i].type; i++) {
        if (strcmp(cmd->argc[1], CMD_MAP[i].name) == 0) {
            cmd->type = CMD_MAP[i].type;
            cmd->f    = CMD_MAP[i].f;

            /* Parse command options */
            for (int i = 2; i < cmd->argv; i++) {
                if (cmd->argc[i][0] == '-') {
                    switch (cmd->argc[i][1]) {
                        case '-':
                            command_parselopt(cmd, &cmd->argc[i][2]);
                            break;
                        case '\0':
                            // TODO: Read from STDIN
                            break;
                        default: {
                            char opt = cmd->argc[i][1],
                                *arg = NULL;

                            if (i < cmd->argv - 1 && cmd->argc[i + 1][0] != '-') {
                                arg = cmd->argc[++i];
                            }
                            command_parseopt(cmd, opt, arg);
                        }
                    }
                } else {
                    cmd->inputs[cmd->inputc++] = cmd->argc[i];
                }
            }
            /* Run command */
            return cmd->f(cmd);
        }
    }
    error(1, 0, "unknown command `%s`", cmd->argc[1]);

    return -1;
}

/*
 * Parse short option. Example:
 *
 *     -a
 */
static bool command_parseopt(Command *cmd, char opt, char *arg)
{
    switch (opt) {
        case 'o':
            cmd->output = arg;
            return true;
        default:
            break;
    }
    return false;
}

/*
 * Parse long option. Example:
 *
 *     --help
 */
static void command_parselopt(Command *cmd, char *arg)
{
    for (int i = 0; CMD_OPTIONS[i].type; i++) {
        if (strcmp(CMD_OPTIONS[i].name, arg) == 0) {
            cmd->options |= CMD_OPTIONS[i].type;
        }
    }
}

/*
 * 'test' command
 */
static int command_test(Command *cmd)
{
    return 0;
}

/*
 * 'run' command
 */
static int command_run(Command *c)
{
    TValue *ret;

    if (c->inputc > 1)
        error(1, 0, "more than one input file was given");

    if (c->inputc == 0)
        puts("usage: arbre run <module>"), exit(0);

    // TODO: Use unique path
    c->output = "/tmp/arbre-build.bin";

    command_build(c);

    const char *path = c->inputs[0];

    char *module = strdup(path);

    for (int i = 0; i < strlen(module); i++) {
        if (module[i] == '.') {
            module[i] = '\0';
            break;
        }
    }

    if (c->options & CMDOPT_SYNTAX)
        return 0;

    uint8_t *code = freadbin(c->fp, module);

    VM *v = vm();
    vm_open(v, module, code);

    ret = vm_run(v, module, "main");

    assert(ret->t == TYPE_NUMBER);
    return ret->v.number;
}

#ifdef DEBUG
static void ontoken(Token *t)
{
    char buff[128];
    tokentos(t, buff, true);
    puts(buff);
}
#endif

/*
 * 'build' command
 */
static int command_build(Command *c)
{
    // TODO: If no files were specified, build all arbre
    // files in the current dir.
    if (c->inputc == 0)
        puts("usage: arbre build <files...>"), exit(0);

    for (int i = 0; i < c->inputc; i++) {
        char *path = c->inputs[i];

        Source  *src = source(path);
        Parser  *p   = parser(src);

        #ifdef DEBUG
        p->ontoken = ontoken;
        #endif

        Tree *tree = parse(p);

        #ifdef DEBUG
        putchar('\n');
        #endif

        if (c->options & CMDOPT_AST)
            pp_tree(tree), putchar('\n');

        if (p->errors > 0)
            return 1;

        reduce(tree);

        if (c->options & CMDOPT_SYNTAX)
            break;

        Generator *g = generator(tree, src);

        mkdir(".arbre",     0755);
        mkdir(".arbre/bin", 0755);

        char out[strlen(path) + 4 + 1];
        sprintf(out, ".arbre/bin/%s.bin", path);

        if (! (c->fp = fopen(c->output ? c->output : out, "w+"))) {
            error(1, errno, "couldn't create file %s for writing", out);
        }
        generate(g, c->fp);
    }
    return 0;
}

/*
 * Read an arb.bin file
 */
static uint8_t *freadbin(FILE *fp, const char *module)
{
    uint8_t    *buffer;
    size_t      size;

    fseek(fp, 0L, SEEK_END);

    size = ftell(fp);
    rewind(fp);

    buffer = malloc(size);

    fread(buffer, sizeof(uint8_t), size, fp);

    return buffer;
}
