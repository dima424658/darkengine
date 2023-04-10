#ifndef __G2MAKEWR_H
#define __G2MAKEWR_H
#ifdef _WIN32
EXTERN void g2_make_writable(void);
#else
#define g2_make_writable()
#endif
#endif /* !__G2MAKEWR_H */
