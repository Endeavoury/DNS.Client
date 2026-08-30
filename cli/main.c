#include <stdio.h>
#include <string.h>
#include "commands/commands.h"
#include "ratatoskr/version.h"

static void root_help(FILE *stream) {
    size_t count;
    const ratos_cli_command *commands = ratos_cli_commands(&count);
    size_t i;

    fprintf(stream,
        "Ratatoskr networking toolkit\n\n"
        "Usage:\n  ratos <command> [arguments]\n\n"
        "Commands:\n");
    for (i = 0; i < count; ++i) {
        fprintf(stream, "  %-6s %s\n", commands[i].name, commands[i].summary);
    }
    fprintf(stream, "\nRun 'ratos <command> --help' for command options.\n");
}

int main(int argc, char **argv) {
    size_t count;
    const ratos_cli_command *commands;
    size_t i;

    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        root_help(stdout); return 0;
    }
    if (strcmp(argv[1], "--version") == 0) {
        printf("ratos %u.%u.%u\n", ratos_version_major(), ratos_version_minor(), ratos_version_patch()); return 0;
    }
    commands = ratos_cli_commands(&count);
    for (i = 0; i < count; ++i) {
        if (strcmp(argv[1], commands[i].name) == 0) {
            return commands[i].handler(argc - 2, argv + 2);
        }
    }
    fprintf(stderr, "ratos: unknown command '%s'\n", argv[1]);
    root_help(stderr); return 2;
}
