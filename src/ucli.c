#include <string.h>

#include <io/uart.h>

#include "ucli.h"

static void cmd_dispatch(int argc, char **argv) {
    int i;

    if (argc < 1)
        return;

    for (i = 0; CLI_Programs[i].name != NULL; i++) {
        if (strcmp(argv[0], CLI_Programs[i].name) == 0) {
            CLI_Programs[i].func(argc, argv);
            break;
        }
    }

    if (CLI_Programs[i].name == NULL) {
        uart_puts("unknown program \"");
        uart_puts(argv[0]);
        uart_puts("\"\n");
        uart_puts("please type \"help\" for help\n");
    }
}

static int cmd_tokenize(char *s, char **argv, unsigned int max_argc) {
    char *token;
    unsigned int i;

    token = strtok(s, " ");
    for (i = 0; token != NULL && i < max_argc; i++) {
        argv[i] = token;
        token = strtok(NULL, " ");
    }

    return i;
}

void ucli_server(void) {
    char buf[UCLI_BUFFER_SIZE];
    char *argv[UCLI_MAX_ARGC];
    int argc;

    while (1) {
        uart_puts("\x1b[1;36marm-bmw\x1b[0m > ");
        uart_gets(buf, sizeof(buf), true);
        argc = cmd_tokenize(buf, argv, UCLI_MAX_ARGC);
        cmd_dispatch(argc, argv);
    }
}

