///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/comconn.h $
// $Author: TOML $
// $Date: 1996/10/10 14:31:27 $
// $Revision: 1.2 $
//
// Tools for building and using notification sink lists, aka connection points.
// Useful in allowing common & changeable implementation of COM holder/sink
// relationships.
//

#ifndef __COMCONN_H
#define __COMCONN_H

//////////////////////////////////////////////////////////////////////////////

#define COMAdvise(p, a, b)              COMCall2(p, Advise, a, b)
#define COMPriAdvise(p, a, b)           COMCall3(p, Advise, a, b, c)
#define COMUnadvise(p, a)               COMCall1(p, Unadvise, a)

//////////////////////////////////////////////////////////////////////////////

#ifndef COMTOOL_NO_CONNPT

//
// DECLARE_CONNECTION_POINT()
//
#define DECLARE_CONNECTION_POINT(INTERFACE) \
    \
    __DECLARE_CONNECTION_POINT_BASE(INTERFACE); \
    \
    STDMETHOD (Advise)(INTERFACE * pSink, DWORD * pCookie); \
    STDMETHOD (Unadvise)(DWORD)


//
// DECLARE_PRIORITIZED_CONNECTION_POINT()
//
#define DECLARE_PRIORITIZED_CONNECTION_POINT(INTERFACE) \
    \
    __DECLARE_CONNECTION_POINT_BASE(INTERFACE); \
    \
    STDMETHOD (Advise)(INTERFACE * pSink, int priority, DWORD * pCookie); \
    STDMETHOD (Unadvise)(DWORD)


//
// IMPLEMENT_PRIORITIZED_CONNECTION_POINT()
//
#define IMPLEMENT_PRIORITIZED_CONNECTION_POINT(CLASS, INTERFACE) \
    \
    STDMETHODIMP CLASS::Advise(INTERFACE * pSink, int priority, DWORD * pCookie) \
        { \
        if (m_Sinks.Insert(pSink, priority, pCookie)) \
            return NOERROR; \
        return E_FAIL; \
        } \
    \
    STDMETHODIMP CLASS::Unadvise(DWORD cookie) \
        { \
        if (m_Sinks.Remove(cookie)) \
            { \
            return NOERROR; \
            } \
        return E_FAIL; \
        }


//
// IMPLEMENT_CONNECTION_POINT()
//
#define IMPLEMENT_CONNECTION_POINT(CLASS, INTERFACE) \
    \
    STDMETHODIMP CLASS::Advise(INTERFACE * pSink, DWORD * pCookie) \
        { \
        if (m_Sinks.Insert(pSink, pCookie)) \
            return NOERROR; \
        return E_FAIL; \
        } \
    \
    STDMETHODIMP CLASS::Unadvise(DWORD cookie) \
        { \
        if (m_Sinks.Remove(cookie)) \
            { \
            return NOERROR; \
            } \
        return E_FAIL; \
        }


//
// CONNECTION_POINT_ITERATE()
//

#define CONNECTION_POINT_ITERATE() \
    \
    tConnSetHandle handle; \
    for (tSinkClass * pSink = m_Sinks.GetFirst(handle); pSink; pSink = m_Sinks.GetNext(handle))


//
// CONNECTION_POINT_ITERATE_VER()
//
#define CONNECTION_POINT_ITERATOR_VER(VER) \
    \
    CONNECTION_POINT_ITERATE() \
        if (pSink->GetVersion() < VER) \
            ; \
        else


///////////////////////////////////////

//
// __DECLARE_CONNECTION_POINT_BASE()
//
#define __DECLARE_CONNECTION_POINT_BASE(INTERFACE) \
    \
    typedef INTERFACE tSinkClass; \
    cCOMConnectionSet<INTERFACE> m_Sinks;

#ifdef __cplusplus

#include <pdynarr.h>

//
// STRUCT: sCOMConnection.  Stores the connection
//

int LGAPI GetCOMConnectionPriority(const void *);

struct sCOMConnection
    {
    IUnknown *  pSink;
    int         priority;
    };

DECLARE_HANDLE(tConnSetHandle);

//
// CLASS: cCOMConnectionSetBase.  Stores the connections, provides iteration
//
class cCOMConnectionSetBase
    {
public:
    cCOMConnectionSetBase()
      : m_fSorted(TRUE),
        m_fPrioritized(FALSE)
        {
        }

    unsigned Count()
        {
        return m_Connections.Size();
        }

protected:

    BOOL Search(IUnknown *);
    BOOL Insert(IUnknown *, DWORD * pCookie);
    BOOL Insert(IUnknown *, int priority, DWORD * pCookie);
    BOOL Remove(DWORD cookie);

    IUnknown * GetFirst(tConnSetHandle &);
    IUnknown * GetNext(tConnSetHandle &);

private:
    cPriDynArray<sCOMConnection, GetCOMConnectionPriority> m_Connections;

    int m_fSorted;
    int m_fPrioritized;
    };

//
// TEMPLATE: cCOMConnectionSet.  Templatizes cCOMConnectionSetBase
//

template <class T>
class cCOMConnectionSet : public cCOMConnectionSetBase
    {
public:
    BOOL Search(T * p)
        {
        return cCOMConnectionSetBase::Search(p);
        }

    BOOL Insert(T * p, DWORD * pCookie)
        {
        return cCOMConnectionSetBase::Insert(p, pCookie);
        }

    BOOL Insert(T * p, int priority, DWORD * pCookie)
        {
        return cCOMConnectionSetBase::Insert(p, priority, pCookie);
        }

    BOOL Remove(DWORD cookie)
        {
        return cCOMConnectionSetBase::Remove(cookie);
        }

    T * GetFirst(tConnSetHandle & h)
        {
        return (T *)cCOMConnectionSetBase::GetFirst(h);
        }

    T * GetNext(tConnSetHandle & h)
        {
        return (T *)cCOMConnectionSetBase::GetNext(h);
        }

    };

#endif


#endif /* !defined(COMTOOL_NO_CONNPT) */

//////////////////////////////////////////////////////////////////////////////

#endif /* !__COMCONN_H */
