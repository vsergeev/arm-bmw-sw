#ifndef _UCLI_H
#define _UCLI_H

#define CLI_MAX_ARGC    5

struct cli_program {
    const char *name;
    void (*func)(int argc, char **argv);
};

void cli(void);

#endif

