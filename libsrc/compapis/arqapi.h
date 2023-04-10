///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/arqapi.h $
// $Author: TOML $
// $Date: 1997/09/18 13:04:50 $
// $Revision: 1.12 $
//

#ifndef __ARQAPI_H
#define __ARQAPI_H

#include <comtools.h>
#include <prikind.h>
#include <arqguid.h>

#include <pshpack4.h>

///////////////////////////////////////////////////////////////////////////////

struct IStream; typedef struct IStream IStream;

F_DECLARE_INTERFACE(IAsyncReadQueue);
F_DECLARE_INTERFACE(IAsyncReadControl);
F_DECLARE_INTERFACE(IAsyncReadFulfiller);
F_DECLARE_INTERFACE(IAsyncStreamer);

typedef struct sARQResult         sARQResult;
typedef struct sARQRequest        sARQRequest;
typedef struct sARQStreamerDesc   sARQStreamerDesc;
typedef struct sARQStreamRequest  sARQStreamRequest;

///////////////////////////////////////////////////////////////////////////////
//
// Asynchronous Read Queue API
//

//
// Create an asynchronous read queue and add it to the global app-object
//
#define AsyncReadQueueCreate() \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _AsyncReadQueueCreate(IID_TO_REFIID(IID_IAsyncReadQueue), NULL, pAppUnknown); \
    COMRelease(pAppUnknown); \
}


//
// Creates an asynchronous read queue, aggregating it with specfied pOuter,
// use IAggregate protocol if ppAsyncReadQueue is NULL, else self-init
//
EXTERN tResult LGAPI _AsyncReadQueueCreate(REFIID, IAsyncReadQueue ** ppAsyncReadQueue, IUnknown * pOuter);


///////////////////////////////////////////////////////////////////////////////
//
// Read request structures
//

///////////////////////////////////////
//
// STRUCT: sARQRequest
//

struct sARQRequest
{
    // Pointer to object used to execute read
    IAsyncReadFulfiller *   pFulfiller;

    // Queue to use, zero-based
    unsigned                queue;

    // Priority of read
    int                     priority;

    // Optional, client defined data values
    DWORD                   dwData[6];

    // Optional tracing name for debug routines
    const char *            pszTrace;

};


///////////////////////////////////////
//
// STRUCT: sARQStreamRequest
//

struct sARQStreamRequest
{
    // Source of read
    IStream *       pSource;

    // Starting offset, may be kARQCurPos
    long            offset;

    // Length of read, may be kARQToEOF
    long            length;

    // Queue to use, zero-based
    unsigned        queue;

    // Priority of read
    int             priority;

    // Flags composed of bits from eARQStreamRequestFlags
    int             flags;

    // Destination, may be NULL if dynamic allocation is desired
    void *          pBufOut;

    // Destination size
    long            bufSize;

    // Optional tracing name for debug routines
    const char *    pszTrace;

};

// Helper macro
#define ARQInitStreamReq(arqreq, src, off, len, que, pri, f, p, sz, nm); \
{ \
    arqreq.pSource = src; \
    arqreq.offset = off; \
    arqreq.length = len; \
    arqreq.queue = que; \
    arqreq.priority = pri; \
    arqreq.flags = f; \
    arqreq.pBufOut = p; \
    arqreq.bufSize = sz; \
    arqreq.pszTrace = nm; \
}


///////////////////////////////////////

#define kARQCurPos   (-1L)
#define kARQToEOF    (-1L)


///////////////////////////////////////

enum eARQStreamRequestFlags
{
    // Specified buffer may safely be realloced
    kARQDynamicBuf = 0x01
};


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sARQResult
//

struct sARQResult
{
    // Status of the result
    HRESULT     result;

    // Result, whether dynamically allocated or buffer supplied
    void *      buffer;

    // Size of the result
    long        length;

    // Flags, composed of bits from eARQResultFlags
    unsigned    flags;

    // Number of read, if streaming 0 if starting, 'frame number' on
    // subsequent reads
    int         streamIndex;

};


///////////////////////////////////////

enum eARQResultFlags
{
    // The buffer was malloc’ed by read queue system
    kARQMalloc    = 0x01,

    // The request is associated with streaming data
    kARQStreamed  = 0x02,

    // The request was killed
    kARQKilled    = 0x04

};


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sARQStreamerDesc
//

struct sARQStreamerDesc
{
    // Source of read
    IStream *       pSource;

    // Starting offset, may be kARQCurPos
    long            offsetStart;

    // Length of header
    long            lenHeader;

    // Queue to use, zero-based
    unsigned        queue;

    // Priority of read
    int             priority;

    // Flags composed of bits from eAsyncStreamerFlags
    int             flags;

    // Destination of header read, if lenHeader specified.
    // May be NULL if dynamic allocation is desired
    void *          pBufOut;

    // Destination size
    long            bufSize;

    // Optional tracing name for debug routines
    const char *    pszTrace;

};


// Helper macro
#define ARQInitStrDesc(strdsc, src, off, len, que, pri, f, p, sz, nm); \
{ \
    strdsc.pSource = src; \
    strdsc.offsetStart = off; \
    strdsc.lenHeader = len; \
    strdsc.queue = que; \
    strdsc.priority = pri; \
    strdsc.flags = f; \
    strdsc.pBufOut = p; \
    strdsc.bufSize = sz; \
    strdsc.pszTrace = nm; \
}


///////////////////////////////////////

enum eAsyncStreamerFlags
{
    // The stream should preempt all others in the same queue, for its lifetime
    kAsStrExclusive     = 0x01

};


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAsyncReadQueue
//

#undef INTERFACE
#define INTERFACE IAsyncReadQueue

DECLARE_INTERFACE_(IAsyncReadQueue, IUnknown)
{
    //
    // IUnknown Methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Add a simple request to queue
    //
    STDMETHOD (QueueRequest)(THIS_ const sARQRequest * pRequest,
                                   IAsyncReadControl ** ppControl) PURE;

    //
    // Add a simple stream-oriented request to queue
    //
    STDMETHOD (QueueStreamRequest)(THIS_ const sARQStreamRequest * pRequest,
                                         IAsyncReadControl ** ppControl) PURE;

    //
    // Get a reader for streaming
    //
    STDMETHOD (GetAsyncStreamer)(THIS_ const sARQStreamerDesc * pDesc,
                                       IAsyncStreamer ** ppAsyncStreamer) PURE;

    //
    // Suspend all reads
    //
    STDMETHOD (Suspend)(THIS) PURE;

    //
    // Resume reading
    //
    STDMETHOD (Resume)(THIS) PURE;

    //
    // Query if a streamer is blocking
    //
    STDMETHOD_(BOOL, IsStreamerBlocking)(THIS) PURE;

    //
    // Synchronously satisfy all pending requests -- analogous to flush
    //
    STDMETHOD (FulfillAll)(THIS) PURE;

    //
    // Synchronously satisfy all pending requests with priority equal to or
    // higher than the specified priority
    //
    STDMETHOD (FulfillToPriority)(THIS_ int priority) PURE;

    //
    // Kill all pending requests
    //
    STDMETHOD (KillAll)(THIS) PURE;

};


// @TBD (toml 11-12-96): need to finish coding up all these C access macros!
#define IAsyncReadQueue_QueryInterface(p, a, b)  COMQueryInterface(p, a, b)
#define IAsyncReadQueue_AddRef(p)                COMAddRef(p)
#define IAsyncReadQueue_Release(p)               COMRelease(p)
#define IAsyncReadQueue_QueueRequest(p, a, b)    COMCall2(p, QueueRequest, a, b)
#define IAsyncReadQueue_FulfillAll(p)            COMCall0(p, FulfillAll)

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAsyncReadFulfiller
//

#undef INTERFACE
#define INTERFACE IAsyncReadFulfiller

DECLARE_INTERFACE_(IAsyncReadFulfiller, IUnknown)
{
    //
    // IUnknown Methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Satisfy the request
    //
    STDMETHOD (DoFulfill)(THIS_ const sARQRequest *, sARQResult *) PURE;

    //
    // Kill the request, discard if paritally or wholly fulfilled
    //
    STDMETHOD (DoKill)(THIS_ const sARQRequest *, BOOL fDiscard) PURE;

};


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAsyncReadControl
//

#undef INTERFACE
#define INTERFACE IAsyncReadControl

DECLARE_INTERFACE_(IAsyncReadControl, IUnknown)
{
    //
    // IUnknown Methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Query if request has been fulfilled
    //
    STDMETHOD_(BOOL, IsFulfilled)(THIS) PURE;

    //
    // Synchronously satisfy the request
    //
    STDMETHOD (Fulfill)(THIS) PURE;

    //
    // Kill the request, discard if paritally or wholly fulfilled
    //
    STDMETHOD (Kill)(THIS_ BOOL fDiscard) PURE;

    //
    // Get the result
    //
    STDMETHOD (GetResult)(THIS_ sARQResult *) PURE;

};


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAsyncStreamer
//

#undef INTERFACE
#define INTERFACE IAsyncStreamer

DECLARE_INTERFACE_(IAsyncStreamer, IUnknown)
{
    //
    // IUnknown Methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    //
    //
    STDMETHOD (Read)(THIS_ void * pBufOut, long length) PURE;

    //
    //
    //
    STDMETHOD (GetNextResult)(THIS_ sARQResult *) PURE;

    //
    // Synchronously satisfy all pending requests from this stream -- analogous
    // to flush
    //
    STDMETHOD (FulfillAll)(THIS) PURE;

    //
    // Kill all pending requests from this stream
    //
    STDMETHOD (KillAll)(THIS_ BOOL fDiscard) PURE;

};


///////////////////////////////////////////////////////////////////////////////

#include <poppack.h>

#endif /* !__ARQAPI_H */
