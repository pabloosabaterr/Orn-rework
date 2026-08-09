#include "parse-options.h"
#include "log.h"

#include <stddef.h>
#include <string.h>

int parse_options(int argc, char **argv, struct option *options)
{
    int i;

    for (i = 1; i < argc; i++) {
        const struct option *opt = NULL;

        if (*argv[i] != '-' || argv[i][1] == '\0' || (argv[i][1] == '-' && argv[i][2] == '\0'))
            break;

        if (argv[i][1] == '-') {
            const char *name = argv[i] + 2;
            for (const struct option *o = options; o->type != OPT_END; o++) {
                if (o->name && !strcmp(name, o->name)) {
                    opt = o;
                    break;
                }
            }
        } else if (argv[i][2] == '\0') {
            char abbrev = argv[i][1];
            for (const struct option *o = options; o->type != OPT_END; o++) {
                if (o->abbrev && o->abbrev == abbrev) {
                    opt = o;
                    break;
                }
            }
        }

        if (!opt)
            die("unknown option '%s'", argv[i]);

        switch (opt->type) {
        case OPT_BOOL:
            *(unsigned *)opt->val = 1;
            break;
        case OPT_END:
            break;
        }
    }

    return i;
}
