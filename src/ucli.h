#ifndef _UCLI_H
#define _UCLI_H

struct ucli_program {
    const char *name;
    void (*func)(int argc, char **argv);
};

extern const struct ucli_program CLI_Programs[];

#define UCLI_BUFFER_SIZE    128
#define UCLI_MAX_ARGC       5

void ucli_server(void);

#endif

