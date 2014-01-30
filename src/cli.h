#ifndef _CLI_H
#define _CLI_H

struct cli_program {
    const char *name;
    void (*func)(int argc, char **argv);
};

void cli(void);

#endif

