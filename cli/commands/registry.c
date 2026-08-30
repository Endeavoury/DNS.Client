#include "commands.h"

const ratos_cli_command *ratos_cli_commands(size_t *count) {
    static const ratos_cli_command commands[] = {
        {"dns", "Query the Domain Name System", ratos_cli_dns},
    };

    if (count != NULL) {
        *count = sizeof(commands) / sizeof(commands[0]);
    }
    return commands;
}
