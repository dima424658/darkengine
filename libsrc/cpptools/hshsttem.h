///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/hshsttem.h $
// $Author: TOML $
// $Date: 1998/02/11 09:30:34 $
// $Revision: 1.7 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __HSHSTTEM_H
#define __HSHSTTEM_H

#include <hshstimp.h>


template <class NODE, class KEY, class FUNCLASS/*=cHashFunctions*/>
void cHashSet<NODE, KEY, FUNCLASS>::DestroyAll()
{
    if (!m_nItems)
        return;

    for (register unsigned i = 0; i < m_nPts; i++)
    {
        sHashSetChunk * pNext;
        for (sHashSetChunk * p = m_Table[i]; p; p = pNext)
        {
            pNext = p->pNext;
#ifndef HASHSET_TEMPLATE_NO_DELETE
            delete (NODE)(p->node);
#endif
            delete p;
        }

        m_Table[i] = 0;
    }

    m_nItems = 0;
}


#if 0
//
// Write out a hash node to a stream
//
template <class NODE, class KEY, class FUNCLASS=cHashFunctions>
BOOL cSerialHashSet<NODE, KEY, FUNCLASS>::NodeToStream(cOStore &OStore, const tHashSetNode *node) const
{
    return ((NODE *)node)->ToStream(OStore);
}


//
// Read in a hash node from a stream
//
template <class NODE, class KEY, class FUNCLASS=cHashFunctions>
tHashSetNode *cSerialHashSet<NODE, KEY, FUNCLASS>::NodeFromStream(cIStore &IStore)
{
    NODE *node = new NODE;
    node->FromStream(IStore);
    return node;
}

#endif

#endif /* !__HSHSTTEM_H */
