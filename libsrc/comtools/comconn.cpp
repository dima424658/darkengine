///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/comconn.cpp $
// $Author: TOML $
// $Date: 1996/10/10 14:31:26 $
// $Revision: 1.2 $
//

#include <lg.h>
#include <comtools.h>
#include <comconn.h>
#include <unordered_map>

static std::unordered_map<DWORD, IUnknown *> &ConnectionCookies()
    {
    // The aggregate is torn down from an atexit handler.  Keep this tiny
    // process registry alive through that teardown instead of depending on
    // cross-translation-unit static destruction order.
    static std::unordered_map<DWORD, IUnknown *> * cookies =
        new std::unordered_map<DWORD, IUnknown *>;
    return *cookies;
    }

static DWORD AllocateConnectionCookie(IUnknown * p)
    {
    static DWORD nextCookie = 1;
    std::unordered_map<DWORD, IUnknown *> &cookies = ConnectionCookies();
    DWORD cookie;
    do
        {
        cookie = nextCookie++;
        }
    while (cookie == 0 || cookies.find(cookie) != cookies.end());
    cookies[cookie] = p;
    return cookie;
    }

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
    for (int i = 0; i < m_Connections.Size(); i++)
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
    *pCookie = AllocateConnectionCookie(p);

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
    *pCookie = AllocateConnectionCookie(p);

    return TRUE;
    }

///////////////////////////////////////

BOOL cCOMConnectionSetBase::Remove(DWORD cookie)
    {
    std::unordered_map<DWORD, IUnknown *> &cookies = ConnectionCookies();
    std::unordered_map<DWORD, IUnknown *>::iterator found = cookies.find(cookie);
    if (found == cookies.end())
        {
        CriticalMsg("Unknown notification sink");
        return FALSE;
        }
    IUnknown * p = found->second;
    for (int i = 0; i < m_Connections.Size(); i++)
        {
        if (m_Connections[i].pSink == p)
            {
            m_Connections.DeleteItem(i);
            cookies.erase(found);
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
    const index_t index = (index_t)(uintptr_t)hIndex + 1;
    if (index < (index_t)m_Connections.Size())
        {
        hIndex = (tConnSetHandle)(uintptr_t)index;
        return m_Connections[index].pSink;
        }
    return NULL;
    }

///////////////////////////////////////
