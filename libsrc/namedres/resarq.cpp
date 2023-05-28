///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/resarq.cpp $
// $Author: TOML $
// $Date: 1997/10/16 13:23:05 $
// $Revision: 1.19 $
//
// @TBD (toml 08-29-96): need to add more verbose error handling if there is
// no ARQ and these functions are used
//
// @TBD (toml 09-12-96): Need to analyze how the very broad thread locking
// in the resource library might negatively impact performance.  In
// particular, one can't queue a request while another is being served -- silly.
//

#ifdef _WIN32
#include <windows.h>
#endif

#include <comtools.h>
#include <appagg.h>
#include <lg.h>

#include <hashset.h>

#include <arqapi.h>

#include <res.h>
#include <res_.h>
#include <resapi.h>
#include <resman.h>
#include <resthred.h>

#include <hshsttem.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResARQFulfiller
//

class cResARQFulfiller : public cCTUnaggregated<IAsyncReadFulfiller, &IID_IAsyncReadFulfiller, kCTU_NoSelfDelete>
{
public:
    cResARQFulfiller();
    ~cResARQFulfiller();

    ///////////////////////////////////

    void    Init();
    void    Term();

    ///////////////////////////////////

    bool    Lock(IRes* pRes, int priority);
    bool    Extract(IRes* pRes, int priority, void* pBuf, long bufSize);
    bool    Preload(IRes* pRes);
    bool    IsFulfilled(IRes* pRes);
    HRESULT Kill(IRes* pRes);
    HRESULT GetResult(IRes* pRes, void** ppResult);

    void ClearPreload(cResourceTypeData* pId);
    bool IsAsynchronous();

    ///////////////////////////////////

    STDMETHOD(DoFulfill)(const sARQRequest* pRequest, sARQResult* pResult);
    STDMETHOD(DoKill)(const sARQRequest* pRequest, BOOL fDiscard);

    ///////////////////////////////////

private:

    struct sResRequest;
    class cResControlTable : public cPtrHashSet<sResRequest*>
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

        sResRequest(cResControlTable& owningControlTable,
            cResourceTypeData* id, IRes* pRes, int priority,
            sResRequest::eRequestKind kind, void* pBuf, int bufSize)
            :
            pControl{ nullptr },
            id{ id },
            pResource{ pRes },
            priority{ priority },
            kind{ kind },
            pBuf{ pBuf },
            bufSize{ bufSize },
            nRequests{ 0 },
            m_OwningControlTable{ owningControlTable },
            satisfyingPreload{ 0 }
        {
            pResource->AddRef();
            m_OwningControlTable.Insert(this);
        }

        ~sResRequest()
        {
            Verify(m_OwningControlTable.Remove(this));
            pResource->Release();
            if (pControl)
                pControl->Release();
            memset(this, 0xfe, sizeof(sResRequest));
        }

        ///////////////////////////////

        IAsyncReadControl* pControl;
        cResourceTypeData* id;
        IRes* pResource;
        int                 priority;
        eRequestKind        kind;
        void* pBuf;
        long                bufSize;
        uint                nRequests;

        cResControlTable& m_OwningControlTable;

        bool                satisfyingPreload;
    };

    ///////////////////////////////////

    bool    QueueRequest(IRes* pRes, int priority, sResRequest::eRequestKind kind, void* pBuf = nullptr, long bufSize = 0);
    HRESULT SatisfyRequest(sResRequest* pResRequest, void** ppResult);

    ///////////////////////////////////

    IAsyncReadQueue* m_pAsyncReadQueue;
    cResControlTable        m_Controls;
};

///////////////////////////////////////////////////////////////////////////////

static cResARQFulfiller g_ResARQFulfiller;
static cResMan* g_pResMan;

///////////////////////////////////////

bool cResARQFulfiller::Lock(IRes* pRes, int priority)
{
    return QueueRequest(pRes, priority, sResRequest::kLock);
}

///////////////////////////////////////

bool cResARQFulfiller::Extract(IRes* pRes, int priority, void* pBuf, long bufSize)
{
    return QueueRequest(pRes, priority, sResRequest::kExtract, pBuf, bufSize);
}

///////////////////////////////////////

bool cResARQFulfiller::Preload(IRes* pRes)
{
    return QueueRequest(pRes, kPriorityLowest, sResRequest::kPreload);
}

///////////////////////////////////////

bool cResARQFulfiller::IsAsynchronous()
{
    return m_pAsyncReadQueue != nullptr;
}

cResARQFulfiller::cResARQFulfiller()
    : m_pAsyncReadQueue{ nullptr }, m_Controls{}
{
}

///////////////////////////////////////

cResARQFulfiller::~cResARQFulfiller()
{
}

///////////////////////////////////////

void cResARQFulfiller::Init()
{
    #ifdef _WIN32
    if (GetPrivateProfileInt("Res", "ARQ", TRUE, "lg.ini"))
    #endif
        m_pAsyncReadQueue = AppGetObj(IAsyncReadQueue);
}

///////////////////////////////////////

void cResARQFulfiller::Term()
{
    SafeRelease(m_pAsyncReadQueue);
    // @TBD (toml 08-29-96): clean up any left over controls
}

///////////////////////////////////////

bool cResARQFulfiller::IsFulfilled(IRes* pRes)
{
    auto* pResourceTypeData = g_pResMan->GetResourceTypeData(pRes);
    auto* pResRequest = m_Controls.Search(pResourceTypeData);

    AssertMsg1(pResRequest, "Resource 0x%x was never queued", pResourceTypeData->GetName());

    // If we're actually operating asynchronously...
    if (pResRequest && IsAsynchronous())
        // ... get the status
        return pResRequest->pControl->IsFulfilled();

    // Otherwise, just promise sucess
    return true;
}

///////////////////////////////////////
// @TBD (toml 03-21-97): should change arq kill to return s_false for this

HRESULT cResARQFulfiller::Kill(IRes* pRes)
{
    ResThreadLock();

    auto* pResourceTypeData = g_pResMan->GetResourceTypeData(pRes);
    auto* pResRequest = m_Controls.Search(pResourceTypeData);

    if (pResRequest)
    {
        HRESULT retVal = S_OK;

        pResRequest->nRequests--;
        if (!pResRequest->nRequests)
        {
            if (IsAsynchronous())
            {
                ResThreadUnlock();
                retVal = pResRequest->pControl->Kill(TRUE);
                ResThreadLock();

                if (retVal == E_FAIL && pResRequest->kind == sResRequest::kLock)
                    g_pResMan->UnlockResource(pResRequest->pResource);
            }

            delete pResRequest;
        }
        ResThreadUnlock();
        return retVal;
    }

    ResThreadUnlock();
    return E_FAIL;
}

///////////////////////////////////////

HRESULT cResARQFulfiller::GetResult(IRes* pRes, void ** ppResult)
{
    ResThreadLock();
    auto* pResourceTypeData = g_pResMan->GetResourceTypeData(pRes);
    auto* pResRequest = m_Controls.Search(pResourceTypeData);

    if (pResRequest)
    {
        HRESULT retVal;

        // If we're actually operating asynchronously...
        if (IsAsynchronous())
        {
            // ... get the result
            sARQResult result;

            ResThreadUnlock();

            // Synchronously ensure it's been loaded
            pResRequest->pControl->Fulfill();

            // Retrieve it
            retVal = pResRequest->pControl->GetResult(&result);

            ResThreadLock();

            // If we had multiple requests for a lock, up the lock count as
            // the ARQ will no-op fulfill requests on already fulfilled items
            if (pResRequest->nRequests > 1 &&
                pResRequest->kind == sResRequest::kLock) // @Note (toml 04-08-97): this should be cleaned up
            {
                g_pResMan->LockResource(pResRequest->pResource);
            }

            *ppResult = result.buffer;
        }
        else
        {
            // Else synchronously satisfy the request
            retVal = SatisfyRequest(pResRequest, ppResult);
        }

        AssertMsg(pResRequest->nRequests, "Expected request count to be non-zero");
        pResRequest->nRequests--;
        if (!pResRequest->nRequests)
            delete pResRequest;

        ResThreadUnlock();
        return retVal;
    }

    CriticalMsg("Tried to get the result of an unknown async request.");

    *ppResult = nullptr;
    ResThreadUnlock();
    return E_FAIL;
}

///////////////////////////////////////

void cResARQFulfiller::ClearPreload(cResourceTypeData* pId)
{
    cAutoResThreadLock lock;

    if (IsAsynchronous())
    {
        auto* pResRequest = m_Controls.Search(pId);

        if (pResRequest && pResRequest->kind == sResRequest::kPreload)
            Kill(pResRequest->pResource);
    }
}

///////////////////////////////////////

STDMETHODIMP cResARQFulfiller::DoFulfill(const sARQRequest * pRequest, sARQResult * pResult)
{
    cAutoResThreadLock lock;
    sResRequest * pResRequest = (sResRequest * ) pRequest->dwData[0];

    AssertMsg(m_Controls.Search(&pResRequest->id), "Fuck!");

    pResult->flags       = 0;
    pResult->streamIndex = 0;
    pResult->length      = 0;
    pResult->result      = SatisfyRequest(pResRequest, &pResult->buffer);

    if(pResRequest->kind == sResRequest::eRequestKind::kPreload)
    {
        --pResRequest->nRequests;
        if (pResRequest->nRequests == 0)
            delete pResRequest;
    }

    return pResult->result;
}

///////////////////////////////////////

STDMETHODIMP cResARQFulfiller::DoKill(const sARQRequest * /*pRequest*/, BOOL /*fDiscard*/)
{
    return S_OK;
}

///////////////////////////////////////

bool cResARQFulfiller::QueueRequest(IRes* pRes, int priority,
                                    sResRequest::eRequestKind kind,
                                    void * pBuf, long bufSize)
{
// @TBD (toml 09-12-96): on lock, if resource is already in memory, could we just to another
// lock here & circumvent the ARQ? hmm...
    cAutoResThreadLock lock;
    auto* pResourceTypeData = g_pResMan->GetResourceTypeData(pRes);
    auto* pResRequest = m_Controls.Search(pResourceTypeData);

    // If we have a request for this Id already...
    if (pResRequest)
    {
        // Make sure the new one is compatible...
        switch (pResRequest->kind)
        {
            // If the existing request is a lock...
            //
            case sResRequest::kLock:
                // The new one can't be an extract...
                if (kind == sResRequest::kExtract)
                {
                    CriticalMsg("Can't mix async Lock/Extract");
                    return false;
                }

                // A lock supercedes a preload
                if (kind == sResRequest::kPreload)
                    return true;

                // If the new one is of equal or lower priority...
                if (ComparePriorities(pResRequest->priority, priority) >= 0)
                {
                    // ... we're ok
                    ++pResRequest->nRequests;
                    return true;
                }

                // Otherwise, the request is boosting priority
                // @TBD (toml 09-11-96): resubmit here, matching request counter + 1
                Warning(("ResAsync: Cannot boost request priority (not yet imlemented)"));
                return true;

            // If there's an extract...
            //
            case sResRequest::kExtract:
                // We can't resolve it, because there's no distinguishing for ResAsyncGetResult()
                CriticalMsg("Can't mix async Extract with any other async resource operation");
                return false;

            // If what we have is a preload...
            //
            case sResRequest::kPreload:

                // If the new one is preload we're fine...
                if (kind == sResRequest::kPreload)
                    return true;

                // @TBD (toml 09-11-96): resubmit here as a lock if lock, blow up if extract
                // @TBD (toml 09-11-96): We can't resolve it right now, because we're lazy!
                CriticalMsg("Oof! Don't know how to turn a preload into something else right now!");
                return false;

            default:
                CriticalMsg("Unknown async request kind!");
        }
        return false;
    }

    // If it's a preload request...
    if (kind == sResRequest::kPreload)
    {
        // ... and it's already locked, there's nothing to do but make sure it's at the back of the LRU...
        // ... or if we're not operating asynchronously just do it...
        if (pResourceTypeData->m_pData != nullptr || !IsAsynchronous())
        {
            g_pResMan->LockResource(pResRequest->pResource);
            g_pResMan->UnlockResource(pResRequest->pResource);
            return true;
        }
    }

    // Reaching here, we have a fresh request to handle...
    // AssertMsg(!pResRequest, "Expected request to be new");
    pResRequest = new sResRequest{ m_Controls, pResourceTypeData, pRes, priority, kind, pBuf, bufSize };

    // If we're actually operating asynchronously...
    if (IsAsynchronous())
    {
        // ...queue it
        sARQRequest request =
        {
            this,                                // Fulfiller
            0,                                   // Queue
            priority,                            // Priority
            {
                (DWORD) pResRequest,             // Custom data
                0, 0, 0
            },
            "Resource"                           // Trace name
        };

        if (m_pAsyncReadQueue->QueueRequest(&request, &pResRequest->pControl) != S_OK)
        {
            delete pResRequest;
            return false;
        }
    }

    // When not asynchronous, leave the request hanging around in the table to be picked
    // up later when GetResult() is called
    pResRequest->nRequests++;
    return true;
}

///////////////////////////////////////

HRESULT cResARQFulfiller::SatisfyRequest(sResRequest * pResRequest, void ** ppResult)
{
    void * pResult;

    AssertMsg(m_Controls.Search(&pResRequest->id), "Fuck!");

    switch (pResRequest->kind)
    {
        case sResRequest::kLock:
            AssertMsg(ppResult, "Must have a destination for ResLock()");
            pResult = g_pResMan->LockResource(pResRequest->pResource);
            break;

        case sResRequest::kExtract:
            pResult = g_pResMan->ExtractResource(pResRequest->pResource, pResRequest->pBuf);
            break;

        case sResRequest::kPreload:
        {
            pResRequest->satisfyingPreload = true;
            pResult = pResRequest->pResource->PreLoad();
            pResRequest->satisfyingPreload = false;
            break;
        }

        default:
            CriticalMsg("Unknown async request kind!");
    }

    if (ppResult)
        *ppResult = pResult;


    return (pResult) ? S_OK : E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResARQFulfiller::cResControlTable
//

tHashSetKey cResARQFulfiller::cResControlTable::GetKey(tHashSetNode node) const
{
    return (tHashSetKey)(&(((sResRequest*)(node))->id));
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResARQ
//

void cResManARQ::Init(cResMan* pManager)
{
    g_ResARQFulfiller.Init();
    g_pResMan = pManager;
}

void cResManARQ::Term()
{
    g_pResMan = nullptr;
    g_ResARQFulfiller.Term();
}

void cResManARQ::ClearPreload(cResourceTypeData* id)
{
    g_ResARQFulfiller.ClearPreload(id);
}

int cResManARQ::Lock(IRes* pRes, int priority)
{
    return g_ResARQFulfiller.Lock(pRes, priority);
}

int cResManARQ::Extract(IRes* pRes, int priority, void* buf, long bufSize)
{
    return g_ResARQFulfiller.Extract(pRes, priority, buf, bufSize);
}

int cResManARQ::Preload(IRes* pRes)
{
    return g_ResARQFulfiller.Preload(pRes);
}

int cResManARQ::IsFulfilled(IRes* pRes)
{
    return g_ResARQFulfiller.Preload(pRes);
}

long cResManARQ::Kill(IRes* pRes)
{
    return g_ResARQFulfiller.Kill(pRes);
}

long cResManARQ::GetResult(IRes* pRes, void** ppResult)
{
    return g_ResARQFulfiller.GetResult(pRes, ppResult);
}