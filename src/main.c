#include "compiler.h"
#include "orn.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct compiler_context cc = COMPILER_CONTEXT_INIT;
    int ret = 0;

    compiler_init(&cc);
    ret = orn_cmd_parse(argc, argv, &cc);
    compiler_free(&cc);
    return ret;
}
