#ifndef ATTRS_H
#define ATTRS_H

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_FMT(f, a) __attribute__((format(printf, (f), (a))))
#define NORETURN __attribute__((noreturn))
#else
#define PRINTF_FMT(f, a)
#define NORETURN
#endif

#define UNUSED __attribute__((unused))

#endif /* ATTRS_H */
