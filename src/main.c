#include "driver/compiler.h"
#include "driver/orn.h"


int main(int argc, char **argv)
{
    struct compiler_context cc = COMPILER_CONTEXT_INIT;
    int ret = 0;

    compiler_init(&cc);
    ret = orn_cmd_parse(argc, argv, &cc);
    compiler_free(&cc);
    return ret;
}
