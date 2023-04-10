///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/memtest.cpp $
// $Author: TOML $
// $Date: 1997/07/10 17:12:22 $
// $Revision: 1.2 $
//

#include <stdio.h>
#include <stdlib.h>
#include <allocovr.h>

int main()
{
    void * p = malloc(10);
    printf("allocated at 0x%x\n", p);
    free(p);
    p = new int[10];
    printf("allocated at 0x%x\n", p);
    delete p;
    return 0;
}

