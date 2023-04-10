///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/templexp.h $
// $Author: TOML $
// $Date: 1996/05/01 20:27:32 $
// $Revision: 1.1 $
//
// Small tool to assist in forcing template expansion
//

#ifndef __TEMPLEXP_H
#define __TEMPLEXP_H

#define TEMPLATE_EXPAND1(templ, p1) \
    void __ ## templ ## __ ## p1 ## __ ## Expand() { templ<p1> a; } class __SEMICOLON_REQUIRED

#define TEMPLATE_EXPAND2(templ, p1, p2) \
    void __ ## templ ## __ ## p1 ## p2 ## __ ## Expand() { templ<p1, p2> a; } class __SEMICOLON_REQUIRED

#define TEMPLATE_EXPAND3(templ, p1, p2, p3) \
    void __ ## templ ## __ ## p1 ## p2 ## p3 ## __ ## Expand() { templ<p1, p2, p3> a; } class __SEMICOLON_REQUIRED

#define TEMPLATE_EXPAND4(templ, p1, p2, p3, p4) \
    void __ ## templ ## __ ## p1 ## p2 ## p3 ## p4 ## __ ## Expand() { templ<p1, p2, p3, p4> a; } class __SEMICOLON_REQUIRED

#define TEMPLATE_EXPAND5(templ, p1, p2, p3, p4, p5) \
    void __ ## templ ## __ ## p1 ## p2 ## p3 ## p4 ## p5 ## __ ## Expand() { templ<p1, p2, p3, p4, p5> a; } class __SEMICOLON_REQUIRED

#endif /* !__TEMPLEXP_H */
