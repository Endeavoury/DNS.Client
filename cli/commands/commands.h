#ifndef RATOS_CLI_COMMANDS_H
#define RATOS_CLI_COMMANDS_H

#include <stddef.h>

typedef int (*ratos_cli_command_handler)(int argc, char **argv);

typedef struct ratos_cli_command {
    const char *name;
    const char *summary;
    ratos_cli_command_handler handler;
} ratos_cli_command;

int ratos_cli_dns(int argc, char **argv);

const ratos_cli_command *ratos_cli_commands(size_t *count);

#endif
