///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dlisttem.h $
// $Author: TOML $
// $Date: 1997/06/24 18:07:07 $
// $Revision: 1.3 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __DLISTTEM_H
#define __DLISTTEM_H

//include "storintr.h"

#ifdef DEBUG_DLIST_TEM
    #pragma message "DEBUG_DLIST_TEM enabled"
    #define DListTemDebugStr(s) DebugStr(s)
#else
    #define DListTemDebugStr(s)
#endif

///////////////////////////////////////////////////////////////////////////////

template <class NODE, int ID>
void cDList<NODE, ID>::DestroyAll()
{
    while (GetFirst())
        delete Remove(GetFirst());
}

#if 0
///////////////////////////////////////
//
// Write a list to a stream
//
template <class NODE, int ID>
BOOL cSerialDList<NODE, ID>::ToStream(cOStore &OStore) const
{
    typedef cDListNode<NODE, ID> cNode;

    NODE *p = GetFirst();

    if (!OStore.WriteHeader("DLIST"))
        return FALSE;

    long i = 0;
    while (p)
    {
        if (!OStore.To(i++))
            return FALSE;

        if (!p->ToStream(OStore))
            return FALSE;

        p = p->cNode::GetNext();
    }

    if (!OStore.To(-1L))
        return FALSE;

    return OStore.WriteTrailer();
}


///////////////////////////////////////
//
// Fill a list from a stream
//
template <class NODE, int ID>
BOOL cSerialDList<NODE, ID>::FromStream(cIStore &IStore)
{
    if (!IStore.ReadHeader("DLIST"))
        return FALSE;

    long i = 0;
    long j;
    for (;;)
    {
        if (!IStore.From(j))
    {
            DListTemDebugStr("cSerialDList<NODE, ID>::FromStream() -> FALSE: no/empty list");
            return FALSE;
    }

        if (j != i++)
            break;

        NODE *p = new NODE;
        p->FromStream(IStore);
        Append(*p);
    }

    if (j != -1L)
    {
        DListTemDebugStr(cFmtStr("cSerialDList<NODE, ID>::FromStream() failed: %ld != -1L, i==%ld", j, i));
        return IStore.Fail();
    }

    return IStore.ReadTrailer();
}
#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__DLISTTEM_H */
