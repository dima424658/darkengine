#include "vernum.h"

#include <cstdio>
#include <string>

int VersionNumsCompare(const VersionNum* v1, const VersionNum* v2) // reversed
{
    if (v1->major != v2->major)
        return v1->major - v2->major;

    if (v1->minor != v2->minor)
        return v1->minor - v2->minor;

    return 0;
}

const char* VersionNum2String(const VersionNum* v) // reversed
{
    static char buf[256];

    snprintf(buf, std::size(buf), "%d.%02d", v->major, v->minor);
    
    return buf;
}