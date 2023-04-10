///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dlist.cpp $
// $Author: TOML $
// $Date: 1998/02/23 10:46:15 $
// $Revision: 1.4 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#include <lg.h>
#include <dlist.h>

extern const char * g_pszDlistInsertError = "List node improperly added to multiple lists";

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDListNodeBase
//

///////////////////////////////////////
//
// Scan backwards to the head of the list, returning a zero-based count
//
long cDListNodeBase::CountBackwards()
{
    long i = 0;
    for (cDListNodeBase *p = GetPrevious(); p; p = p->GetPrevious())
        i++;
    return i;
}


///////////////////////////////////////////////////////////////////////////////
