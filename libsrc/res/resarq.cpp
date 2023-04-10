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
#include <resarq.h>
#include <resthred.h>

#include <hshsttem.h>

///////////////////////////////////////////////////////////////////////////////
//
// cResARQFulfiller request pool
//

IMPLEMENT_POOL(cResARQFulfiller::sResRequest);

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////

static cResARQFulfiller g_ResARQFulfiller;

///////////////////////////////////////

void LGAPI ResARQInit()
{
    g_ResARQFulfiller.Init();
    g_pfnResARQClearPreloadFunc = ResARQClearPreload;
}

///////////////////////////////////////

void LGAPI ResARQClearPreload(Id id)
{
    g_ResARQFulfiller.ClearPreload(id);
}

///////////////////////////////////////

void LGAPI ResARQTerm()
{
    g_ResARQFulfiller.Term();
    // @TBD (toml 09-11-96): Remove all requests from table!
}

///////////////////////////////////////

BOOL LGAPI ResAsyncLock(Id id, int priority)
{
    ValidateRes(id);

    return g_ResARQFulfiller.Lock(id, priority);
}

///////////////////////////////////////

BOOL LGAPI ResAsyncExtract(Id id, int priority, void *buf, long bufSize)
{
    ValidateRes(id);

    return g_ResARQFulfiller.Extract(id, priority, buf, bufSize);
}

///////////////////////////////////////

BOOL LGAPI ResAsyncPreload(Id id)
{
    ValidateRes(id);

    return g_ResARQFulfiller.Preload(id);
}

///////////////////////////////////////

BOOL LGAPI ResAsyncIsFulfilled (Id id)
{
    ValidateRes(id);

    return g_ResARQFulfiller.IsFulfilled(id);
}

///////////////////////////////////////

HRESULT LGAPI ResAsyncKill(Id id)
{
    ValidateRes(id);

    return g_ResARQFulfiller.Kill(id);
}

///////////////////////////////////////

HRESULT LGAPI ResAsyncGetResult(Id id, void ** ppResult)
{
    ValidateRes(id);

    return g_ResARQFulfiller.GetResult(id, ppResult);
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResARQFulfiller
//

IMPLEMENT_UNAGGREGATABLE_NO_FINAL_RELEASE(cResARQFulfiller, IAsyncReadFulfiller);

///////////////////////////////////////

cResARQFulfiller::cResARQFulfiller()
  : m_pAsyncReadQueue(NULL),
    m_fSatisfyingPreload(FALSE)
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

BOOL cResARQFulfiller::IsFulfilled(Id id)
{
    sResRequest * pResRequest = m_Controls.Search(&id);

    AssertMsg1(pResRequest, "Resource 0x%x was never queued", id);

    // If we're actually operating asynchronously...
    if (pResRequest && IsAsynchronous())
        // ... get the status
        return pResRequest->pControl->IsFulfilled();

    // Otherwise, just promise sucess
    return TRUE;
}

///////////////////////////////////////
// @TBD (toml 03-21-97): should change arq kill to return s_false for this

HRESULT cResARQFulfiller::Kill(Id id)
{
    ResThreadLock();
    sResRequest * pResRequest = m_Controls.Search(&id);

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
                    ResUnlock(pResRequest->id);
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

HRESULT cResARQFulfiller::GetResult(Id id, void ** ppResult)
{
    ResThreadLock();
    sResRequest * pResRequest = m_Controls.Search(&id);

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
                ResLock(id);
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

    *ppResult = NULL;
    ResThreadUnlock();
    return E_FAIL;
}

///////////////////////////////////////

void cResARQFulfiller::ClearPreload(Id id)
{
    cAutoResThreadLock lock;

    if (IsAsynchronous())
    {
        sResRequest * pResRequest = m_Controls.Search(&id);

        if (pResRequest && pResRequest->kind == sResRequest::kPreload)
        {
            Kill(id);
        }
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

    return pResult->result;
}

///////////////////////////////////////

STDMETHODIMP cResARQFulfiller::DoKill(const sARQRequest *, BOOL /*fDiscard*/)
{
    return S_OK;
}

///////////////////////////////////////

BOOL cResARQFulfiller::QueueRequest(Id id, int priority,
                                    sResRequest::eRequestKind kind,
                                    void * pBuf, long bufSize)
{
// @TBD (toml 09-12-96): on lock, if resource is already in memory, could we just to another
// lock here & circumvent the ARQ? hmm...
    cAutoResThreadLock lock;
    sResRequest * pResRequest = m_Controls.Search(&id);

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
                    return FALSE;
                }

                // A lock supercedes a preload
                if (kind == sResRequest::kPreload)
                    return TRUE;

                // If the new one is of equal or lower priority...
                if (ComparePriorities(pResRequest->priority, priority) >= 0)
                {
                    // ... we're ok
                    pResRequest->nRequests++;
                    return TRUE;
                }

                // Otherwise, the request is boosting priority
                // @TBD (toml 09-11-96): resubmit here, matching request counter + 1
                Warning(("ResAsync: Cannot boost request priority (not yet imlemented)"));
                return TRUE;

            // If there's an extract...
            //
            case sResRequest::kExtract:
                // We can't resolve it, because there's no distinguishing for ResAsyncGetResult()
                CriticalMsg("Can't mix async Extract with any other async resource operation");
                return FALSE;

            // If what we have is a preload...
            //
            case sResRequest::kPreload:

                // If the new one is preload we're fine...
                if (kind == sResRequest::kPreload)
                {
                    return TRUE;
                }

                // @TBD (toml 09-11-96): resubmit here as a lock if lock, blow up if extract
                // @TBD (toml 09-11-96): We can't resolve it right now, because we're lazy!
                CriticalMsg("Oof! Don't know how to turn a preload into something else right now!");
                return FALSE;

            default:
                CriticalMsg("Unknown async request kind!");
        }
        return FALSE;
    }

    // If it's a preload request...
    if (kind == sResRequest::kPreload)
    {
        // ... and it's already locked, there's nothing to do but make sure it's at the back of the LRU...
        // ... or if we're not operating asynchronously just do it...
        if (ResPtr(id) || !IsAsynchronous())
        {
            ::ResLock(id);
            ::ResUnlock(id);
            return TRUE;
        }
    }

    // Reaching here, we have a fresh request to handle...
    AssertMsg(!pResRequest, "Expected request to be new");
    pResRequest = new sResRequest(m_Controls, id, priority, kind, pBuf, bufSize);

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
            return FALSE;
        }

    }

    // When not asynchronous, leave the request hanging around in the table to be picked
    // up later when GetResult() is called
    pResRequest->nRequests++;
    return TRUE;
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
            pResult = ResLock(pResRequest->id);
            break;

        case sResRequest::kExtract:
            pResult = ResExtract(pResRequest->id, pResRequest->pBuf);
            break;

        case sResRequest::kPreload:
        {
            Id id = pResRequest->id;
            m_fSatisfyingPreload++;

            pResult = ResLock(id); // This will delete pResRequest
            ResUnlock(id);
            m_fSatisfyingPreload--;
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
