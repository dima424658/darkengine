///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/resarq.h $
// $Author: TOML $
// $Date: 1997/08/12 12:19:14 $
// $Revision: 1.9 $
//

#ifndef __RESARQ_H
#define __RESARQ_H

EXTERN void LGAPI ResARQClearPreload(Id id);

#if defined(__ARQAPI_H) && defined(__cplusplus)

#include <pool.h>
#include <hashset.h>

#include <pshpack4.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResARQ
//

class cResARQFulfiller : public IAsyncReadFulfiller
    {
public:
    cResARQFulfiller();
    ~cResARQFulfiller();

    ///////////////////////////////////

    void    Init();
    void    Term();

    ///////////////////////////////////

    BOOL    Lock(Id id, int priority);
    BOOL    Extract(Id id, int priority, void *buf, long bufSize);
    BOOL    Preload(Id id);
    BOOL    IsFulfilled(Id id);
    HRESULT Kill(Id id);
    HRESULT GetResult(Id id, void **);

    void ClearPreload(Id id);
    BOOL IsAsynchronous();

    ///////////////////////////////////

    STDMETHOD (DoFulfill)(const sARQRequest *, sARQResult *);
    STDMETHOD (DoKill)(const sARQRequest *, BOOL fDiscard);

    DECLARE_UNAGGREGATABLE();

    ///////////////////////////////////

private:

    struct sResRequest;
    class cResControlTable : public cHashSet<sResRequest *, Id *, cHashFunctions>
        {
    private:
        virtual tHashSetKey GetKey(tHashSetNode node) const;
        };

    ///////////////////////////////////
    //
    // Asynchronous resource requests are tracked using a hash table of
    // sResRequest structures
    //

    struct sResRequest
        {
        enum eRequestKind
            {
            kLock,
            kExtract,
            kPreload
            };

        ///////////////////////////////

        sResRequest(cResControlTable &  owningControlTable,
                    Id id, int priority, eRequestKind kind,
                    void * pBuf = NULL, long bufSize = 0)
          : pControl(NULL),
            id(id),
            priority(priority),
            pBuf(pBuf),
            bufSize(bufSize),
            nRequests(0),
            kind(kind),
            m_OwningControlTable(owningControlTable)
            {
            m_OwningControlTable.Insert(this);
            }

        ~sResRequest()
            {
            Verify(m_OwningControlTable.Remove(this));
            if (pControl)
                pControl->Release();
            memset(this, 0xfe, sizeof(sResRequest));
            }

        ///////////////////////////////

        IAsyncReadControl * pControl;
        Id                  id;
        int                 priority;
        eRequestKind        kind;
        void *              pBuf;
        long                bufSize;
        unsigned            nRequests;

        cResControlTable &  m_OwningControlTable;

        ///////////////////////////////

        DECLARE_POOL();

        };

    ///////////////////////////////////

    BOOL    QueueRequest(Id id, int priority, sResRequest::eRequestKind, void *buf = NULL, long bufSize = 0);
    HRESULT SatisfyRequest(sResRequest *, void **);

    ///////////////////////////////////

    IAsyncReadQueue *       m_pAsyncReadQueue;
    cResControlTable        m_Controls;

    BOOL                    m_fSatisfyingPreload;

    };

///////////////////////////////////////

inline BOOL cResARQFulfiller::Lock(Id id, int priority)
    {
    return QueueRequest(id, priority, sResRequest::kLock);
    }

///////////////////////////////////////

inline BOOL cResARQFulfiller::Extract(Id id, int priority, void *buf, long bufSize)
    {
    return QueueRequest(id, priority, sResRequest::kExtract, buf, bufSize);
    }

///////////////////////////////////////

inline BOOL cResARQFulfiller::Preload(Id id)
    {
    return QueueRequest(id, kPriorityLowest, sResRequest::kPreload);
    }

///////////////////////////////////////

inline BOOL cResARQFulfiller::IsAsynchronous()
    {
    return !!m_pAsyncReadQueue;
    }

///////////////////////////////////////////////////////////////////////////////

#include <poppack.h>

#endif


#endif /* !__RESARQ_H */
