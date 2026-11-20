#pragma once

/* Windows' initguid.h turns subsequent GUID declarations into definitions. */
#define INITGUID
#ifdef DEFINE_GUID
#undef DEFINE_GUID
#endif

#ifdef _MSC_VER
#define DARK_GUID_SELECTANY __declspec(selectany)
#else
#define DARK_GUID_SELECTANY
#endif

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name; \
    const GUID DARK_GUID_SELECTANY name = { l, w1, w2, \
        { b1, b2, b3, b4, b5, b6, b7, b8 } }
