///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/actmovie/RCS/amalloc.cpp $
// $Author: JON $
// $Date: 1996/09/18 11:32:28 $
// $Revision: 1.3 $
//

#include <windows.h>
#include <lg.h>

#include <comtools.h>

#include <actmovie.h>

#include <control.h>
#include <strmif.h>
#include <evcode.h>
#include <uuids.h>
#include <vfwmsgs.h>

#include <amalloc.h>

///////////////////////////////////////////////////////////////////////////////
//
// @Note (toml 09-06-96):  The following are HACKS to get this file to compiler
//

// @TBD (toml 09-06-96): Thse macros are placeholders for things in mtype.*, which needs to be imported (?)
#define DeleteMediaType(p)
#define CreateMediaType(p) (p)

// @TBD (toml 09-06-96): these are things from wxutil
#define ValidateReadPtr(p,cb)
#define ValidateWritePtr(p,cb)
#define ValidateReadWritePtr(p,cb)
#define ValidateStringPtr(p)
#define ValidateStringPtrA(p)
#define ValidateStringPtrW(p)

// @TBD (toml 09-06-96): From wxdebug
#define CheckPointer(p,ret) {if((p)==NULL) return (ret);}

#undef DbgLog
#define DbgLog(x)

#define NAME(s) NULL

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMediaSample
//
// Memory allocation class, implements cMediaSample
//

///////////////////////////////////////
//
// Pre-fab COM implementations
//

IMPLEMENT_UNAGGREGATABLE(cMediaSample, IMediaSample);

///////////////////////////////////////

#pragma off(unreferenced)
cMediaSample::cMediaSample(const char *pName,
                           cBaseAllocator * pAllocator,
                           HRESULT * phr,
                           BYTE * pBuffer,
                           long length)
  : m_pBuffer(pBuffer),                          // Initialise the buffer
    m_cbBuffer(length),                          // And it's length
    m_lActual(length),                           // By default, actual = length
    m_pMediaType(NULL),                          // No media type change
    m_dwFlags(0)                                 // Nothing set
    {
    /* We must have an owner and it must also be derived from class cBaseAllocator BUT we do not hold a reference count on it */
    m_pAllocator = pAllocator;
    Assert_(pAllocator);

    if (pAllocator == NULL)
        {
        *phr = VFW_E_NEED_OWNER;
        }
    }
#pragma on(unreferenced)


///////////////////////////////////////

cMediaSample::~cMediaSample()
    {
    if (m_pMediaType)
        {
        DeleteMediaType(m_pMediaType);
        m_pMediaType = NULL;
        }
    }


///////////////////////////////////////
//
// On final release of this sample buffer it is not deleted but
// returned to the freelist of the owning memory allocator
//
// The allocator may be waiting for the last buffer to be placed on the free
// list in order to decommit all the memory, so the ReleaseBuffer() call may
// result in this sample being deleted. We also need to hold a refcount on
// the allocator to stop that going away until we have finished with this.
// However, we cannot release the allocator before the ReleaseBuffer, as the
// release may cause us to be deleted. Similarly we can't do it afterwards.
//
// Thus we must leave it to the allocator to hold an addref on our behalf.
// When he issues us in GetBuffer, he addref's himself. When ReleaseBuffer
// is called, he releases himself, possibly causing us and him to be deleted.

void cMediaSample::OnFinalRelease()
    {
    /* Free all resources */
    SetMediaType(NULL);
    m_dwFlags = 0;

    /* This may cause us to be deleted */
    // Our refcount is reliably 0 thus no-one will mess with us
    m_pAllocator->ReleaseBuffer(this);
    }


///////////////////////////////////////
//
// set the buffer pointer and length. Used by allocators that
// want variable sized pointers or pointers into already-read data.
// This is only available through a cMediaSample* not an IMediaSample*
// and so cannot be changed by clients.
//

HRESULT cMediaSample::SetPointer(BYTE * ptr, long cBytes)
    {
    m_pBuffer = ptr;                             // new buffer area (could be null)
    m_cbBuffer = cBytes;                         // length of buffer
    m_lActual = cBytes;                          // length of data in buffer (assume full)

    return S_OK;
    }


///////////////////////////////////////
//
// get me a read/write pointer to this buffer's memory. I will actually
// want to use sizeUsed bytes.
//

STDMETHODIMP cMediaSample::GetPointer(BYTE ** ppBuffer)
    {
    CheckPointer(ppBuffer, E_POINTER);
    ValidateReadWritePtr(ppBuffer, sizeof(BYTE *));

    // creator must have set pointer either during
    // constructor or by SetPointer
    Assert_(m_pBuffer);
    if (m_pBuffer == 0)
        {
        return VFW_E_BUFFER_NOTSET;
        }

    *ppBuffer = m_pBuffer;
    return NOERROR;
    }


///////////////////////////////////////
//
// return the size in bytes of this buffer
//

STDMETHODIMP_(long) cMediaSample::GetSize(void)
    {
    return m_cbBuffer;
    }


///////////////////////////////////////
//
// get the stream time at which this sample should start and finish.
//

STDMETHODIMP cMediaSample::GetTime(REFERENCE_TIME * pTimeStart,      // put time here
                                   REFERENCE_TIME * pTimeEnd)
    {
    CheckPointer(pTimeStart, E_POINTER);
    CheckPointer(pTimeEnd, E_POINTER);
    ValidateReadWritePtr(pTimeStart, sizeof(REFERENCE_TIME));
    ValidateReadWritePtr(pTimeEnd, sizeof(REFERENCE_TIME));

    if (!(m_dwFlags & Sample_TimeValid))
        {
        return VFW_E_SAMPLE_TIME_NOT_SET;
        }

    *pTimeStart = m_Start;
    *pTimeEnd = m_End;
    return NOERROR;
    }


///////////////////////////////////////
//
// Set the stream time at which this sample should start and finish.
//
STDMETHODIMP cMediaSample::SetTime(REFERENCE_TIME * pTimeStart,
                                   REFERENCE_TIME * pTimeEnd)
    {
    if (!pTimeStart || !pTimeEnd)
        {
        if (!pTimeStart && !pTimeEnd)
            {
            m_dwFlags &= ~Sample_TimeValid;
            }
        else
            {
            CheckPointer(pTimeStart, E_POINTER);
            CheckPointer(pTimeEnd, E_POINTER);
            }
        }
    else
        {
        ValidateReadPtr(pTimeStart, sizeof(REFERENCE_TIME));
        ValidateReadPtr(pTimeEnd, sizeof(REFERENCE_TIME));
        Assert_(*pTimeEnd >= *pTimeStart);

        m_Start = *pTimeStart;
        m_End = *pTimeEnd;
        m_dwFlags |= Sample_TimeValid;
        }
    return NOERROR;
    }


///////////////////////////////////////
//
// get the media times (eg bytes) for this sample
//

STDMETHODIMP cMediaSample::GetMediaTime(LONGLONG * pTimeStart,
                                        LONGLONG * pTimeEnd)
    {
    CheckPointer(pTimeStart, E_POINTER);
    CheckPointer(pTimeEnd, E_POINTER);
    ValidateReadWritePtr(pTimeStart, sizeof(LONGLONG));
    ValidateReadWritePtr(pTimeEnd, sizeof(LONGLONG));

    if (!(m_dwFlags & Sample_MediaTimeValid))
        {
        return VFW_E_MEDIA_TIME_NOT_SET;
        }

    *pTimeStart = m_MediaStart;
    *pTimeEnd = (m_MediaStart + m_MediaEnd);
    return NOERROR;
    }


///////////////////////////////////////
//
// Set the media times for this sample
//

STDMETHODIMP cMediaSample::SetMediaTime(LONGLONG * pTimeStart,
                                        LONGLONG * pTimeEnd)
    {
    if (!pTimeStart && !pTimeEnd)
        {
        m_dwFlags &= ~Sample_MediaTimeValid;
        }
    else
        {
        CheckPointer(pTimeStart, E_POINTER);
        CheckPointer(pTimeEnd, E_POINTER);
        ValidateReadPtr(pTimeStart, sizeof(LONGLONG));
        ValidateReadPtr(pTimeEnd, sizeof(LONGLONG));
        Assert_(*pTimeEnd >= *pTimeStart);

        m_MediaStart = *pTimeStart;
        m_MediaEnd = (long) (*pTimeEnd - *pTimeStart);
        m_dwFlags |= Sample_MediaTimeValid;
        }
    return NOERROR;
    }


///////////////////////////////////////

STDMETHODIMP cMediaSample::IsSyncPoint(void)
    {
    if (m_dwFlags & Sample_SyncPoint)
        {
        return S_OK;
        }
    else
        {
        return S_FALSE;
        }
    }


///////////////////////////////////////

STDMETHODIMP cMediaSample::SetSyncPoint(BOOL bIsSyncPoint)
    {
    if (bIsSyncPoint)
        {
        m_dwFlags |= Sample_SyncPoint;
        }
    else
        {
        m_dwFlags &= ~Sample_SyncPoint;
        }
    return NOERROR;
    }

///////////////////////////////////////
//
// returns S_OK if there is a discontinuity in the data (this same is
// not a continuation of the previous stream of data
// - there has been a seek).
//

STDMETHODIMP cMediaSample::IsDiscontinuity(void)
    {
    if (m_dwFlags & Sample_Discontinuity)
        {
        return S_OK;
        }
    else
        {
        return S_FALSE;
        }
    }

///////////////////////////////////////
//
// set the discontinuity property - TRUE if this sample is not a
// continuation, but a new sample after a seek.
//

STDMETHODIMP cMediaSample::SetDiscontinuity(BOOL bDiscont)
    {
    // should be TRUE or FALSE
    if (bDiscont)
        {
        m_dwFlags |= Sample_Discontinuity;
        }
    else
        {
        m_dwFlags &= ~Sample_Discontinuity;
        }
    return S_OK;
    }

///////////////////////////////////////

STDMETHODIMP cMediaSample::IsPreroll(void)
    {
    if (m_dwFlags & Sample_Preroll)
        {
        return S_OK;
        }
    else
        {
        return S_FALSE;
        }
    }


///////////////////////////////////////

STDMETHODIMP cMediaSample::SetPreroll(BOOL bIsPreroll)
    {
    if (bIsPreroll)
        {
        m_dwFlags |= Sample_Preroll;
        }
    else
        {
        m_dwFlags &= ~Sample_Preroll;
        }
    return NOERROR;
    }

///////////////////////////////////////

STDMETHODIMP_(long) cMediaSample::GetActualDataLength(void)
    {
    return m_lActual;
    }


///////////////////////////////////////

STDMETHODIMP cMediaSample::SetActualDataLength(long lActual)
    {
    if (lActual > GetSize())
        {
        Assert_(lActual <= GetSize());
        return VFW_E_BUFFER_OVERFLOW;
        }
    m_lActual = lActual;
    return NOERROR;
    }


///////////////////////////////////////

STDMETHODIMP cMediaSample::GetMediaType(AM_MEDIA_TYPE ** ppMediaType)
    {
    CheckPointer(ppMediaType, E_POINTER);
    ValidateReadWritePtr(ppMediaType, sizeof(AM_MEDIA_TYPE *));
    Assert_(ppMediaType);

    /* Do we have a new media type for them */

    if (!(m_dwFlags & Sample_TypeChanged))
        {
        Assert_(m_pMediaType == NULL);
        *ppMediaType = NULL;
        return S_FALSE;
        }

    Assert_(m_pMediaType);

    /* Create a copy of our media type */

    *ppMediaType = CreateMediaType(m_pMediaType);
    if (*ppMediaType == NULL)
        {
        return E_OUTOFMEMORY;
        }
    return NOERROR;
    }


///////////////////////////////////////

STDMETHODIMP cMediaSample::SetMediaType(AM_MEDIA_TYPE * pMediaType)
    {
    /* Delete the current media type */

    if (m_pMediaType)
        {
        DeleteMediaType(m_pMediaType);
        m_pMediaType = NULL;
        }

    /* Mechanism for resetting the format type */

    if (pMediaType == NULL)
        {
        m_dwFlags &= ~Sample_TypeChanged;
        return NOERROR;
        }

    Assert_(pMediaType);
    ValidateReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE));

    /* Take a copy of the media type */

    m_pMediaType = CreateMediaType(pMediaType);
    if (m_pMediaType == NULL)
        {
        m_dwFlags &= ~Sample_TypeChanged;
        return E_OUTOFMEMORY;
        }

    m_dwFlags |= Sample_TypeChanged;
    return NOERROR;
    }

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cBaseAllocator
//

///////////////////////////////////////
//
// Pre-fab COM implementations
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseAllocator, IMemAllocator);

///////////////////////////////////////

#pragma off(unreferenced)
cBaseAllocator::cBaseAllocator(const char *pName, LPUNKNOWN pUnk, HRESULT * phr, BOOL bEvent)
  : m_lAllocated(0),
    m_bChanged(FALSE),
    m_bCommitted(FALSE),
    m_bDecommitInProgress(FALSE),
    m_lSize(0),
    m_lCount(0),
    m_lAlignment(0),
    m_lPrefix(0),
    m_hSem(NULL),
    m_lWaiting(0)
    {
    AssertMsg(!pUnk, "Aggregation of cBaseAllocator not implemented!");
    if (bEvent)
        {
        m_hSem = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
        if (m_hSem == NULL)
            {
            *phr = E_OUTOFMEMORY;
            return;
            }
        }
    }
#pragma on(unreferenced)


///////////////////////////////////////

cBaseAllocator::~cBaseAllocator()
    {
    // we can't call Decommit here since that would mean a call to a
    // pure virtual in destructor.
    // We must assume that the derived class has gone into decommit state in
    // its destructor.
    Assert_(!m_bCommitted);
    if (m_hSem != NULL)
        {
        Verify(CloseHandle(m_hSem));
        }
    }


///////////////////////////////////////

/* This sets the size and count of the required samples. The memory isn't
   actually allocated until Commit() is called, if memory has already been
   allocated then assuming no samples are outstanding the user may call us
   to change the buffering, the memory will be released in Commit() */

STDMETHODIMP cBaseAllocator::SetProperties(ALLOCATOR_PROPERTIES * pRequest,
                                           ALLOCATOR_PROPERTIES * pActual)
    {
    CheckPointer(pRequest, E_POINTER);
    CheckPointer(pActual, E_POINTER);
    ValidateReadWritePtr(pActual, sizeof(ALLOCATOR_PROPERTIES));
    cAutoLock lock(m_Lock);
    ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

    Assert_(pRequest->cbBuffer > 0);

    /* Check the alignment requested */
    if (pRequest->cbAlign != 1)
        {
        DbgLog((LOG_ERROR, 2, TEXT("Alignment requested was 0x%x, not 1"),
                pRequest->cbAlign));
        return VFW_E_BADALIGN;
        }

    /* Can't do this if already committed, there is an argument that says we should not reject the SetProperties call if there
       are buffers still active. However this is called by the source filter, which is the same person who is holding the
       samples. Therefore it is not unreasonable for them to free all their samples before changing the requirements */

    if (m_bCommitted)
        {
        return VFW_E_ALREADY_COMMITTED;
        }

    /* Must be no outstanding buffers */

    if (m_lAllocated != m_lFree.GetCount())
        {
        return VFW_E_BUFFERS_OUTSTANDING;
        }

    /* There isn't any real need to check the parameters as they will just be rejected when the user finally calls Commit */

    pActual->cbBuffer = m_lSize = pRequest->cbBuffer;
    pActual->cBuffers = m_lCount = pRequest->cBuffers;
    pActual->cbAlign = m_lAlignment = pRequest->cbAlign;
    pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

    m_bChanged = TRUE;
    return NOERROR;
    }

///////////////////////////////////////

STDMETHODIMP cBaseAllocator::GetProperties(ALLOCATOR_PROPERTIES * pActual)
    {
    CheckPointer(pActual, E_POINTER);
    ValidateReadWritePtr(pActual, sizeof(ALLOCATOR_PROPERTIES));

    cAutoLock lock(m_Lock);
    pActual->cbBuffer = m_lSize;
    pActual->cBuffers = m_lCount;
    pActual->cbAlign = m_lAlignment;
    pActual->cbPrefix = m_lPrefix;
    return NOERROR;
    }

///////////////////////////////////////
// get container for a sample. Blocking, synchronous call to get the
// next free buffer (as represented by an IMediaSample interface).
// on return, the time etc properties will be invalid, but the buffer
// pointer and size will be correct.

HRESULT cBaseAllocator::GetBuffer(IMediaSample ** ppBuffer,
                                  REFERENCE_TIME * /* pStartTime */,
                                  REFERENCE_TIME * /*pEndTime */,
                                  DWORD /* dwFlags */)
    {
    cMediaSample *pSample;
    *ppBuffer = NULL;
    for (;;)
        {
            {                                    // scope for lock
            cAutoLock lock(m_Lock);
            /* Check we are committed */
            if (!m_bCommitted)
                {
                return VFW_E_NOT_COMMITTED;
                }
            pSample = (cMediaSample *) m_lFree.RemoveHead();
            if (pSample == NULL)
                {
                SetWaiting();
                }
            }

        /* If we didn't get a sample then wait for the list to signal */

        if (pSample)
            {
            break;
            }
        Assert_(m_hSem != NULL);
        WaitForSingleObject(m_hSem, INFINITE);
        }

    /* This QueryInterface should addref the buffer up to one. On release back to zero instead of being deleted, it will
       requeue itself by calling the ReleaseBuffer member function. NOTE the owner of a media sample must always be derived
       from cBaseAllocator */

    HRESULT hr = pSample->QueryInterface(IID_IMediaSample, (void **) ppBuffer);
    /* For each sample outstanding, we need to AddRef ourselves on his behalf he cannot do it, as there is no correct ordering
       of his release and his call to ReleaseBuffer as both could destroy him. We release this count in ReleaseBuffer, called
       when the sample's count drops to zero */

    AddRef();
    return NOERROR;
    }


///////////////////////////////////////

/* Final release of a cMediaSample will call this */

STDMETHODIMP cBaseAllocator::ReleaseBuffer(IMediaSample * pSample)
    {
    CheckPointer(pSample, E_POINTER);
    ValidateReadPtr(pSample, sizeof(IMediaSample));
        {
        cAutoLock lock(m_Lock);
        /* Put back on the free list */

        m_lFree.Add((cMediaSample *) pSample);
        NotifySample();

        // if there is a pending Decommit, then we need to complete it by
        // calling Free() when the last buffer is placed on the free list

        long l1 = m_lFree.GetCount();
        if (m_bDecommitInProgress && (l1 == m_lAllocated))
            {
            Free();
            m_bDecommitInProgress = FALSE;
            }
        }

    /* For each buffer there is one AddRef, made in GetBuffer and released here. This may cause the allocator and all samples
       to be deleted */

    Release();
    return NOERROR;
    }

///////////////////////////////////////

void cBaseAllocator::NotifySample()
    {
    if (m_lWaiting != 0)
        {
        Assert_(m_hSem != NULL);
        ReleaseSemaphore(m_hSem, m_lWaiting, 0);
        m_lWaiting = 0;
        }
    }

///////////////////////////////////////

STDMETHODIMP cBaseAllocator::Commit()
    {
    /* Check we are not decommitted */
    cAutoLock lock(m_Lock);
    // cannot need to alloc or re-alloc if we are committed
    if (m_bCommitted)
        {
        return NOERROR;
        }

    /* Allow GetBuffer calls */

    m_bCommitted = TRUE;

    // is there a pending decommit ? if so, just cancel it
    if (m_bDecommitInProgress)
        {
        m_bDecommitInProgress = FALSE;

        // don't call Alloc at this point. He cannot allow SetProperties
        // between Decommit and the last free, so the buffer size cannot have
        // changed. And because some of the buffers are not free yet, he
        // cannot re-alloc anyway.
        return NOERROR;
        }

    DbgLog((LOG_MEMORY, 1, TEXT("Allocating: %ldx%ld"), m_lCount, m_lSize));

    // actually need to allocate the samples
    HRESULT hr = Alloc();
    if (FAILED(hr))
        {
        m_bCommitted = FALSE;
        return hr;
        }
    return NOERROR;
    }


///////////////////////////////////////

STDMETHODIMP cBaseAllocator::Decommit()
    {
    /* Check we are not already decommitted */
    cAutoLock lock(m_Lock);
    if (m_bCommitted == FALSE)
        {
        if (m_bDecommitInProgress == FALSE)
            {
            return NOERROR;
            }
        }

    /* No more GetBuffer calls will succeed */
    m_bCommitted = FALSE;

    // are any buffers outstanding?
    if (m_lFree.GetCount() < m_lAllocated)
        {
        // please complete the decommit when last buffer is freed
        m_bDecommitInProgress = TRUE;
        }
    else
        {
        m_bDecommitInProgress = FALSE;

        // need to complete the decommit here as there are no
        // outstanding buffers

        Free();
        }

    // Tell anyone whose waiting that they can go now so we can
    // reject their call
    NotifySample();

    return NOERROR;
    }


///////////////////////////////////////

/* Base definition of allocation which checks we are ok to go ahead and do
   the full allocation. We return S_FALSE if the requirements are the same */

HRESULT cBaseAllocator::Alloc(void)
    {
    /* Error if he hasn't set the size yet */
    if (m_lCount <= 0 || m_lSize <= 0 || m_lAlignment <= 0)
        {
        return VFW_E_SIZENOTSET;
        }

    /* should never get here while buffers outstanding */
    Assert_(m_lFree.GetCount() == m_lAllocated);

    /* If the requirements haven't changed then don't reallocate */
    if (m_bChanged == FALSE)
        {
        return S_FALSE;
        }

    return NOERROR;
    }

///////////////////////////////////////

/*  Implement cBaseAllocator::cSampleList::Remove(pSample)
    Removes pSample from the list
*/
void cBaseAllocator::cSampleList::Remove(cMediaSample * pSample)
    {
    cMediaSample **pSearch;
    for (pSearch = &m_List;
         *pSearch != NULL;
         pSearch = &(cBaseAllocator::NextSample(*pSearch)))
        {
        if (*pSearch == pSample)
            {
            *pSearch = cBaseAllocator::NextSample(pSample);
            cBaseAllocator::NextSample(pSample) = NULL;
            m_nOnList--;
            return;
            }
        }

    CriticalMsg("Couldn't find sample in list");
    }

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMemAllocator
//


cMemAllocator::cMemAllocator(char *pName,
                             LPUNKNOWN pUnk,
                             HRESULT * phr)
  : cBaseAllocator(pName, pUnk, phr),
    m_pBuffer(NULL)
    {
    }

///////////////////////////////////////

/* This sets the size and count of the required samples. The memory isn't
   actually allocated until Commit() is called, if memory has already been
   allocated then assuming no samples are outstanding the user may call us
   to change the buffering, the memory will be released in Commit() */
STDMETHODIMP cMemAllocator::SetProperties(ALLOCATOR_PROPERTIES * pRequest,
                                          ALLOCATOR_PROPERTIES * pActual)
    {
    CheckPointer(pActual, E_POINTER);
    ValidateReadWritePtr(pActual, sizeof(ALLOCATOR_PROPERTIES));
    cAutoLock lock(m_Lock);
    ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

    Assert_(pRequest->cbBuffer > 0);

    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);

    /* Check the alignment request is a power of 2 */
    if ((-pRequest->cbAlign & pRequest->cbAlign) != pRequest->cbAlign)
        {
        DbgLog((LOG_ERROR, 1, TEXT("Alignment requested 0x%x not a power of 2!"),
                pRequest->cbAlign));
        }
    /* Check the alignment requested */
    if (pRequest->cbAlign == 0 ||
        SysInfo.dwAllocationGranularity & (pRequest->cbAlign - 1) != 0)
        {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid alignment 0x%x requested - granularity = 0x%x"),
                pRequest->cbAlign, SysInfo.dwAllocationGranularity));
        return VFW_E_BADALIGN;
        }

    /* Can't do this if already committed, there is an argument that says we should not reject the SetProperties call if there
       are buffers still active. However this is called by the source filter, which is the same person who is holding the
       samples. Therefore it is not unreasonable for them to free all their samples before changing the requirements */

    if (m_bCommitted == TRUE)
        {
        return VFW_E_ALREADY_COMMITTED;
        }

    /* Must be no outstanding buffers */

    if (m_lFree.GetCount() < m_lAllocated)
        {
        return VFW_E_BUFFERS_OUTSTANDING;
        }

    /* There isn't any real need to check the parameters as they will just be rejected when the user finally calls Commit */

    // round length up to alignment - remember that prefix is included in
    // the alignment
    long lSize = pRequest->cbBuffer + pRequest->cbPrefix;
    long lRemainder = lSize % pRequest->cbAlign;
    if (lRemainder != 0)
        {
        lSize = lSize - lRemainder + pRequest->cbAlign;
        }
    pActual->cbBuffer = m_lSize = (lSize - pRequest->cbPrefix);

    pActual->cBuffers = m_lCount = pRequest->cBuffers;
    pActual->cbAlign = m_lAlignment = pRequest->cbAlign;
    pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

    m_bChanged = TRUE;
    return NOERROR;
    }

///////////////////////////////////////
// override this to allocate our resources when Commit is called.
//
// note that our resources may be already allocated when this is called,
// since we don't free them on Decommit. We will only be called when in
// decommit state with all buffers free.
//
// object locked by caller

HRESULT cMemAllocator::Alloc(void)
    {
    cAutoLock lock(m_Lock);
    /* Check he has called SetProperties */
    HRESULT hr = cBaseAllocator::Alloc();
    if (FAILED(hr))
        {
        return hr;
        }

    /* If the requirements haven't changed then don't reallocate */
    if (hr == S_FALSE)
        {
        Assert_(m_pBuffer);
        return NOERROR;
        }
    Assert_(hr == S_OK);                          // we use this fact in the loop below

    /* Free the old resources */
    if (m_pBuffer)
        {
        ReallyFree();
        }

    /* Create the contiguous memory block for the samples making sure it's properly aligned (64K should be enough!) */
    Assert_(m_lAlignment != 0 &&
           (m_lSize + m_lPrefix) % m_lAlignment == 0);

    m_pBuffer = (PBYTE) VirtualAlloc(NULL,
                                     m_lCount * (m_lSize + m_lPrefix),
                                     MEM_COMMIT,
                                     PAGE_READWRITE);

    if (m_pBuffer == NULL)
        {
        return E_OUTOFMEMORY;
        }

    BYTE *pNext = m_pBuffer;
    cMediaSample *pSample;
    Assert_(m_lAllocated == 0);

    // Create the new samples - we have allocated m_lSize bytes for each sample
    // plus m_lPrefix bytes per sample as a prefix. We set the pointer to
    // the memory after the prefix - so that GetPointer() will return a pointer
    // to m_lSize bytes.
    for (; m_lAllocated < m_lCount; m_lAllocated++, pNext += (m_lSize + m_lPrefix))
        {


        pSample = new cMediaSample(NAME("Default memory media sample"),
                                   this,
                                   &hr,
                                   pNext + m_lPrefix,   // GetPointer() value
                                   m_lSize);     // not including prefix

        if (FAILED(hr) || pSample == NULL)
            {
            delete pSample;
            return E_OUTOFMEMORY;
            }

        // This CANNOT fail
        //        m_lFree.Add(pSample);
        // Our new results in a 1 reference count not zero, so we start by releasing the sample
        // (which decrements the reference count and then adds it to the free list anyway).
        // This should fix things so that media samples are actually released when we're finished with them.
        pSample->Release();
        }

    m_bChanged = FALSE;
    return NOERROR;
    }


///////////////////////////////////////
// override this to free up any resources we have allocated.
// called from the base class on Decommit when all buffers have been
// returned to the free list.
//
// caller has already locked the object.

// in our case, we keep the memory until we are deleted, so
// we do nothing here. The memory is deleted in the destructor by
// calling ReallyFree()

void cMemAllocator::Free(void)
    {
    return;
    }


///////////////////////////////////////
// called from the destructor (and from Alloc if changing size/count) to
// actually free up the memory

void cMemAllocator::ReallyFree(void)
    {
    /* Should never be deleting this unless all buffers are freed */

    Assert_(m_lAllocated == m_lFree.GetCount());

    /* Free up all the cMediaSamples */

    cMediaSample *pSample;
    for (;;)
        {
        pSample = m_lFree.RemoveHead();
        if (pSample != NULL)
            {
            delete pSample;
            }
        else
            {
            break;
            }
        }

    m_lAllocated = 0;

    // free the block of buffer memory
    if (m_pBuffer)
        {
        Verify(VirtualFree(m_pBuffer, 0, MEM_RELEASE));
        m_pBuffer = NULL;
        }
    }


///////////////////////////////////////

/* Destructor frees our memory resources */

cMemAllocator::~cMemAllocator()
    {
    Decommit();
    ReallyFree();
    }

///////////////////////////////////////////////////////////////////////////////

#pragma off(unreferenced)
cDDSample::cDDSample(
               TCHAR *pName,
               cBaseAllocator *pAllocator,   
               HRESULT *phr,
               LPDIRECTDRAWSURFACE pDDSurface,
               IDirectDrawSurface *pIDDSurface,
               LPDDSURFACEDESC pDDSurfaceDesc,
               LONG length):
   cMediaSample(pName, pAllocator, phr, (BYTE*)pDDSurfaceDesc->lpSurface, length),
   m_pDDSurface(pDDSurface),
   m_pDDSurfaceDesc(pDDSurfaceDesc),
   m_pIDDSurface(pIDDSurface)
{

}
#pragma on(unreferenced)
   
LPDIRECTDRAWSURFACE cDDSample::GetDDSurface()
{
   return m_pDDSurface;
}

IDirectDrawSurface *cDDSample::GetIDDSurface()
{
   return m_pIDDSurface;
}

LPDDSURFACEDESC cDDSample::GetDDSurfaceDesc()
{
   return m_pDDSurfaceDesc;
}

#pragma off(unreferenced)
cBitmapSample::cBitmapSample(
                             const TCHAR *pName,
                             cBaseAllocator *pAllocator,   
                             HRESULT *phr,
                             grs_bitmap *pBitmap,
                             LONG length):
   cMediaSample(pName, pAllocator, phr, (BYTE*)pBitmap->bits, length),
   m_pBitmap(pBitmap)
{
}
#pragma on(unreferenced)

grs_bitmap *cBitmapSample::GetBitmap()
{
   return m_pBitmap;
}






