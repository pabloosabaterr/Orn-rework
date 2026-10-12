#ifndef PARSE_OPTIONS_H
#define PARSE_OPTIONS_H

enum option_type {
    OPT_BOOL,
    OPT_END,
};

struct option {
    char abbrev;
    const char *name;
    enum option_type type;
    void *val;
};

#define OPT_BOOL(abbrev, name, var) { abbrev, name, OPT_BOOL, var }

#define OPT_END() { 0, NULL, OPT_END, NULL }

/*
 * Parse the cmd args.
 * Returns the nr of parsed args before hiting a OPT_END.
 */
int parse_options(int argc, char **argv, struct option *options);

#endif
