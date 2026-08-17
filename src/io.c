#include "io.h"
#include "log.h"
#include "wrapper.h"
#include <stdio.h>

char *read_file(const char *path)
{
    FILE *f;
    long len;
    char *buf;
    size_t n;

    f = fopen(path, "r");
    if (!f)
        die("cannot open '%s'", path);

    if (fseek(f, 0, SEEK_END))
        die("cannot seek '%s'", path);
    len = ftell(f);
    if (len < 0)
        die("cannot know size of '%s'", path);
    if (fseek(f, 0, SEEK_SET))
        die("cannot seek '%s'", path);

    buf = xmalloc(len + 1);
    n = fread(buf, 1, len, f);
    buf[n] = '\0';
    fclose(f);

    return buf;
}
