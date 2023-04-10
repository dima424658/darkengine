///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/actmovie/RCS/amalloc.h $
// $Author: JON $
// $Date: 1996/09/18 11:32:12 $
// $Revision: 1.3 $
//
// Active Movie related allocation code cribbed from Microsoft
//

#ifndef __AMALLOC_H
#define __AMALLOC_H

#include <thrdtool.h>
#include <dev2d.h>
#include <ddraw.h>

#undef Free

class cMediaSample;
class cBaseAllocator;
class cMemAllocator;

///////////////////////////////////////////////////////////////////////////////
//
// Memory allocators
//
// the shared memory transport between pins requires the input pin
// to provide a memory allocator that can provide sample objects. A
// sample object supports the IMediaSample interface.
//
// cBaseAllocator handles the management of free and busy samples. It
// allocates cMediaSample objects. cBaseAllocator is an abstract class:
// in particular it has no method of initializing the list of free
// samples. cMemAllocator is derived from cBaseAllocator and initializes
// the list of samples using memory from the standard IMalloc interface.
//
// If you want your buffers to live in some special area of memory,
// derive your allocator object from cBaseAllocator. If you derive your
// IMemInputPin interface object from cBaseMemInputPin, you will get
// cMemAllocator-based allocation etc for free and will just need to
// supply the Receive handling, and media type / format negotiation.
//

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMediaSample
//
// an object of this class supports IMediaSample and represents a buffer
// for media data with some associated properties. Releasing it returns
// it to a freelist managed by a cBaseAllocator derived object.
//

class cMediaSample : public IMediaSample
    {
public:

    cMediaSample(char * pName, cBaseAllocator * pAllocator, HRESULT * phr,
                  BYTE * pBuffer = NULL, long length = 0);

    virtual ~cMediaSample();

    // IUnknown Methods
    DECLARE_UNAGGREGATABLE();

public:
    // set the buffer pointer and length. Used by allocators that
    // want variable sized pointers or pointers into already-read data.
    // This is only available through a cMediaSample* not an IMediaSample*
    // and so cannot be changed by clients.
    HRESULT SetPointer(BYTE * ptr, long cBytes);

    // Get me a read/write pointer to this buffer's memory.
    STDMETHOD (GetPointer)(BYTE ** ppBuffer);

    STDMETHOD_(long, GetSize)(void);

    // get the stream time at which this sample should start and finish.
    STDMETHOD (GetTime)(REFERENCE_TIME * pTimeStart, REFERENCE_TIME * pTimeEnd);

    // Set the stream time at which this sample should start and finish.
    STDMETHOD (SetTime)(REFERENCE_TIME * pTimeStart, REFERENCE_TIME * pTimeEnd);
    STDMETHOD (IsSyncPoint)(void);
    STDMETHOD (SetSyncPoint)(BOOL bIsSyncPoint);
    STDMETHOD (IsPreroll)(void);
    STDMETHOD (SetPreroll)(BOOL bIsPreroll);

    STDMETHOD_(long, GetActualDataLength)(void);
    STDMETHOD (SetActualDataLength)(long lActual);

    // these allow for limited format changes in band
    STDMETHOD (GetMediaType)(AM_MEDIA_TYPE ** ppMediaType);
    STDMETHOD (SetMediaType)(AM_MEDIA_TYPE * pMediaType);

    // returns S_OK if there is a discontinuity in the data (this same is
    // not a continuation of the previous stream of data
    // - there has been a seek).
    STDMETHOD (IsDiscontinuity)(void);

    // set the discontinuity property - TRUE if this sample is not a
    // continuation, but a new sample after a seek.
    STDMETHOD (SetDiscontinuity)(BOOL bDiscontinuity);

    // get the media times for this sample
    STDMETHOD (GetMediaTime)(LONGLONG * pTimeStart, LONGLONG * pTimeEnd);

    // Set the media times for this sample
    STDMETHOD (SetMediaTime)(LONGLONG * pTimeStart, LONGLONG * pTimeEnd);

protected:

    friend class cBaseAllocator;

    /* Values for dwFlags */
    enum
        {
        Sample_SyncPoint        = 0x01,             /* Is this a sync point */
        Sample_Preroll          = 0x02,             /* Is this a preroll sample */
        Sample_Discontinuity    = 0x04,             /* Set if start of new segment */
        Sample_TypeChanged      = 0x08,             /* Has the type changed */
        Sample_TimeValid        = 0x10,             /* Set if time is valid */
        Sample_MediaTimeValid   = 0x20              /* Is the media time valid */
        };

    /* Properties, the media sample class can be a container for a format change in which case we take a copy of a type
       through the SetMediaType interface function and then return it when GetMediaType is called. As we do no internal
       processing on it we leave it as a pointer */

    REFERENCE_TIME  m_Start;                     // Start sample time
    REFERENCE_TIME  m_End;                       // End sample time
    LONGLONG        m_MediaStart;                // Real media start position
    long            m_MediaEnd;                  // A difference to get the end
    DWORD           m_dwFlags;                   // Flags for this sample
    BYTE *          m_pBuffer;                   // Pointer to the complete buffer
    long            m_cbBuffer;                  // Size of the buffer
    long            m_lActual;                   // Length of data in this sample
    AM_MEDIA_TYPE * m_pMediaType;                // Media type change data
    cMediaSample *  m_pNext;                     // Chaining in free list

    cBaseAllocator * m_pAllocator;              /* The allocator who owns us */

    };

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cBitmapSample
//
//
class cBitmapSample : public cMediaSample
{
protected:
   grs_bitmap *m_pBitmap;

public:
   cBitmapSample(
                 TCHAR *pName,
                 cBaseAllocator *pAllocator,   
                 HRESULT *phr,
                 grs_bitmap *pBitmap,
                 LONG length);
   
   grs_bitmap *GetBitmap();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cBitmapSample
//
//
class cDDSample : public cMediaSample
{
protected:
   LPDIRECTDRAWSURFACE m_pDDSurface;
   IDirectDrawSurface *m_pIDDSurface;
   LPDDSURFACEDESC m_pDDSurfaceDesc;

public:
   cDDSample(
             TCHAR *pName,
             cBaseAllocator *pAllocator,   
             HRESULT *phr,
             LPDIRECTDRAWSURFACE pDDSurface,
             IDirectDrawSurface *pIDDSurface,
             LPDDSURFACEDESC pDDSurfaceDesc, 
             LONG length);
   
   LPDIRECTDRAWSURFACE GetDDSurface();
   IDirectDrawSurface *GetIDDSurface();
   LPDDSURFACEDESC GetDDSurfaceDesc();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cBaseAllocator
//
// Abstract base class that manages a list of media samples
//
// This class provides support for getting buffers from the free list,
// including handling of commit and (asynchronous) decommit.
//
// Derive from this class and override the Alloc and Free functions to
// allocate your cMediaSample (or derived) objects and add them to the
// free list, preparing them as necessary.
//

class cBaseAllocator : public IMemAllocator
    {
public:
    cBaseAllocator(char *, LPUNKNOWN, HRESULT *, BOOL bEvent = TRUE);
    virtual ~ cBaseAllocator();

    // IUnknown Methods
    DECLARE_UNAGGREGATABLE();

public:
    STDMETHOD (SetProperties)(ALLOCATOR_PROPERTIES * pRequest, ALLOCATOR_PROPERTIES * pActual);

    // return the properties actually being used on this allocator
    STDMETHOD (GetProperties)(ALLOCATOR_PROPERTIES * pProps);

    // override Commit to allocate memory. We handle the GetBuffer
    // state changes
    STDMETHOD (Commit)();

    // override this to handle the memory freeing. We handle any outstanding
    // GetBuffer calls
    STDMETHOD (Decommit)();

    // get container for a sample. Blocking, synchronous call to get the
    // next free buffer (as represented by an IMediaSample interface).
    // on return, the time etc properties will be invalid, but the buffer
    // pointer and size will be correct. The two time parameters are
    // optional and either may be NULL, they may alternatively be set to
    // the start and end times the sample will have attached to it
    // bPrevFramesSkipped is not used (used only by the video renderer's
    // allocator where it affects quality management in direct draw).
    STDMETHOD (GetBuffer)(IMediaSample ** ppBuffer,
                          REFERENCE_TIME * pStartTime,
                          REFERENCE_TIME * pEndTime,
                          DWORD dwFlags);

    // final release of a cMediaSample will call this
    STDMETHOD (ReleaseBuffer)(IMediaSample * pBuffer);

    // Notify that a sample is available
    void NotifySample();

    // Notify that we're waiting for a sample
    void SetWaiting()
        {
        m_lWaiting++;
        }

protected:

    cThreadLock m_Lock;

    class cSampleList;
    friend class cSampleList;

    /* Hack to get at protected member in cMediaSample */
    static cMediaSample *&NextSample(cMediaSample * pSample)
        {
        return pSample->m_pNext;
        };

    /* Mini list class for the free list */
    class cSampleList
        {
    public:
        cSampleList()
          : m_List(NULL),
            m_nOnList(0)                                    {}
        ~cSampleList()                                      { Assert_(m_nOnList == 0); }
        cMediaSample * Head() const                         { return m_List; }
        cMediaSample * Next(cMediaSample * pSample) const   { return cBaseAllocator::NextSample(pSample); }
        int GetCount() const                                { return m_nOnList; };

        void Add(cMediaSample * pSample)
            {
            Assert_(pSample != NULL);
            cBaseAllocator::NextSample(pSample) = m_List;
            m_List = pSample;
            m_nOnList++;
            }

        cMediaSample *RemoveHead()
            {
            cMediaSample *pSample = m_List;
            if (pSample != NULL)
                {
                m_List = cBaseAllocator::NextSample(m_List);
                m_nOnList--;
                }
             return pSample;
            }

        void Remove(cMediaSample * pSample);

        cMediaSample * m_List;
        int m_nOnList;
        };

    cSampleList m_lFree;                         // Free list

    /* Note to overriders of cBaseAllocator.

    We use a lazy signalling mechanism for waiting for samples. This means we don't call the OS if no waits occur.

    In order to implement this:

    1. When a new sample is added to m_lFree call NotifySample() which calls ReleaseSemaphore on m_hSem with a count of
       m_lWaiting and sets m_lWaiting to 0. This must all be done holding the allocator's critical section.

    2. When waiting for a sample call SetWait() which increments m_lWaiting BEFORE leaving the allocator's critical section.

    3. Actually wait by calling WaitForSingleObject(m_hSem, INFINITE) having left the allocator's critical section.  The
       effect of this is to remove 1 from the semaphore's count.  You MUST call this once having incremented m_lWaiting.

    The following are then true when the critical section is not held : (let nWaiting = number about to wait or waiting)

    (1) if (m_lFree.GetCount() != 0) then (m_lWaiting == 0) (2) m_lWaiting + Semaphore count == nWaiting

    We would deadlock if nWaiting != 0 && m_lFree.GetCount() != 0 && Semaphore count == 0

    But from (1) if m_lFree.GetCount() != 0 then m_lWaiting == 0 so from (2) Semaphore count == nWaiting (which is non-0) so
       the deadlock can't happen. */

    HANDLE  m_hSem;                              // For signalling
    long    m_lWaiting;                          // Waiting for a free element
    long    m_lCount;                            // how many buffers we have agreed to provide
    long    m_lAllocated;                        // how many buffers are currently allocated
    long    m_lSize;                             // agreed size of each buffer
    long    m_lAlignment;                        // agreed alignment
    long    m_lPrefix;                           // agreed prefix (preceeds GetPointer() value)
    BOOL    m_bChanged;                          // Have the buffer requirements changed

    // if true, we are decommitted and can't allocate memory
    BOOL    m_bCommitted;

    // if true, the decommit has happened, but we haven't called Free yet
    // as there are still outstanding buffers
    BOOL    m_bDecommitInProgress;

    // called to decommit the memory when the last buffer is freed
    // pure virtual - need to override this
    virtual void Free(void) = 0;

    // override to allocate the memory when commit called
    virtual HRESULT Alloc(void);

    };


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMemAllocator
//
// this is an allocator based on cBaseAllocator that allocates sample
// buffers in main memory (from 'new'). You must call SetProperties
// before calling Commit.
//
// we don't free the memory when going into Decommit state. The simplest
// way to implement this without complicating cBaseAllocator is to
// have a Free() function, called to go into decommit state, that does
// nothing and a ReallyFree function called from our destructor that
// actually frees the memory.
//
class cMemAllocator : public cBaseAllocator
    {
public:
     cMemAllocator(char *, LPUNKNOWN, HRESULT *);
     ~cMemAllocator();

protected:
     STDMETHOD (SetProperties)(ALLOCATOR_PROPERTIES * pRequest,
                               ALLOCATOR_PROPERTIES * pActual);

    // override to free the memory when decommit completes
    // - we actually do nothing, and save the memory until deletion.
    void Free(void);

    // called from the destructor (and from Alloc if changing size/count) to
    // actually free up the memory
    void ReallyFree(void);

    // overriden to allocate the memory when commit called
    HRESULT Alloc(void);

private:
    BYTE * m_pBuffer;                            // combined memory for all buffers

    };

#endif /* !__AMALLOC_H */







