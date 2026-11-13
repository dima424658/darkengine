#ifndef DARK_EDITOR_COMPAT_H
#define DARK_EDITOR_COMPAT_H

/* Windows headers supplied these comparison macros to legacy editor C files. */
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#endif
