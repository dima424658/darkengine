///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/comconn.cpp $
// $Author: TOML $
// $Date: 1996/10/10 14:31:26 $
// $Revision: 1.2 $
//

#include <lg.h>
#include <comtools.h>
#include <comconn.h>

///////////////////////////////////////////////////////////////////////////////

int LGAPI GetCOMConnectionPriority(const void * p)
    {
    return ((sCOMConnection *)p)->priority;
    }

///////////////////////////////////////////////////////////////////////////////

BOOL cCOMConnectionSetBase::Search(IUnknown * p)
    {
    // Because these lists never get very long, and because
    // insertion/removal is an uncommon event relative
    // to iteration, we simply use a linear search
    for (index_t i = 0; i < m_Connections.Size(); i++)
        {
        if (m_Connections[i].pSink == p)
            return TRUE;
        }
    return FALSE;
    }

///////////////////////////////////////

BOOL cCOMConnectionSetBase::Insert(IUnknown * p, DWORD * pCookie)
    {
    if (Search(p))
        {
        CriticalMsg("Multiple connection point advises are not permitted");
        return FALSE;
        }

    sCOMConnection connection;
    connection.pSink = p;
    connection.priority = 0;
    m_Connections.Append(connection);

    p->AddRef();
    *pCookie = DWORD(p);

    return TRUE;
    }

///////////////////////////////////////

BOOL cCOMConnectionSetBase::Insert(IUnknown * p, int priority, DWORD * pCookie)
    {
    if (Search(p))
        {
        CriticalMsg("Multiple connection point advises are not permitted");
        return FALSE;
        }

    m_fPrioritized = TRUE;
    m_fSorted = FALSE;

    sCOMConnection connection;
    connection.pSink = p;
    connection.priority = priority;
    m_Connections.Append(connection);

    p->AddRef();
    *pCookie = DWORD(p);

    return TRUE;
    }

///////////////////////////////////////

BOOL cCOMConnectionSetBase::Remove(DWORD cookie)
    {
    IUnknown * p = (IUnknown *) cookie;
    for (index_t i = 0; i < m_Connections.Size(); i++)
        {
        if (m_Connections[i].pSink == p)
            {
            m_Connections.DeleteItem(i);
            p->Release();
            return TRUE;
            }
        }
    CriticalMsg("Unknown notification sink");
    return FALSE;
    }

///////////////////////////////////////

IUnknown * cCOMConnectionSetBase::GetFirst(tConnSetHandle & hIndex)
    {
    if (m_Connections.Size())
        {
        if (m_fPrioritized && !m_fSorted)
            {
            m_Connections.Sort();
            }
        hIndex = (tConnSetHandle)0;
        return m_Connections[(index_t)0].pSink;
        }
    return NULL;
    }

///////////////////////////////////////

IUnknown * cCOMConnectionSetBase::GetNext(tConnSetHandle & hIndex)
    {
    const index_t index = (index_t)hIndex + 1;
    if (index < m_Connections.Size())
        {
        hIndex = (tConnSetHandle)index;
        return m_Connections[index].pSink;
        }
    return NULL;
    }

///////////////////////////////////////
