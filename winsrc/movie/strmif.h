/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0102 */
/* at Wed Jun 26 01:38:28 1996
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __strmif_h__
#define __strmif_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPin_FWD_DEFINED__
#define __IPin_FWD_DEFINED__
typedef interface IPin IPin;
#endif 	/* __IPin_FWD_DEFINED__ */


#ifndef __IEnumPins_FWD_DEFINED__
#define __IEnumPins_FWD_DEFINED__
typedef interface IEnumPins IEnumPins;
#endif 	/* __IEnumPins_FWD_DEFINED__ */


#ifndef __IEnumMediaTypes_FWD_DEFINED__
#define __IEnumMediaTypes_FWD_DEFINED__
typedef interface IEnumMediaTypes IEnumMediaTypes;
#endif 	/* __IEnumMediaTypes_FWD_DEFINED__ */


#ifndef __IFilterGraph_FWD_DEFINED__
#define __IFilterGraph_FWD_DEFINED__
typedef interface IFilterGraph IFilterGraph;
#endif 	/* __IFilterGraph_FWD_DEFINED__ */


#ifndef __IEnumFilters_FWD_DEFINED__
#define __IEnumFilters_FWD_DEFINED__
typedef interface IEnumFilters IEnumFilters;
#endif 	/* __IEnumFilters_FWD_DEFINED__ */


#ifndef __IMediaFilter_FWD_DEFINED__
#define __IMediaFilter_FWD_DEFINED__
typedef interface IMediaFilter IMediaFilter;
#endif 	/* __IMediaFilter_FWD_DEFINED__ */


#ifndef __IFilter_FWD_DEFINED__
#define __IFilter_FWD_DEFINED__
typedef interface IFilter IFilter;
#endif 	/* __IFilter_FWD_DEFINED__ */


#ifndef __IReferenceClock_FWD_DEFINED__
#define __IReferenceClock_FWD_DEFINED__
typedef interface IReferenceClock IReferenceClock;
#endif 	/* __IReferenceClock_FWD_DEFINED__ */


#ifndef __IMediaSample_FWD_DEFINED__
#define __IMediaSample_FWD_DEFINED__
typedef interface IMediaSample IMediaSample;
#endif 	/* __IMediaSample_FWD_DEFINED__ */


#ifndef __IMemAllocator_FWD_DEFINED__
#define __IMemAllocator_FWD_DEFINED__
typedef interface IMemAllocator IMemAllocator;
#endif 	/* __IMemAllocator_FWD_DEFINED__ */


#ifndef __IMemInputPin_FWD_DEFINED__
#define __IMemInputPin_FWD_DEFINED__
typedef interface IMemInputPin IMemInputPin;
#endif 	/* __IMemInputPin_FWD_DEFINED__ */


#ifndef __IAMovieSetup_FWD_DEFINED__
#define __IAMovieSetup_FWD_DEFINED__
typedef interface IAMovieSetup IAMovieSetup;
#endif 	/* __IAMovieSetup_FWD_DEFINED__ */


#ifndef __IMediaSelection_FWD_DEFINED__
#define __IMediaSelection_FWD_DEFINED__
typedef interface IMediaSelection IMediaSelection;
#endif 	/* __IMediaSelection_FWD_DEFINED__ */


#ifndef __IMediaSeeking_FWD_DEFINED__
#define __IMediaSeeking_FWD_DEFINED__
typedef interface IMediaSeeking IMediaSeeking;
#endif 	/* __IMediaSeeking_FWD_DEFINED__ */


#ifndef __IEnumRegFilters_FWD_DEFINED__
#define __IEnumRegFilters_FWD_DEFINED__
typedef interface IEnumRegFilters IEnumRegFilters;
#endif 	/* __IEnumRegFilters_FWD_DEFINED__ */


#ifndef __IFilterMapper_FWD_DEFINED__
#define __IFilterMapper_FWD_DEFINED__
typedef interface IFilterMapper IFilterMapper;
#endif 	/* __IFilterMapper_FWD_DEFINED__ */


#ifndef __IQualityControl_FWD_DEFINED__
#define __IQualityControl_FWD_DEFINED__
typedef interface IQualityControl IQualityControl;
#endif 	/* __IQualityControl_FWD_DEFINED__ */


#ifndef __IOverlayNotify_FWD_DEFINED__
#define __IOverlayNotify_FWD_DEFINED__
typedef interface IOverlayNotify IOverlayNotify;
#endif 	/* __IOverlayNotify_FWD_DEFINED__ */


#ifndef __IOverlay_FWD_DEFINED__
#define __IOverlay_FWD_DEFINED__
typedef interface IOverlay IOverlay;
#endif 	/* __IOverlay_FWD_DEFINED__ */


#ifndef __IMediaEventSink_FWD_DEFINED__
#define __IMediaEventSink_FWD_DEFINED__
typedef interface IMediaEventSink IMediaEventSink;
#endif 	/* __IMediaEventSink_FWD_DEFINED__ */


#ifndef __IFileSourceFilter_FWD_DEFINED__
#define __IFileSourceFilter_FWD_DEFINED__
typedef interface IFileSourceFilter IFileSourceFilter;
#endif 	/* __IFileSourceFilter_FWD_DEFINED__ */


#ifndef __IFileSinkFilter_FWD_DEFINED__
#define __IFileSinkFilter_FWD_DEFINED__
typedef interface IFileSinkFilter IFileSinkFilter;
#endif 	/* __IFileSinkFilter_FWD_DEFINED__ */


#ifndef __IFileAsyncIO_FWD_DEFINED__
#define __IFileAsyncIO_FWD_DEFINED__
typedef interface IFileAsyncIO IFileAsyncIO;
#endif 	/* __IFileAsyncIO_FWD_DEFINED__ */


#ifndef __IGraphBuilder_FWD_DEFINED__
#define __IGraphBuilder_FWD_DEFINED__
typedef interface IGraphBuilder IGraphBuilder;
#endif 	/* __IGraphBuilder_FWD_DEFINED__ */


#ifndef __IStreamBuilder_FWD_DEFINED__
#define __IStreamBuilder_FWD_DEFINED__
typedef interface IStreamBuilder IStreamBuilder;
#endif 	/* __IStreamBuilder_FWD_DEFINED__ */


#ifndef __IAsyncReader_FWD_DEFINED__
#define __IAsyncReader_FWD_DEFINED__
typedef interface IAsyncReader IAsyncReader;
#endif 	/* __IAsyncReader_FWD_DEFINED__ */


#ifndef __IGraphVersion_FWD_DEFINED__
#define __IGraphVersion_FWD_DEFINED__
typedef interface IGraphVersion IGraphVersion;
#endif 	/* __IGraphVersion_FWD_DEFINED__ */


#ifndef __IResourceConsumer_FWD_DEFINED__
#define __IResourceConsumer_FWD_DEFINED__
typedef interface IResourceConsumer IResourceConsumer;
#endif 	/* __IResourceConsumer_FWD_DEFINED__ */


#ifndef __IResourceManager_FWD_DEFINED__
#define __IResourceManager_FWD_DEFINED__
typedef interface IResourceManager IResourceManager;
#endif 	/* __IResourceManager_FWD_DEFINED__ */


#ifndef __IDistributorNotify_FWD_DEFINED__
#define __IDistributorNotify_FWD_DEFINED__
typedef interface IDistributorNotify IDistributorNotify;
#endif 	/* __IDistributorNotify_FWD_DEFINED__ */


#ifndef __IAMovie_FWD_DEFINED__
#define __IAMovie_FWD_DEFINED__
typedef interface IAMovie IAMovie;
#endif 	/* __IAMovie_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "objidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


#pragma warning(disable: 4152 4209 4211 4214 4310 4514)
#define CHARS_IN_GUID     39
			/* size is 72 */
typedef struct  _AMMediaType
    {
    GUID majortype;
    GUID subtype;
    BOOL bFixedSizeSamples;
    BOOL bTemporalCompression;
    ULONG lSampleSize;
    GUID formattype;
    IUnknown __RPC_FAR *pUnk;
    ULONG cbFormat;
    /* [size_is] */ BYTE __RPC_FAR *pbFormat;
    }	AM_MEDIA_TYPE;

			/* size is 2 */
typedef 
enum _PinDirection
    {	PINDIR_INPUT	= 0,
	PINDIR_OUTPUT	= PINDIR_INPUT + 1
    }	PIN_DIRECTION;

#define MAX_PIN_NAME     128
#define MAX_FILTER_NAME  128
			/* size is 8 */
typedef LONGLONG REFERENCE_TIME;

			/* size is 8 */
typedef double REFTIME;

			/* size is 8 */
typedef LONGLONG TIME;

			/* size is 4 */
typedef unsigned long HSEMAPHORE;

			/* size is 4 */
typedef unsigned long HEVENT;

			/* size is 16 */
typedef struct  _AllocatorProperties
    {
    long cBuffers;
    long cbBuffer;
    long cbAlign;
    long cbPrefix;
    }	ALLOCATOR_PROPERTIES;

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IPin_INTERFACE_DEFINED__
#define __IPin_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPin
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 262 */
typedef struct  _PinInfo
    {
    IFilter __RPC_FAR *pFilter;
    PIN_DIRECTION dir;
    /* [string] */ WCHAR achName[ 128 ];
    }	PIN_INFO;


EXTERN_C const IID IID_IPin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IPin : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Connect( 
            /* [in] */ IPin __RPC_FAR *pReceivePin,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
        virtual HRESULT __stdcall ReceiveConnection( 
            /* [in] */ IPin __RPC_FAR *pConnector,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
        virtual HRESULT __stdcall Disconnect( void) = 0;
        
        virtual HRESULT __stdcall ConnectedTo( 
            /* [out] */ IPin __RPC_FAR *__RPC_FAR *pPin) = 0;
        
        virtual HRESULT __stdcall ConnectionMediaType( 
            /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
        virtual HRESULT __stdcall QueryPinInfo( 
            /* [out] */ PIN_INFO __RPC_FAR *pInfo) = 0;
        
        virtual HRESULT __stdcall QueryDirection( 
            /* [out] */ PIN_DIRECTION __RPC_FAR *pPinDir) = 0;
        
        virtual HRESULT __stdcall QueryId( 
            /* [out] */ LPWSTR __RPC_FAR *Id) = 0;
        
        virtual HRESULT __stdcall QueryAccept( 
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
        virtual HRESULT __stdcall EnumMediaTypes( 
            /* [out] */ IEnumMediaTypes __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT __stdcall QueryInternalConnections( 
            /* [out] */ IPin __RPC_FAR *__RPC_FAR *apPin,
            /* [out][in] */ ULONG __RPC_FAR *nPin) = 0;
        
        virtual HRESULT __stdcall EndOfStream( void) = 0;
        
        virtual HRESULT __stdcall BeginFlush( void) = 0;
        
        virtual HRESULT __stdcall EndFlush( void) = 0;
        
        virtual HRESULT __stdcall NewSegment( 
            /* [in] */ REFERENCE_TIME tStart,
            /* [in] */ REFERENCE_TIME tStop,
            /* [in] */ double dRate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPinVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IPin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IPin __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IPin __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Connect )( 
            IPin __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *pReceivePin,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *ReceiveConnection )( 
            IPin __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *pConnector,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *Disconnect )( 
            IPin __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *ConnectedTo )( 
            IPin __RPC_FAR * This,
            /* [out] */ IPin __RPC_FAR *__RPC_FAR *pPin);
        
        HRESULT ( __stdcall __RPC_FAR *ConnectionMediaType )( 
            IPin __RPC_FAR * This,
            /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *QueryPinInfo )( 
            IPin __RPC_FAR * This,
            /* [out] */ PIN_INFO __RPC_FAR *pInfo);
        
        HRESULT ( __stdcall __RPC_FAR *QueryDirection )( 
            IPin __RPC_FAR * This,
            /* [out] */ PIN_DIRECTION __RPC_FAR *pPinDir);
        
        HRESULT ( __stdcall __RPC_FAR *QueryId )( 
            IPin __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *Id);
        
        HRESULT ( __stdcall __RPC_FAR *QueryAccept )( 
            IPin __RPC_FAR * This,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *EnumMediaTypes )( 
            IPin __RPC_FAR * This,
            /* [out] */ IEnumMediaTypes __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *QueryInternalConnections )( 
            IPin __RPC_FAR * This,
            /* [out] */ IPin __RPC_FAR *__RPC_FAR *apPin,
            /* [out][in] */ ULONG __RPC_FAR *nPin);
        
        HRESULT ( __stdcall __RPC_FAR *EndOfStream )( 
            IPin __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *BeginFlush )( 
            IPin __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *EndFlush )( 
            IPin __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *NewSegment )( 
            IPin __RPC_FAR * This,
            /* [in] */ REFERENCE_TIME tStart,
            /* [in] */ REFERENCE_TIME tStop,
            /* [in] */ double dRate);
        
    } IPinVtbl;

    interface IPin
    {
        CONST_VTBL struct IPinVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPin_Connect(This,pReceivePin,pmt)	\
    (This)->lpVtbl -> Connect(This,pReceivePin,pmt)

#define IPin_ReceiveConnection(This,pConnector,pmt)	\
    (This)->lpVtbl -> ReceiveConnection(This,pConnector,pmt)

#define IPin_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IPin_ConnectedTo(This,pPin)	\
    (This)->lpVtbl -> ConnectedTo(This,pPin)

#define IPin_ConnectionMediaType(This,pmt)	\
    (This)->lpVtbl -> ConnectionMediaType(This,pmt)

#define IPin_QueryPinInfo(This,pInfo)	\
    (This)->lpVtbl -> QueryPinInfo(This,pInfo)

#define IPin_QueryDirection(This,pPinDir)	\
    (This)->lpVtbl -> QueryDirection(This,pPinDir)

#define IPin_QueryId(This,Id)	\
    (This)->lpVtbl -> QueryId(This,Id)

#define IPin_QueryAccept(This,pmt)	\
    (This)->lpVtbl -> QueryAccept(This,pmt)

#define IPin_EnumMediaTypes(This,ppEnum)	\
    (This)->lpVtbl -> EnumMediaTypes(This,ppEnum)

#define IPin_QueryInternalConnections(This,apPin,nPin)	\
    (This)->lpVtbl -> QueryInternalConnections(This,apPin,nPin)

#define IPin_EndOfStream(This)	\
    (This)->lpVtbl -> EndOfStream(This)

#define IPin_BeginFlush(This)	\
    (This)->lpVtbl -> BeginFlush(This)

#define IPin_EndFlush(This)	\
    (This)->lpVtbl -> EndFlush(This)

#define IPin_NewSegment(This,tStart,tStop,dRate)	\
    (This)->lpVtbl -> NewSegment(This,tStart,tStop,dRate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IPin_Connect_Proxy( 
    IPin __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *pReceivePin,
    /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IPin_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_ReceiveConnection_Proxy( 
    IPin __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *pConnector,
    /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IPin_ReceiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_Disconnect_Proxy( 
    IPin __RPC_FAR * This);


void __RPC_STUB IPin_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_ConnectedTo_Proxy( 
    IPin __RPC_FAR * This,
    /* [out] */ IPin __RPC_FAR *__RPC_FAR *pPin);


void __RPC_STUB IPin_ConnectedTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_ConnectionMediaType_Proxy( 
    IPin __RPC_FAR * This,
    /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IPin_ConnectionMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_QueryPinInfo_Proxy( 
    IPin __RPC_FAR * This,
    /* [out] */ PIN_INFO __RPC_FAR *pInfo);


void __RPC_STUB IPin_QueryPinInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_QueryDirection_Proxy( 
    IPin __RPC_FAR * This,
    /* [out] */ PIN_DIRECTION __RPC_FAR *pPinDir);


void __RPC_STUB IPin_QueryDirection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_QueryId_Proxy( 
    IPin __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *Id);


void __RPC_STUB IPin_QueryId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_QueryAccept_Proxy( 
    IPin __RPC_FAR * This,
    /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IPin_QueryAccept_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_EnumMediaTypes_Proxy( 
    IPin __RPC_FAR * This,
    /* [out] */ IEnumMediaTypes __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IPin_EnumMediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_QueryInternalConnections_Proxy( 
    IPin __RPC_FAR * This,
    /* [out] */ IPin __RPC_FAR *__RPC_FAR *apPin,
    /* [out][in] */ ULONG __RPC_FAR *nPin);


void __RPC_STUB IPin_QueryInternalConnections_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_EndOfStream_Proxy( 
    IPin __RPC_FAR * This);


void __RPC_STUB IPin_EndOfStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_BeginFlush_Proxy( 
    IPin __RPC_FAR * This);


void __RPC_STUB IPin_BeginFlush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_EndFlush_Proxy( 
    IPin __RPC_FAR * This);


void __RPC_STUB IPin_EndFlush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IPin_NewSegment_Proxy( 
    IPin __RPC_FAR * This,
    /* [in] */ REFERENCE_TIME tStart,
    /* [in] */ REFERENCE_TIME tStop,
    /* [in] */ double dRate);


void __RPC_STUB IPin_NewSegment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPin_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0041
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IPin __RPC_FAR *PPIN;



extern RPC_IF_HANDLE __MIDL__intf_0041_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0041_v0_0_s_ifspec;

#ifndef __IEnumPins_INTERFACE_DEFINED__
#define __IEnumPins_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumPins
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumPins;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumPins : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Next( 
            /* [in] */ ULONG cPins,
            /* [size_is][out] */ IPin __RPC_FAR *__RPC_FAR *ppPins,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT __stdcall Skip( 
            /* [in] */ ULONG cPins) = 0;
        
        virtual HRESULT __stdcall Reset( void) = 0;
        
        virtual HRESULT __stdcall Clone( 
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumPinsVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IEnumPins __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IEnumPins __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IEnumPins __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Next )( 
            IEnumPins __RPC_FAR * This,
            /* [in] */ ULONG cPins,
            /* [size_is][out] */ IPin __RPC_FAR *__RPC_FAR *ppPins,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( __stdcall __RPC_FAR *Skip )( 
            IEnumPins __RPC_FAR * This,
            /* [in] */ ULONG cPins);
        
        HRESULT ( __stdcall __RPC_FAR *Reset )( 
            IEnumPins __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Clone )( 
            IEnumPins __RPC_FAR * This,
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);
        
    } IEnumPinsVtbl;

    interface IEnumPins
    {
        CONST_VTBL struct IEnumPinsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumPins_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumPins_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumPins_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumPins_Next(This,cPins,ppPins,pcFetched)	\
    (This)->lpVtbl -> Next(This,cPins,ppPins,pcFetched)

#define IEnumPins_Skip(This,cPins)	\
    (This)->lpVtbl -> Skip(This,cPins)

#define IEnumPins_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumPins_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IEnumPins_Next_Proxy( 
    IEnumPins __RPC_FAR * This,
    /* [in] */ ULONG cPins,
    /* [size_is][out] */ IPin __RPC_FAR *__RPC_FAR *ppPins,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumPins_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumPins_Skip_Proxy( 
    IEnumPins __RPC_FAR * This,
    /* [in] */ ULONG cPins);


void __RPC_STUB IEnumPins_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumPins_Reset_Proxy( 
    IEnumPins __RPC_FAR * This);


void __RPC_STUB IEnumPins_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumPins_Clone_Proxy( 
    IEnumPins __RPC_FAR * This,
    /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumPins_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumPins_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0042
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IEnumPins __RPC_FAR *PENUMPINS;



extern RPC_IF_HANDLE __MIDL__intf_0042_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0042_v0_0_s_ifspec;

#ifndef __IEnumMediaTypes_INTERFACE_DEFINED__
#define __IEnumMediaTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumMediaTypes
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumMediaTypes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumMediaTypes : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Next( 
            /* [in] */ ULONG cMediaTypes,
            /* [size_is][out] */ AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppMediaTypes,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT __stdcall Skip( 
            /* [in] */ ULONG cMediaTypes) = 0;
        
        virtual HRESULT __stdcall Reset( void) = 0;
        
        virtual HRESULT __stdcall Clone( 
            /* [out] */ IEnumMediaTypes __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumMediaTypesVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IEnumMediaTypes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IEnumMediaTypes __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IEnumMediaTypes __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Next )( 
            IEnumMediaTypes __RPC_FAR * This,
            /* [in] */ ULONG cMediaTypes,
            /* [size_is][out] */ AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppMediaTypes,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( __stdcall __RPC_FAR *Skip )( 
            IEnumMediaTypes __RPC_FAR * This,
            /* [in] */ ULONG cMediaTypes);
        
        HRESULT ( __stdcall __RPC_FAR *Reset )( 
            IEnumMediaTypes __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Clone )( 
            IEnumMediaTypes __RPC_FAR * This,
            /* [out] */ IEnumMediaTypes __RPC_FAR *__RPC_FAR *ppEnum);
        
    } IEnumMediaTypesVtbl;

    interface IEnumMediaTypes
    {
        CONST_VTBL struct IEnumMediaTypesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumMediaTypes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumMediaTypes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumMediaTypes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumMediaTypes_Next(This,cMediaTypes,ppMediaTypes,pcFetched)	\
    (This)->lpVtbl -> Next(This,cMediaTypes,ppMediaTypes,pcFetched)

#define IEnumMediaTypes_Skip(This,cMediaTypes)	\
    (This)->lpVtbl -> Skip(This,cMediaTypes)

#define IEnumMediaTypes_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumMediaTypes_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IEnumMediaTypes_Next_Proxy( 
    IEnumMediaTypes __RPC_FAR * This,
    /* [in] */ ULONG cMediaTypes,
    /* [size_is][out] */ AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppMediaTypes,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumMediaTypes_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumMediaTypes_Skip_Proxy( 
    IEnumMediaTypes __RPC_FAR * This,
    /* [in] */ ULONG cMediaTypes);


void __RPC_STUB IEnumMediaTypes_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumMediaTypes_Reset_Proxy( 
    IEnumMediaTypes __RPC_FAR * This);


void __RPC_STUB IEnumMediaTypes_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumMediaTypes_Clone_Proxy( 
    IEnumMediaTypes __RPC_FAR * This,
    /* [out] */ IEnumMediaTypes __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumMediaTypes_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumMediaTypes_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0043
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IEnumMediaTypes __RPC_FAR *PENUMMEDIATYPES;



extern RPC_IF_HANDLE __MIDL__intf_0043_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0043_v0_0_s_ifspec;

#ifndef __IFilterGraph_INTERFACE_DEFINED__
#define __IFilterGraph_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFilterGraph
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IFilterGraph;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IFilterGraph : public IUnknown
    {
    public:
        virtual HRESULT __stdcall AddFilter( 
            /* [in] */ IFilter __RPC_FAR *pFilter,
            /* [string][in] */ LPCWSTR pName) = 0;
        
        virtual HRESULT __stdcall RemoveFilter( 
            /* [in] */ IFilter __RPC_FAR *pFilter) = 0;
        
        virtual HRESULT __stdcall EnumFilters( 
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT __stdcall FindFilterByName( 
            /* [string][in] */ LPCWSTR pName,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter) = 0;
        
        virtual HRESULT __stdcall ConnectDirect( 
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
        virtual HRESULT __stdcall Reconnect( 
            /* [in] */ IPin __RPC_FAR *ppin) = 0;
        
        virtual HRESULT __stdcall Disconnect( 
            /* [in] */ IPin __RPC_FAR *ppin) = 0;
        
        virtual HRESULT __stdcall SetDefaultSyncSource( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFilterGraphVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IFilterGraph __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IFilterGraph __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IFilterGraph __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *AddFilter )( 
            IFilterGraph __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pFilter,
            /* [string][in] */ LPCWSTR pName);
        
        HRESULT ( __stdcall __RPC_FAR *RemoveFilter )( 
            IFilterGraph __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pFilter);
        
        HRESULT ( __stdcall __RPC_FAR *EnumFilters )( 
            IFilterGraph __RPC_FAR * This,
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *FindFilterByName )( 
            IFilterGraph __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pName,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter);
        
        HRESULT ( __stdcall __RPC_FAR *ConnectDirect )( 
            IFilterGraph __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *Reconnect )( 
            IFilterGraph __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppin);
        
        HRESULT ( __stdcall __RPC_FAR *Disconnect )( 
            IFilterGraph __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppin);
        
        HRESULT ( __stdcall __RPC_FAR *SetDefaultSyncSource )( 
            IFilterGraph __RPC_FAR * This);
        
    } IFilterGraphVtbl;

    interface IFilterGraph
    {
        CONST_VTBL struct IFilterGraphVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFilterGraph_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFilterGraph_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFilterGraph_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFilterGraph_AddFilter(This,pFilter,pName)	\
    (This)->lpVtbl -> AddFilter(This,pFilter,pName)

#define IFilterGraph_RemoveFilter(This,pFilter)	\
    (This)->lpVtbl -> RemoveFilter(This,pFilter)

#define IFilterGraph_EnumFilters(This,ppEnum)	\
    (This)->lpVtbl -> EnumFilters(This,ppEnum)

#define IFilterGraph_FindFilterByName(This,pName,ppFilter)	\
    (This)->lpVtbl -> FindFilterByName(This,pName,ppFilter)

#define IFilterGraph_ConnectDirect(This,ppinOut,ppinIn,pmt)	\
    (This)->lpVtbl -> ConnectDirect(This,ppinOut,ppinIn,pmt)

#define IFilterGraph_Reconnect(This,ppin)	\
    (This)->lpVtbl -> Reconnect(This,ppin)

#define IFilterGraph_Disconnect(This,ppin)	\
    (This)->lpVtbl -> Disconnect(This,ppin)

#define IFilterGraph_SetDefaultSyncSource(This)	\
    (This)->lpVtbl -> SetDefaultSyncSource(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IFilterGraph_AddFilter_Proxy( 
    IFilterGraph __RPC_FAR * This,
    /* [in] */ IFilter __RPC_FAR *pFilter,
    /* [string][in] */ LPCWSTR pName);


void __RPC_STUB IFilterGraph_AddFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterGraph_RemoveFilter_Proxy( 
    IFilterGraph __RPC_FAR * This,
    /* [in] */ IFilter __RPC_FAR *pFilter);


void __RPC_STUB IFilterGraph_RemoveFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterGraph_EnumFilters_Proxy( 
    IFilterGraph __RPC_FAR * This,
    /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IFilterGraph_EnumFilters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterGraph_FindFilterByName_Proxy( 
    IFilterGraph __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pName,
    /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter);


void __RPC_STUB IFilterGraph_FindFilterByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterGraph_ConnectDirect_Proxy( 
    IFilterGraph __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppinOut,
    /* [in] */ IPin __RPC_FAR *ppinIn,
    /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IFilterGraph_ConnectDirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterGraph_Reconnect_Proxy( 
    IFilterGraph __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppin);


void __RPC_STUB IFilterGraph_Reconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterGraph_Disconnect_Proxy( 
    IFilterGraph __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppin);


void __RPC_STUB IFilterGraph_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterGraph_SetDefaultSyncSource_Proxy( 
    IFilterGraph __RPC_FAR * This);


void __RPC_STUB IFilterGraph_SetDefaultSyncSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFilterGraph_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0044
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IFilterGraph __RPC_FAR *PFILTERGRAPH;



extern RPC_IF_HANDLE __MIDL__intf_0044_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0044_v0_0_s_ifspec;

#ifndef __IEnumFilters_INTERFACE_DEFINED__
#define __IEnumFilters_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumFilters
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumFilters;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumFilters : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Next( 
            /* [in] */ ULONG cFilters,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT __stdcall Skip( 
            /* [in] */ ULONG cFilters) = 0;
        
        virtual HRESULT __stdcall Reset( void) = 0;
        
        virtual HRESULT __stdcall Clone( 
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumFiltersVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IEnumFilters __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IEnumFilters __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IEnumFilters __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Next )( 
            IEnumFilters __RPC_FAR * This,
            /* [in] */ ULONG cFilters,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( __stdcall __RPC_FAR *Skip )( 
            IEnumFilters __RPC_FAR * This,
            /* [in] */ ULONG cFilters);
        
        HRESULT ( __stdcall __RPC_FAR *Reset )( 
            IEnumFilters __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Clone )( 
            IEnumFilters __RPC_FAR * This,
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);
        
    } IEnumFiltersVtbl;

    interface IEnumFilters
    {
        CONST_VTBL struct IEnumFiltersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumFilters_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumFilters_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumFilters_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumFilters_Next(This,cFilters,ppFilter,pcFetched)	\
    (This)->lpVtbl -> Next(This,cFilters,ppFilter,pcFetched)

#define IEnumFilters_Skip(This,cFilters)	\
    (This)->lpVtbl -> Skip(This,cFilters)

#define IEnumFilters_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumFilters_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IEnumFilters_Next_Proxy( 
    IEnumFilters __RPC_FAR * This,
    /* [in] */ ULONG cFilters,
    /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumFilters_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumFilters_Skip_Proxy( 
    IEnumFilters __RPC_FAR * This,
    /* [in] */ ULONG cFilters);


void __RPC_STUB IEnumFilters_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumFilters_Reset_Proxy( 
    IEnumFilters __RPC_FAR * This);


void __RPC_STUB IEnumFilters_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumFilters_Clone_Proxy( 
    IEnumFilters __RPC_FAR * This,
    /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumFilters_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumFilters_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0045
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IEnumFilters __RPC_FAR *PENUMFILTERS;



extern RPC_IF_HANDLE __MIDL__intf_0045_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0045_v0_0_s_ifspec;

#ifndef __IMediaFilter_INTERFACE_DEFINED__
#define __IMediaFilter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMediaFilter
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 2 */
typedef 
enum _FilterState
    {	State_Stopped	= 0,
	State_Paused	= State_Stopped + 1,
	State_Running	= State_Paused + 1
    }	FILTER_STATE;


EXTERN_C const IID IID_IMediaFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMediaFilter : public IPersist
    {
    public:
        virtual HRESULT __stdcall Stop( void) = 0;
        
        virtual HRESULT __stdcall Pause( void) = 0;
        
        virtual HRESULT __stdcall Run( 
            REFERENCE_TIME tStart) = 0;
        
        virtual HRESULT __stdcall GetState( 
            /* [in] */ DWORD dwMilliSecsTimeout,
            /* [out] */ FILTER_STATE __RPC_FAR *State) = 0;
        
        virtual HRESULT __stdcall SetSyncSource( 
            /* [in] */ IReferenceClock __RPC_FAR *pClock) = 0;
        
        virtual HRESULT __stdcall GetSyncSource( 
            /* [out] */ IReferenceClock __RPC_FAR *__RPC_FAR *pClock) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMediaFilterVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMediaFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMediaFilter __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMediaFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetClassID )( 
            IMediaFilter __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
        HRESULT ( __stdcall __RPC_FAR *Stop )( 
            IMediaFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Pause )( 
            IMediaFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Run )( 
            IMediaFilter __RPC_FAR * This,
            REFERENCE_TIME tStart);
        
        HRESULT ( __stdcall __RPC_FAR *GetState )( 
            IMediaFilter __RPC_FAR * This,
            /* [in] */ DWORD dwMilliSecsTimeout,
            /* [out] */ FILTER_STATE __RPC_FAR *State);
        
        HRESULT ( __stdcall __RPC_FAR *SetSyncSource )( 
            IMediaFilter __RPC_FAR * This,
            /* [in] */ IReferenceClock __RPC_FAR *pClock);
        
        HRESULT ( __stdcall __RPC_FAR *GetSyncSource )( 
            IMediaFilter __RPC_FAR * This,
            /* [out] */ IReferenceClock __RPC_FAR *__RPC_FAR *pClock);
        
    } IMediaFilterVtbl;

    interface IMediaFilter
    {
        CONST_VTBL struct IMediaFilterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMediaFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMediaFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMediaFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMediaFilter_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IMediaFilter_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IMediaFilter_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IMediaFilter_Run(This,tStart)	\
    (This)->lpVtbl -> Run(This,tStart)

#define IMediaFilter_GetState(This,dwMilliSecsTimeout,State)	\
    (This)->lpVtbl -> GetState(This,dwMilliSecsTimeout,State)

#define IMediaFilter_SetSyncSource(This,pClock)	\
    (This)->lpVtbl -> SetSyncSource(This,pClock)

#define IMediaFilter_GetSyncSource(This,pClock)	\
    (This)->lpVtbl -> GetSyncSource(This,pClock)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMediaFilter_Stop_Proxy( 
    IMediaFilter __RPC_FAR * This);


void __RPC_STUB IMediaFilter_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaFilter_Pause_Proxy( 
    IMediaFilter __RPC_FAR * This);


void __RPC_STUB IMediaFilter_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaFilter_Run_Proxy( 
    IMediaFilter __RPC_FAR * This,
    REFERENCE_TIME tStart);


void __RPC_STUB IMediaFilter_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaFilter_GetState_Proxy( 
    IMediaFilter __RPC_FAR * This,
    /* [in] */ DWORD dwMilliSecsTimeout,
    /* [out] */ FILTER_STATE __RPC_FAR *State);


void __RPC_STUB IMediaFilter_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaFilter_SetSyncSource_Proxy( 
    IMediaFilter __RPC_FAR * This,
    /* [in] */ IReferenceClock __RPC_FAR *pClock);


void __RPC_STUB IMediaFilter_SetSyncSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaFilter_GetSyncSource_Proxy( 
    IMediaFilter __RPC_FAR * This,
    /* [out] */ IReferenceClock __RPC_FAR *__RPC_FAR *pClock);


void __RPC_STUB IMediaFilter_GetSyncSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMediaFilter_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0046
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IMediaFilter __RPC_FAR *PMEDIAFILTER;



extern RPC_IF_HANDLE __MIDL__intf_0046_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0046_v0_0_s_ifspec;

#ifndef __IFilter_INTERFACE_DEFINED__
#define __IFilter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFilter
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 260 */
typedef struct  _FilterInfo
    {
    /* [string] */ WCHAR achName[ 128 ];
    IFilterGraph __RPC_FAR *pGraph;
    }	FILTER_INFO;


EXTERN_C const IID IID_IFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IFilter : public IMediaFilter
    {
    public:
        virtual HRESULT __stdcall EnumPins( 
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT __stdcall FindPin( 
            /* [string][in] */ LPCWSTR Id,
            /* [out] */ IPin __RPC_FAR *__RPC_FAR *ppPin) = 0;
        
        virtual HRESULT __stdcall QueryFilterInfo( 
            /* [out] */ FILTER_INFO __RPC_FAR *pInfo) = 0;
        
        virtual HRESULT __stdcall JoinFilterGraph( 
            /* [in] */ IFilterGraph __RPC_FAR *pGraph,
            /* [string][in] */ LPCWSTR pName) = 0;
        
        virtual HRESULT __stdcall QueryVendorInfo( 
            /* [string][out] */ LPWSTR __RPC_FAR *pVendorInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFilterVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IFilter __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetClassID )( 
            IFilter __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
        HRESULT ( __stdcall __RPC_FAR *Stop )( 
            IFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Pause )( 
            IFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Run )( 
            IFilter __RPC_FAR * This,
            REFERENCE_TIME tStart);
        
        HRESULT ( __stdcall __RPC_FAR *GetState )( 
            IFilter __RPC_FAR * This,
            /* [in] */ DWORD dwMilliSecsTimeout,
            /* [out] */ FILTER_STATE __RPC_FAR *State);
        
        HRESULT ( __stdcall __RPC_FAR *SetSyncSource )( 
            IFilter __RPC_FAR * This,
            /* [in] */ IReferenceClock __RPC_FAR *pClock);
        
        HRESULT ( __stdcall __RPC_FAR *GetSyncSource )( 
            IFilter __RPC_FAR * This,
            /* [out] */ IReferenceClock __RPC_FAR *__RPC_FAR *pClock);
        
        HRESULT ( __stdcall __RPC_FAR *EnumPins )( 
            IFilter __RPC_FAR * This,
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *FindPin )( 
            IFilter __RPC_FAR * This,
            /* [string][in] */ LPCWSTR Id,
            /* [out] */ IPin __RPC_FAR *__RPC_FAR *ppPin);
        
        HRESULT ( __stdcall __RPC_FAR *QueryFilterInfo )( 
            IFilter __RPC_FAR * This,
            /* [out] */ FILTER_INFO __RPC_FAR *pInfo);
        
        HRESULT ( __stdcall __RPC_FAR *JoinFilterGraph )( 
            IFilter __RPC_FAR * This,
            /* [in] */ IFilterGraph __RPC_FAR *pGraph,
            /* [string][in] */ LPCWSTR pName);
        
        HRESULT ( __stdcall __RPC_FAR *QueryVendorInfo )( 
            IFilter __RPC_FAR * This,
            /* [string][out] */ LPWSTR __RPC_FAR *pVendorInfo);
        
    } IFilterVtbl;

    interface IFilter
    {
        CONST_VTBL struct IFilterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFilter_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IFilter_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IFilter_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IFilter_Run(This,tStart)	\
    (This)->lpVtbl -> Run(This,tStart)

#define IFilter_GetState(This,dwMilliSecsTimeout,State)	\
    (This)->lpVtbl -> GetState(This,dwMilliSecsTimeout,State)

#define IFilter_SetSyncSource(This,pClock)	\
    (This)->lpVtbl -> SetSyncSource(This,pClock)

#define IFilter_GetSyncSource(This,pClock)	\
    (This)->lpVtbl -> GetSyncSource(This,pClock)


#define IFilter_EnumPins(This,ppEnum)	\
    (This)->lpVtbl -> EnumPins(This,ppEnum)

#define IFilter_FindPin(This,Id,ppPin)	\
    (This)->lpVtbl -> FindPin(This,Id,ppPin)

#define IFilter_QueryFilterInfo(This,pInfo)	\
    (This)->lpVtbl -> QueryFilterInfo(This,pInfo)

#define IFilter_JoinFilterGraph(This,pGraph,pName)	\
    (This)->lpVtbl -> JoinFilterGraph(This,pGraph,pName)

#define IFilter_QueryVendorInfo(This,pVendorInfo)	\
    (This)->lpVtbl -> QueryVendorInfo(This,pVendorInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IFilter_EnumPins_Proxy( 
    IFilter __RPC_FAR * This,
    /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IFilter_EnumPins_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilter_FindPin_Proxy( 
    IFilter __RPC_FAR * This,
    /* [string][in] */ LPCWSTR Id,
    /* [out] */ IPin __RPC_FAR *__RPC_FAR *ppPin);


void __RPC_STUB IFilter_FindPin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilter_QueryFilterInfo_Proxy( 
    IFilter __RPC_FAR * This,
    /* [out] */ FILTER_INFO __RPC_FAR *pInfo);


void __RPC_STUB IFilter_QueryFilterInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilter_JoinFilterGraph_Proxy( 
    IFilter __RPC_FAR * This,
    /* [in] */ IFilterGraph __RPC_FAR *pGraph,
    /* [string][in] */ LPCWSTR pName);


void __RPC_STUB IFilter_JoinFilterGraph_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilter_QueryVendorInfo_Proxy( 
    IFilter __RPC_FAR * This,
    /* [string][out] */ LPWSTR __RPC_FAR *pVendorInfo);


void __RPC_STUB IFilter_QueryVendorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFilter_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0047
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IFilter __RPC_FAR *PFILTER;



extern RPC_IF_HANDLE __MIDL__intf_0047_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0047_v0_0_s_ifspec;

#ifndef __IReferenceClock_INTERFACE_DEFINED__
#define __IReferenceClock_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IReferenceClock
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IReferenceClock;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IReferenceClock : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetTime( 
            /* [out] */ REFERENCE_TIME __RPC_FAR *pTime) = 0;
        
        virtual HRESULT __stdcall AdviseTime( 
            /* [in] */ REFERENCE_TIME baseTime,
            /* [in] */ REFERENCE_TIME streamTime,
            /* [in] */ HEVENT hEvent,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie) = 0;
        
        virtual HRESULT __stdcall AdvisePeriodic( 
            /* [in] */ REFERENCE_TIME startTime,
            /* [in] */ REFERENCE_TIME periodTime,
            /* [in] */ HSEMAPHORE hSemaphore,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie) = 0;
        
        virtual HRESULT __stdcall Unadvise( 
            /* [in] */ DWORD dwAdviseCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReferenceClockVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IReferenceClock __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IReferenceClock __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetTime )( 
            IReferenceClock __RPC_FAR * This,
            /* [out] */ REFERENCE_TIME __RPC_FAR *pTime);
        
        HRESULT ( __stdcall __RPC_FAR *AdviseTime )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ REFERENCE_TIME baseTime,
            /* [in] */ REFERENCE_TIME streamTime,
            /* [in] */ HEVENT hEvent,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
        
        HRESULT ( __stdcall __RPC_FAR *AdvisePeriodic )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ REFERENCE_TIME startTime,
            /* [in] */ REFERENCE_TIME periodTime,
            /* [in] */ HSEMAPHORE hSemaphore,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
        
        HRESULT ( __stdcall __RPC_FAR *Unadvise )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ DWORD dwAdviseCookie);
        
    } IReferenceClockVtbl;

    interface IReferenceClock
    {
        CONST_VTBL struct IReferenceClockVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReferenceClock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReferenceClock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReferenceClock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReferenceClock_GetTime(This,pTime)	\
    (This)->lpVtbl -> GetTime(This,pTime)

#define IReferenceClock_AdviseTime(This,baseTime,streamTime,hEvent,pdwAdviseCookie)	\
    (This)->lpVtbl -> AdviseTime(This,baseTime,streamTime,hEvent,pdwAdviseCookie)

#define IReferenceClock_AdvisePeriodic(This,startTime,periodTime,hSemaphore,pdwAdviseCookie)	\
    (This)->lpVtbl -> AdvisePeriodic(This,startTime,periodTime,hSemaphore,pdwAdviseCookie)

#define IReferenceClock_Unadvise(This,dwAdviseCookie)	\
    (This)->lpVtbl -> Unadvise(This,dwAdviseCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IReferenceClock_GetTime_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [out] */ REFERENCE_TIME __RPC_FAR *pTime);


void __RPC_STUB IReferenceClock_GetTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IReferenceClock_AdviseTime_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [in] */ REFERENCE_TIME baseTime,
    /* [in] */ REFERENCE_TIME streamTime,
    /* [in] */ HEVENT hEvent,
    /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);


void __RPC_STUB IReferenceClock_AdviseTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IReferenceClock_AdvisePeriodic_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [in] */ REFERENCE_TIME startTime,
    /* [in] */ REFERENCE_TIME periodTime,
    /* [in] */ HSEMAPHORE hSemaphore,
    /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);


void __RPC_STUB IReferenceClock_AdvisePeriodic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IReferenceClock_Unadvise_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [in] */ DWORD dwAdviseCookie);


void __RPC_STUB IReferenceClock_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReferenceClock_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0048
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IReferenceClock __RPC_FAR *PREFERENCECLOCK;



extern RPC_IF_HANDLE __MIDL__intf_0048_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0048_v0_0_s_ifspec;

#ifndef __IMediaSample_INTERFACE_DEFINED__
#define __IMediaSample_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMediaSample
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IMediaSample;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMediaSample : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetPointer( 
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppBuffer) = 0;
        
        virtual long __stdcall GetSize( void) = 0;
        
        virtual HRESULT __stdcall GetTime( 
            /* [out] */ REFERENCE_TIME __RPC_FAR *pTimeStart,
            /* [out] */ REFERENCE_TIME __RPC_FAR *pTimeEnd) = 0;
        
        virtual HRESULT __stdcall SetTime( 
            /* [in] */ REFERENCE_TIME __RPC_FAR *pTimeStart,
            /* [in] */ REFERENCE_TIME __RPC_FAR *pTimeEnd) = 0;
        
        virtual HRESULT __stdcall IsSyncPoint( void) = 0;
        
        virtual HRESULT __stdcall SetSyncPoint( 
            BOOL bIsSyncPoint) = 0;
        
        virtual HRESULT __stdcall IsPreroll( void) = 0;
        
        virtual HRESULT __stdcall SetPreroll( 
            BOOL bIsPreroll) = 0;
        
        virtual long __stdcall GetActualDataLength( void) = 0;
        
        virtual HRESULT __stdcall SetActualDataLength( 
            long __MIDL_0002) = 0;
        
        virtual HRESULT __stdcall GetMediaType( 
            AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppMediaType) = 0;
        
        virtual HRESULT __stdcall SetMediaType( 
            AM_MEDIA_TYPE __RPC_FAR *pMediaType) = 0;
        
        virtual HRESULT __stdcall IsDiscontinuity( void) = 0;
        
        virtual HRESULT __stdcall SetDiscontinuity( 
            BOOL bDiscontinuity) = 0;
        
        virtual HRESULT __stdcall GetMediaTime( 
            /* [out] */ LONGLONG __RPC_FAR *pTimeStart,
            /* [out] */ LONGLONG __RPC_FAR *pTimeEnd) = 0;
        
        virtual HRESULT __stdcall SetMediaTime( 
            /* [in] */ LONGLONG __RPC_FAR *pTimeStart,
            /* [in] */ LONGLONG __RPC_FAR *pTimeEnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMediaSampleVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMediaSample __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMediaSample __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMediaSample __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetPointer )( 
            IMediaSample __RPC_FAR * This,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppBuffer);
        
        long ( __stdcall __RPC_FAR *GetSize )( 
            IMediaSample __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetTime )( 
            IMediaSample __RPC_FAR * This,
            /* [out] */ REFERENCE_TIME __RPC_FAR *pTimeStart,
            /* [out] */ REFERENCE_TIME __RPC_FAR *pTimeEnd);
        
        HRESULT ( __stdcall __RPC_FAR *SetTime )( 
            IMediaSample __RPC_FAR * This,
            /* [in] */ REFERENCE_TIME __RPC_FAR *pTimeStart,
            /* [in] */ REFERENCE_TIME __RPC_FAR *pTimeEnd);
        
        HRESULT ( __stdcall __RPC_FAR *IsSyncPoint )( 
            IMediaSample __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *SetSyncPoint )( 
            IMediaSample __RPC_FAR * This,
            BOOL bIsSyncPoint);
        
        HRESULT ( __stdcall __RPC_FAR *IsPreroll )( 
            IMediaSample __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *SetPreroll )( 
            IMediaSample __RPC_FAR * This,
            BOOL bIsPreroll);
        
        long ( __stdcall __RPC_FAR *GetActualDataLength )( 
            IMediaSample __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *SetActualDataLength )( 
            IMediaSample __RPC_FAR * This,
            long __MIDL_0002);
        
        HRESULT ( __stdcall __RPC_FAR *GetMediaType )( 
            IMediaSample __RPC_FAR * This,
            AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppMediaType);
        
        HRESULT ( __stdcall __RPC_FAR *SetMediaType )( 
            IMediaSample __RPC_FAR * This,
            AM_MEDIA_TYPE __RPC_FAR *pMediaType);
        
        HRESULT ( __stdcall __RPC_FAR *IsDiscontinuity )( 
            IMediaSample __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *SetDiscontinuity )( 
            IMediaSample __RPC_FAR * This,
            BOOL bDiscontinuity);
        
        HRESULT ( __stdcall __RPC_FAR *GetMediaTime )( 
            IMediaSample __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pTimeStart,
            /* [out] */ LONGLONG __RPC_FAR *pTimeEnd);
        
        HRESULT ( __stdcall __RPC_FAR *SetMediaTime )( 
            IMediaSample __RPC_FAR * This,
            /* [in] */ LONGLONG __RPC_FAR *pTimeStart,
            /* [in] */ LONGLONG __RPC_FAR *pTimeEnd);
        
    } IMediaSampleVtbl;

    interface IMediaSample
    {
        CONST_VTBL struct IMediaSampleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMediaSample_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMediaSample_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMediaSample_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMediaSample_GetPointer(This,ppBuffer)	\
    (This)->lpVtbl -> GetPointer(This,ppBuffer)

#define IMediaSample_GetSize(This)	\
    (This)->lpVtbl -> GetSize(This)

#define IMediaSample_GetTime(This,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> GetTime(This,pTimeStart,pTimeEnd)

#define IMediaSample_SetTime(This,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> SetTime(This,pTimeStart,pTimeEnd)

#define IMediaSample_IsSyncPoint(This)	\
    (This)->lpVtbl -> IsSyncPoint(This)

#define IMediaSample_SetSyncPoint(This,bIsSyncPoint)	\
    (This)->lpVtbl -> SetSyncPoint(This,bIsSyncPoint)

#define IMediaSample_IsPreroll(This)	\
    (This)->lpVtbl -> IsPreroll(This)

#define IMediaSample_SetPreroll(This,bIsPreroll)	\
    (This)->lpVtbl -> SetPreroll(This,bIsPreroll)

#define IMediaSample_GetActualDataLength(This)	\
    (This)->lpVtbl -> GetActualDataLength(This)

#define IMediaSample_SetActualDataLength(This,__MIDL_0002)	\
    (This)->lpVtbl -> SetActualDataLength(This,__MIDL_0002)

#define IMediaSample_GetMediaType(This,ppMediaType)	\
    (This)->lpVtbl -> GetMediaType(This,ppMediaType)

#define IMediaSample_SetMediaType(This,pMediaType)	\
    (This)->lpVtbl -> SetMediaType(This,pMediaType)

#define IMediaSample_IsDiscontinuity(This)	\
    (This)->lpVtbl -> IsDiscontinuity(This)

#define IMediaSample_SetDiscontinuity(This,bDiscontinuity)	\
    (This)->lpVtbl -> SetDiscontinuity(This,bDiscontinuity)

#define IMediaSample_GetMediaTime(This,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> GetMediaTime(This,pTimeStart,pTimeEnd)

#define IMediaSample_SetMediaTime(This,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> SetMediaTime(This,pTimeStart,pTimeEnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMediaSample_GetPointer_Proxy( 
    IMediaSample __RPC_FAR * This,
    /* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppBuffer);


void __RPC_STUB IMediaSample_GetPointer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


long __stdcall IMediaSample_GetSize_Proxy( 
    IMediaSample __RPC_FAR * This);


void __RPC_STUB IMediaSample_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_GetTime_Proxy( 
    IMediaSample __RPC_FAR * This,
    /* [out] */ REFERENCE_TIME __RPC_FAR *pTimeStart,
    /* [out] */ REFERENCE_TIME __RPC_FAR *pTimeEnd);


void __RPC_STUB IMediaSample_GetTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_SetTime_Proxy( 
    IMediaSample __RPC_FAR * This,
    /* [in] */ REFERENCE_TIME __RPC_FAR *pTimeStart,
    /* [in] */ REFERENCE_TIME __RPC_FAR *pTimeEnd);


void __RPC_STUB IMediaSample_SetTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_IsSyncPoint_Proxy( 
    IMediaSample __RPC_FAR * This);


void __RPC_STUB IMediaSample_IsSyncPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_SetSyncPoint_Proxy( 
    IMediaSample __RPC_FAR * This,
    BOOL bIsSyncPoint);


void __RPC_STUB IMediaSample_SetSyncPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_IsPreroll_Proxy( 
    IMediaSample __RPC_FAR * This);


void __RPC_STUB IMediaSample_IsPreroll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_SetPreroll_Proxy( 
    IMediaSample __RPC_FAR * This,
    BOOL bIsPreroll);


void __RPC_STUB IMediaSample_SetPreroll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


long __stdcall IMediaSample_GetActualDataLength_Proxy( 
    IMediaSample __RPC_FAR * This);


void __RPC_STUB IMediaSample_GetActualDataLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_SetActualDataLength_Proxy( 
    IMediaSample __RPC_FAR * This,
    long __MIDL_0002);


void __RPC_STUB IMediaSample_SetActualDataLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_GetMediaType_Proxy( 
    IMediaSample __RPC_FAR * This,
    AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppMediaType);


void __RPC_STUB IMediaSample_GetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_SetMediaType_Proxy( 
    IMediaSample __RPC_FAR * This,
    AM_MEDIA_TYPE __RPC_FAR *pMediaType);


void __RPC_STUB IMediaSample_SetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_IsDiscontinuity_Proxy( 
    IMediaSample __RPC_FAR * This);


void __RPC_STUB IMediaSample_IsDiscontinuity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_SetDiscontinuity_Proxy( 
    IMediaSample __RPC_FAR * This,
    BOOL bDiscontinuity);


void __RPC_STUB IMediaSample_SetDiscontinuity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_GetMediaTime_Proxy( 
    IMediaSample __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pTimeStart,
    /* [out] */ LONGLONG __RPC_FAR *pTimeEnd);


void __RPC_STUB IMediaSample_GetMediaTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSample_SetMediaTime_Proxy( 
    IMediaSample __RPC_FAR * This,
    /* [in] */ LONGLONG __RPC_FAR *pTimeStart,
    /* [in] */ LONGLONG __RPC_FAR *pTimeEnd);


void __RPC_STUB IMediaSample_SetMediaTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMediaSample_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0049
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IMediaSample __RPC_FAR *PMEDIASAMPLE;

#define AM_GBF_PREVFRAMESKIPPED 1
#define AM_GBF_NOTASYNCPOINT 2


extern RPC_IF_HANDLE __MIDL__intf_0049_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0049_v0_0_s_ifspec;

#ifndef __IMemAllocator_INTERFACE_DEFINED__
#define __IMemAllocator_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMemAllocator
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMemAllocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMemAllocator : public IUnknown
    {
    public:
        virtual HRESULT __stdcall SetProperties( 
            /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pRequest,
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pActual) = 0;
        
        virtual HRESULT __stdcall GetProperties( 
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps) = 0;
        
        virtual HRESULT __stdcall Commit( void) = 0;
        
        virtual HRESULT __stdcall Decommit( void) = 0;
        
        virtual HRESULT __stdcall GetBuffer( 
            /* [out] */ IMediaSample __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ REFERENCE_TIME __RPC_FAR *pStartTime,
            /* [in] */ REFERENCE_TIME __RPC_FAR *pEndTime,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT __stdcall ReleaseBuffer( 
            /* [in] */ IMediaSample __RPC_FAR *pBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMemAllocatorVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMemAllocator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMemAllocator __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMemAllocator __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *SetProperties )( 
            IMemAllocator __RPC_FAR * This,
            /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pRequest,
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pActual);
        
        HRESULT ( __stdcall __RPC_FAR *GetProperties )( 
            IMemAllocator __RPC_FAR * This,
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps);
        
        HRESULT ( __stdcall __RPC_FAR *Commit )( 
            IMemAllocator __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Decommit )( 
            IMemAllocator __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetBuffer )( 
            IMemAllocator __RPC_FAR * This,
            /* [out] */ IMediaSample __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ REFERENCE_TIME __RPC_FAR *pStartTime,
            /* [in] */ REFERENCE_TIME __RPC_FAR *pEndTime,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( __stdcall __RPC_FAR *ReleaseBuffer )( 
            IMemAllocator __RPC_FAR * This,
            /* [in] */ IMediaSample __RPC_FAR *pBuffer);
        
    } IMemAllocatorVtbl;

    interface IMemAllocator
    {
        CONST_VTBL struct IMemAllocatorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMemAllocator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMemAllocator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMemAllocator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMemAllocator_SetProperties(This,pRequest,pActual)	\
    (This)->lpVtbl -> SetProperties(This,pRequest,pActual)

#define IMemAllocator_GetProperties(This,pProps)	\
    (This)->lpVtbl -> GetProperties(This,pProps)

#define IMemAllocator_Commit(This)	\
    (This)->lpVtbl -> Commit(This)

#define IMemAllocator_Decommit(This)	\
    (This)->lpVtbl -> Decommit(This)

#define IMemAllocator_GetBuffer(This,ppBuffer,pStartTime,pEndTime,dwFlags)	\
    (This)->lpVtbl -> GetBuffer(This,ppBuffer,pStartTime,pEndTime,dwFlags)

#define IMemAllocator_ReleaseBuffer(This,pBuffer)	\
    (This)->lpVtbl -> ReleaseBuffer(This,pBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMemAllocator_SetProperties_Proxy( 
    IMemAllocator __RPC_FAR * This,
    /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pRequest,
    /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pActual);


void __RPC_STUB IMemAllocator_SetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemAllocator_GetProperties_Proxy( 
    IMemAllocator __RPC_FAR * This,
    /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps);


void __RPC_STUB IMemAllocator_GetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemAllocator_Commit_Proxy( 
    IMemAllocator __RPC_FAR * This);


void __RPC_STUB IMemAllocator_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemAllocator_Decommit_Proxy( 
    IMemAllocator __RPC_FAR * This);


void __RPC_STUB IMemAllocator_Decommit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemAllocator_GetBuffer_Proxy( 
    IMemAllocator __RPC_FAR * This,
    /* [out] */ IMediaSample __RPC_FAR *__RPC_FAR *ppBuffer,
    /* [in] */ REFERENCE_TIME __RPC_FAR *pStartTime,
    /* [in] */ REFERENCE_TIME __RPC_FAR *pEndTime,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IMemAllocator_GetBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemAllocator_ReleaseBuffer_Proxy( 
    IMemAllocator __RPC_FAR * This,
    /* [in] */ IMediaSample __RPC_FAR *pBuffer);


void __RPC_STUB IMemAllocator_ReleaseBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMemAllocator_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0050
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IMemAllocator __RPC_FAR *PMEMALLOCATOR;



extern RPC_IF_HANDLE __MIDL__intf_0050_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0050_v0_0_s_ifspec;

#ifndef __IMemInputPin_INTERFACE_DEFINED__
#define __IMemInputPin_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMemInputPin
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMemInputPin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMemInputPin : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetAllocator( 
            /* [out] */ IMemAllocator __RPC_FAR *__RPC_FAR *ppAllocator) = 0;
        
        virtual HRESULT __stdcall NotifyAllocator( 
            /* [in] */ IMemAllocator __RPC_FAR *pAllocator,
            /* [in] */ BOOL bReadOnly) = 0;
        
        virtual HRESULT __stdcall GetAllocatorRequirements( 
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps) = 0;
        
        virtual HRESULT __stdcall Receive( 
            /* [in] */ IMediaSample __RPC_FAR *pSample) = 0;
        
        virtual HRESULT __stdcall ReceiveMultiple( 
            /* [size_is][in] */ IMediaSample __RPC_FAR *__RPC_FAR *pSamples,
            /* [in] */ long nSamples,
            /* [out] */ long __RPC_FAR *nSamplesProcessed) = 0;
        
        virtual HRESULT __stdcall ReceiveCanBlock( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMemInputPinVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMemInputPin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMemInputPin __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMemInputPin __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetAllocator )( 
            IMemInputPin __RPC_FAR * This,
            /* [out] */ IMemAllocator __RPC_FAR *__RPC_FAR *ppAllocator);
        
        HRESULT ( __stdcall __RPC_FAR *NotifyAllocator )( 
            IMemInputPin __RPC_FAR * This,
            /* [in] */ IMemAllocator __RPC_FAR *pAllocator,
            /* [in] */ BOOL bReadOnly);
        
        HRESULT ( __stdcall __RPC_FAR *GetAllocatorRequirements )( 
            IMemInputPin __RPC_FAR * This,
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps);
        
        HRESULT ( __stdcall __RPC_FAR *Receive )( 
            IMemInputPin __RPC_FAR * This,
            /* [in] */ IMediaSample __RPC_FAR *pSample);
        
        HRESULT ( __stdcall __RPC_FAR *ReceiveMultiple )( 
            IMemInputPin __RPC_FAR * This,
            /* [size_is][in] */ IMediaSample __RPC_FAR *__RPC_FAR *pSamples,
            /* [in] */ long nSamples,
            /* [out] */ long __RPC_FAR *nSamplesProcessed);
        
        HRESULT ( __stdcall __RPC_FAR *ReceiveCanBlock )( 
            IMemInputPin __RPC_FAR * This);
        
    } IMemInputPinVtbl;

    interface IMemInputPin
    {
        CONST_VTBL struct IMemInputPinVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMemInputPin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMemInputPin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMemInputPin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMemInputPin_GetAllocator(This,ppAllocator)	\
    (This)->lpVtbl -> GetAllocator(This,ppAllocator)

#define IMemInputPin_NotifyAllocator(This,pAllocator,bReadOnly)	\
    (This)->lpVtbl -> NotifyAllocator(This,pAllocator,bReadOnly)

#define IMemInputPin_GetAllocatorRequirements(This,pProps)	\
    (This)->lpVtbl -> GetAllocatorRequirements(This,pProps)

#define IMemInputPin_Receive(This,pSample)	\
    (This)->lpVtbl -> Receive(This,pSample)

#define IMemInputPin_ReceiveMultiple(This,pSamples,nSamples,nSamplesProcessed)	\
    (This)->lpVtbl -> ReceiveMultiple(This,pSamples,nSamples,nSamplesProcessed)

#define IMemInputPin_ReceiveCanBlock(This)	\
    (This)->lpVtbl -> ReceiveCanBlock(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMemInputPin_GetAllocator_Proxy( 
    IMemInputPin __RPC_FAR * This,
    /* [out] */ IMemAllocator __RPC_FAR *__RPC_FAR *ppAllocator);


void __RPC_STUB IMemInputPin_GetAllocator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemInputPin_NotifyAllocator_Proxy( 
    IMemInputPin __RPC_FAR * This,
    /* [in] */ IMemAllocator __RPC_FAR *pAllocator,
    /* [in] */ BOOL bReadOnly);


void __RPC_STUB IMemInputPin_NotifyAllocator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemInputPin_GetAllocatorRequirements_Proxy( 
    IMemInputPin __RPC_FAR * This,
    /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps);


void __RPC_STUB IMemInputPin_GetAllocatorRequirements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemInputPin_Receive_Proxy( 
    IMemInputPin __RPC_FAR * This,
    /* [in] */ IMediaSample __RPC_FAR *pSample);


void __RPC_STUB IMemInputPin_Receive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemInputPin_ReceiveMultiple_Proxy( 
    IMemInputPin __RPC_FAR * This,
    /* [size_is][in] */ IMediaSample __RPC_FAR *__RPC_FAR *pSamples,
    /* [in] */ long nSamples,
    /* [out] */ long __RPC_FAR *nSamplesProcessed);


void __RPC_STUB IMemInputPin_ReceiveMultiple_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMemInputPin_ReceiveCanBlock_Proxy( 
    IMemInputPin __RPC_FAR * This);


void __RPC_STUB IMemInputPin_ReceiveCanBlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMemInputPin_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0051
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IMemInputPin __RPC_FAR *PMEMINPUTPIN;



extern RPC_IF_HANDLE __MIDL__intf_0051_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0051_v0_0_s_ifspec;

#ifndef __IAMovieSetup_INTERFACE_DEFINED__
#define __IAMovieSetup_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAMovieSetup
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IAMovieSetup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAMovieSetup : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Register( void) = 0;
        
        virtual HRESULT __stdcall Unregister( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAMovieSetupVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IAMovieSetup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IAMovieSetup __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IAMovieSetup __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Register )( 
            IAMovieSetup __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Unregister )( 
            IAMovieSetup __RPC_FAR * This);
        
    } IAMovieSetupVtbl;

    interface IAMovieSetup
    {
        CONST_VTBL struct IAMovieSetupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAMovieSetup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAMovieSetup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAMovieSetup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAMovieSetup_Register(This)	\
    (This)->lpVtbl -> Register(This)

#define IAMovieSetup_Unregister(This)	\
    (This)->lpVtbl -> Unregister(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IAMovieSetup_Register_Proxy( 
    IAMovieSetup __RPC_FAR * This);


void __RPC_STUB IAMovieSetup_Register_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovieSetup_Unregister_Proxy( 
    IAMovieSetup __RPC_FAR * This);


void __RPC_STUB IAMovieSetup_Unregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAMovieSetup_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0052
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IAMovieSetup __RPC_FAR *PAMOVIESETUP;



extern RPC_IF_HANDLE __MIDL__intf_0052_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0052_v0_0_s_ifspec;

#ifndef __IMediaSelection_INTERFACE_DEFINED__
#define __IMediaSelection_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMediaSelection
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMediaSelection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMediaSelection : public IUnknown
    {
    public:
        virtual HRESULT __stdcall IsFormatSupported( 
            /* [in] */ const GUID Format) = 0;
        
        virtual HRESULT __stdcall QueryPreferredFormat( 
            /* [out] */ GUID __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT __stdcall SetTimeFormat( 
            /* [in] */ const GUID Format) = 0;
        
        virtual HRESULT __stdcall GetTimeFormat( 
            /* [out] */ GUID __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT __stdcall GetDuration( 
            /* [out] */ LONGLONG __RPC_FAR *pDuration) = 0;
        
        virtual HRESULT __stdcall GetStopPosition( 
            /* [out] */ LONGLONG __RPC_FAR *pStop) = 0;
        
        virtual HRESULT __stdcall GetCurrentPosition( 
            /* [out] */ LONGLONG __RPC_FAR *pCurrent) = 0;
        
        virtual HRESULT __stdcall SetSelection( 
            /* [in] */ LONGLONG Current,
            /* [in] */ LONGLONG Stop,
            /* [out] */ REFTIME __RPC_FAR *pTime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMediaSelectionVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMediaSelection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMediaSelection __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMediaSelection __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *IsFormatSupported )( 
            IMediaSelection __RPC_FAR * This,
            /* [in] */ const GUID Format);
        
        HRESULT ( __stdcall __RPC_FAR *QueryPreferredFormat )( 
            IMediaSelection __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pFormat);
        
        HRESULT ( __stdcall __RPC_FAR *SetTimeFormat )( 
            IMediaSelection __RPC_FAR * This,
            /* [in] */ const GUID Format);
        
        HRESULT ( __stdcall __RPC_FAR *GetTimeFormat )( 
            IMediaSelection __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pFormat);
        
        HRESULT ( __stdcall __RPC_FAR *GetDuration )( 
            IMediaSelection __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pDuration);
        
        HRESULT ( __stdcall __RPC_FAR *GetStopPosition )( 
            IMediaSelection __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pStop);
        
        HRESULT ( __stdcall __RPC_FAR *GetCurrentPosition )( 
            IMediaSelection __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pCurrent);
        
        HRESULT ( __stdcall __RPC_FAR *SetSelection )( 
            IMediaSelection __RPC_FAR * This,
            /* [in] */ LONGLONG Current,
            /* [in] */ LONGLONG Stop,
            /* [out] */ REFTIME __RPC_FAR *pTime);
        
    } IMediaSelectionVtbl;

    interface IMediaSelection
    {
        CONST_VTBL struct IMediaSelectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMediaSelection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMediaSelection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMediaSelection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMediaSelection_IsFormatSupported(This,Format)	\
    (This)->lpVtbl -> IsFormatSupported(This,Format)

#define IMediaSelection_QueryPreferredFormat(This,pFormat)	\
    (This)->lpVtbl -> QueryPreferredFormat(This,pFormat)

#define IMediaSelection_SetTimeFormat(This,Format)	\
    (This)->lpVtbl -> SetTimeFormat(This,Format)

#define IMediaSelection_GetTimeFormat(This,pFormat)	\
    (This)->lpVtbl -> GetTimeFormat(This,pFormat)

#define IMediaSelection_GetDuration(This,pDuration)	\
    (This)->lpVtbl -> GetDuration(This,pDuration)

#define IMediaSelection_GetStopPosition(This,pStop)	\
    (This)->lpVtbl -> GetStopPosition(This,pStop)

#define IMediaSelection_GetCurrentPosition(This,pCurrent)	\
    (This)->lpVtbl -> GetCurrentPosition(This,pCurrent)

#define IMediaSelection_SetSelection(This,Current,Stop,pTime)	\
    (This)->lpVtbl -> SetSelection(This,Current,Stop,pTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMediaSelection_IsFormatSupported_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [in] */ const GUID Format);


void __RPC_STUB IMediaSelection_IsFormatSupported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSelection_QueryPreferredFormat_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pFormat);


void __RPC_STUB IMediaSelection_QueryPreferredFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSelection_SetTimeFormat_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [in] */ const GUID Format);


void __RPC_STUB IMediaSelection_SetTimeFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSelection_GetTimeFormat_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pFormat);


void __RPC_STUB IMediaSelection_GetTimeFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSelection_GetDuration_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pDuration);


void __RPC_STUB IMediaSelection_GetDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSelection_GetStopPosition_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pStop);


void __RPC_STUB IMediaSelection_GetStopPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSelection_GetCurrentPosition_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pCurrent);


void __RPC_STUB IMediaSelection_GetCurrentPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSelection_SetSelection_Proxy( 
    IMediaSelection __RPC_FAR * This,
    /* [in] */ LONGLONG Current,
    /* [in] */ LONGLONG Stop,
    /* [out] */ REFTIME __RPC_FAR *pTime);


void __RPC_STUB IMediaSelection_SetSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMediaSelection_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0053
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IMediaSelection __RPC_FAR *PMEDIASELECTION;

			/* size is 2 */
typedef 
enum _SeekingFlags
    {	AM_SEEKING_NoPositioning	= 0,
	AM_SEEKING_AbsolutePositioning	= 0x1,
	AM_SEEKING_RelativePositioning	= 0x2,
	AM_SEEKING_IncrementalPositioning	= 0x3,
	AM_SEEKING_PositioningBitsMask	= 0x3,
	AM_SEEKING_SeekToKeyFrame	= 0x4,
	AM_SEEKING_ReturnTime	= 0x8
    }	SEEKING_FLAGS;

			/* size is 2 */
typedef 
enum _SeekingCapabilities
    {	AM_SEEKING_CanSeekAbsolute	= 0x1,
	AM_SEEKING_CanSeekForwards	= 0x2,
	AM_SEEKING_CanSeekBackwards	= 0x4,
	AM_SEEKING_CanGetCurrentPos	= 0x8,
	AM_SEEKING_CanGetStopPos	= 0x10,
	AM_SEEKING_CanGetDuration	= 0x20,
	AM_SEEKING_CanPlayBackwards	= 0x40
    }	SEEKING_CAPABILITIES;



extern RPC_IF_HANDLE __MIDL__intf_0053_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0053_v0_0_s_ifspec;

#ifndef __IMediaSeeking_INTERFACE_DEFINED__
#define __IMediaSeeking_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMediaSeeking
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMediaSeeking;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMediaSeeking : public IMediaSelection
    {
    public:
        virtual HRESULT __stdcall GetCapabilities( 
            /* [out] */ DWORD __RPC_FAR *pCapabilities) = 0;
        
        virtual HRESULT __stdcall CheckCapabilities( 
            /* [out][in] */ DWORD __RPC_FAR *pCapabilities) = 0;
        
        virtual HRESULT __stdcall ConvertTimeFormat( 
            /* [out] */ LONGLONG __RPC_FAR *pTarget,
            /* [in] */ const GUID __RPC_FAR *pTargetFormat,
            /* [in] */ LONGLONG Source,
            /* [in] */ const GUID __RPC_FAR *pSourceFormat) = 0;
        
        virtual HRESULT __stdcall SetPositions( 
            /* [out][in] */ LONGLONG __RPC_FAR *pCurrent,
            /* [in] */ DWORD dwCurrentFlags,
            /* [out][in] */ LONGLONG __RPC_FAR *pStop,
            /* [in] */ DWORD dwStopFlags) = 0;
        
        virtual HRESULT __stdcall GetPositions( 
            /* [out] */ LONGLONG __RPC_FAR *pCurrent,
            /* [out] */ LONGLONG __RPC_FAR *pStop) = 0;
        
        virtual HRESULT __stdcall GetAvailable( 
            /* [out] */ LONGLONG __RPC_FAR *pEarliest,
            /* [out] */ LONGLONG __RPC_FAR *pLatest) = 0;
        
        virtual HRESULT __stdcall SetRate( 
            /* [in] */ double dRate) = 0;
        
        virtual HRESULT __stdcall GetRate( 
            /* [out] */ double __RPC_FAR *pdRate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMediaSeekingVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMediaSeeking __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMediaSeeking __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMediaSeeking __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *IsFormatSupported )( 
            IMediaSeeking __RPC_FAR * This,
            /* [in] */ const GUID Format);
        
        HRESULT ( __stdcall __RPC_FAR *QueryPreferredFormat )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pFormat);
        
        HRESULT ( __stdcall __RPC_FAR *SetTimeFormat )( 
            IMediaSeeking __RPC_FAR * This,
            /* [in] */ const GUID Format);
        
        HRESULT ( __stdcall __RPC_FAR *GetTimeFormat )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pFormat);
        
        HRESULT ( __stdcall __RPC_FAR *GetDuration )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pDuration);
        
        HRESULT ( __stdcall __RPC_FAR *GetStopPosition )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pStop);
        
        HRESULT ( __stdcall __RPC_FAR *GetCurrentPosition )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pCurrent);
        
        HRESULT ( __stdcall __RPC_FAR *SetSelection )( 
            IMediaSeeking __RPC_FAR * This,
            /* [in] */ LONGLONG Current,
            /* [in] */ LONGLONG Stop,
            /* [out] */ REFTIME __RPC_FAR *pTime);
        
        HRESULT ( __stdcall __RPC_FAR *GetCapabilities )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pCapabilities);
        
        HRESULT ( __stdcall __RPC_FAR *CheckCapabilities )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out][in] */ DWORD __RPC_FAR *pCapabilities);
        
        HRESULT ( __stdcall __RPC_FAR *ConvertTimeFormat )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pTarget,
            /* [in] */ const GUID __RPC_FAR *pTargetFormat,
            /* [in] */ LONGLONG Source,
            /* [in] */ const GUID __RPC_FAR *pSourceFormat);
        
        HRESULT ( __stdcall __RPC_FAR *SetPositions )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out][in] */ LONGLONG __RPC_FAR *pCurrent,
            /* [in] */ DWORD dwCurrentFlags,
            /* [out][in] */ LONGLONG __RPC_FAR *pStop,
            /* [in] */ DWORD dwStopFlags);
        
        HRESULT ( __stdcall __RPC_FAR *GetPositions )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pCurrent,
            /* [out] */ LONGLONG __RPC_FAR *pStop);
        
        HRESULT ( __stdcall __RPC_FAR *GetAvailable )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pEarliest,
            /* [out] */ LONGLONG __RPC_FAR *pLatest);
        
        HRESULT ( __stdcall __RPC_FAR *SetRate )( 
            IMediaSeeking __RPC_FAR * This,
            /* [in] */ double dRate);
        
        HRESULT ( __stdcall __RPC_FAR *GetRate )( 
            IMediaSeeking __RPC_FAR * This,
            /* [out] */ double __RPC_FAR *pdRate);
        
    } IMediaSeekingVtbl;

    interface IMediaSeeking
    {
        CONST_VTBL struct IMediaSeekingVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMediaSeeking_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMediaSeeking_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMediaSeeking_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMediaSeeking_IsFormatSupported(This,Format)	\
    (This)->lpVtbl -> IsFormatSupported(This,Format)

#define IMediaSeeking_QueryPreferredFormat(This,pFormat)	\
    (This)->lpVtbl -> QueryPreferredFormat(This,pFormat)

#define IMediaSeeking_SetTimeFormat(This,Format)	\
    (This)->lpVtbl -> SetTimeFormat(This,Format)

#define IMediaSeeking_GetTimeFormat(This,pFormat)	\
    (This)->lpVtbl -> GetTimeFormat(This,pFormat)

#define IMediaSeeking_GetDuration(This,pDuration)	\
    (This)->lpVtbl -> GetDuration(This,pDuration)

#define IMediaSeeking_GetStopPosition(This,pStop)	\
    (This)->lpVtbl -> GetStopPosition(This,pStop)

#define IMediaSeeking_GetCurrentPosition(This,pCurrent)	\
    (This)->lpVtbl -> GetCurrentPosition(This,pCurrent)

#define IMediaSeeking_SetSelection(This,Current,Stop,pTime)	\
    (This)->lpVtbl -> SetSelection(This,Current,Stop,pTime)


#define IMediaSeeking_GetCapabilities(This,pCapabilities)	\
    (This)->lpVtbl -> GetCapabilities(This,pCapabilities)

#define IMediaSeeking_CheckCapabilities(This,pCapabilities)	\
    (This)->lpVtbl -> CheckCapabilities(This,pCapabilities)

#define IMediaSeeking_ConvertTimeFormat(This,pTarget,pTargetFormat,Source,pSourceFormat)	\
    (This)->lpVtbl -> ConvertTimeFormat(This,pTarget,pTargetFormat,Source,pSourceFormat)

#define IMediaSeeking_SetPositions(This,pCurrent,dwCurrentFlags,pStop,dwStopFlags)	\
    (This)->lpVtbl -> SetPositions(This,pCurrent,dwCurrentFlags,pStop,dwStopFlags)

#define IMediaSeeking_GetPositions(This,pCurrent,pStop)	\
    (This)->lpVtbl -> GetPositions(This,pCurrent,pStop)

#define IMediaSeeking_GetAvailable(This,pEarliest,pLatest)	\
    (This)->lpVtbl -> GetAvailable(This,pEarliest,pLatest)

#define IMediaSeeking_SetRate(This,dRate)	\
    (This)->lpVtbl -> SetRate(This,dRate)

#define IMediaSeeking_GetRate(This,pdRate)	\
    (This)->lpVtbl -> GetRate(This,pdRate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMediaSeeking_GetCapabilities_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pCapabilities);


void __RPC_STUB IMediaSeeking_GetCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSeeking_CheckCapabilities_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [out][in] */ DWORD __RPC_FAR *pCapabilities);


void __RPC_STUB IMediaSeeking_CheckCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSeeking_ConvertTimeFormat_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pTarget,
    /* [in] */ const GUID __RPC_FAR *pTargetFormat,
    /* [in] */ LONGLONG Source,
    /* [in] */ const GUID __RPC_FAR *pSourceFormat);


void __RPC_STUB IMediaSeeking_ConvertTimeFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSeeking_SetPositions_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [out][in] */ LONGLONG __RPC_FAR *pCurrent,
    /* [in] */ DWORD dwCurrentFlags,
    /* [out][in] */ LONGLONG __RPC_FAR *pStop,
    /* [in] */ DWORD dwStopFlags);


void __RPC_STUB IMediaSeeking_SetPositions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSeeking_GetPositions_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pCurrent,
    /* [out] */ LONGLONG __RPC_FAR *pStop);


void __RPC_STUB IMediaSeeking_GetPositions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSeeking_GetAvailable_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pEarliest,
    /* [out] */ LONGLONG __RPC_FAR *pLatest);


void __RPC_STUB IMediaSeeking_GetAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSeeking_SetRate_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [in] */ double dRate);


void __RPC_STUB IMediaSeeking_SetRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMediaSeeking_GetRate_Proxy( 
    IMediaSeeking __RPC_FAR * This,
    /* [out] */ double __RPC_FAR *pdRate);


void __RPC_STUB IMediaSeeking_GetRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMediaSeeking_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0054
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IMediaSeeking __RPC_FAR *PMEDIASEEKING;

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 20 */
typedef struct  __MIDL___MIDL__intf_0054_0001
    {
    CLSID Clsid;
    LPWSTR Name;
    }	REGFILTER;



extern RPC_IF_HANDLE __MIDL__intf_0054_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0054_v0_0_s_ifspec;

#ifndef __IEnumRegFilters_INTERFACE_DEFINED__
#define __IEnumRegFilters_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRegFilters
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumRegFilters;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRegFilters : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Next( 
            /* [in] */ ULONG cFilters,
            /* [out] */ REGFILTER __RPC_FAR *__RPC_FAR *apRegFilter,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT __stdcall Skip( 
            /* [in] */ ULONG cFilters) = 0;
        
        virtual HRESULT __stdcall Reset( void) = 0;
        
        virtual HRESULT __stdcall Clone( 
            /* [out] */ IEnumRegFilters __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRegFiltersVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IEnumRegFilters __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IEnumRegFilters __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IEnumRegFilters __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Next )( 
            IEnumRegFilters __RPC_FAR * This,
            /* [in] */ ULONG cFilters,
            /* [out] */ REGFILTER __RPC_FAR *__RPC_FAR *apRegFilter,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( __stdcall __RPC_FAR *Skip )( 
            IEnumRegFilters __RPC_FAR * This,
            /* [in] */ ULONG cFilters);
        
        HRESULT ( __stdcall __RPC_FAR *Reset )( 
            IEnumRegFilters __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Clone )( 
            IEnumRegFilters __RPC_FAR * This,
            /* [out] */ IEnumRegFilters __RPC_FAR *__RPC_FAR *ppEnum);
        
    } IEnumRegFiltersVtbl;

    interface IEnumRegFilters
    {
        CONST_VTBL struct IEnumRegFiltersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRegFilters_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRegFilters_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRegFilters_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRegFilters_Next(This,cFilters,apRegFilter,pcFetched)	\
    (This)->lpVtbl -> Next(This,cFilters,apRegFilter,pcFetched)

#define IEnumRegFilters_Skip(This,cFilters)	\
    (This)->lpVtbl -> Skip(This,cFilters)

#define IEnumRegFilters_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRegFilters_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IEnumRegFilters_Next_Proxy( 
    IEnumRegFilters __RPC_FAR * This,
    /* [in] */ ULONG cFilters,
    /* [out] */ REGFILTER __RPC_FAR *__RPC_FAR *apRegFilter,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumRegFilters_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumRegFilters_Skip_Proxy( 
    IEnumRegFilters __RPC_FAR * This,
    /* [in] */ ULONG cFilters);


void __RPC_STUB IEnumRegFilters_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumRegFilters_Reset_Proxy( 
    IEnumRegFilters __RPC_FAR * This);


void __RPC_STUB IEnumRegFilters_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumRegFilters_Clone_Proxy( 
    IEnumRegFilters __RPC_FAR * This,
    /* [out] */ IEnumRegFilters __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRegFilters_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRegFilters_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0056
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IEnumRegFilters __RPC_FAR *PENUMREGFILTERS;



extern RPC_IF_HANDLE __MIDL__intf_0056_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0056_v0_0_s_ifspec;

#ifndef __IFilterMapper_INTERFACE_DEFINED__
#define __IFilterMapper_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFilterMapper
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 2 */

enum __MIDL_IFilterMapper_0001
    {	MERIT_PREFERRED	= 0x800000,
	MERIT_NORMAL	= 0x600000,
	MERIT_UNLIKELY	= 0x400000,
	MERIT_DO_NOT_USE	= 0x200000
    };

EXTERN_C const IID IID_IFilterMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IFilterMapper : public IUnknown
    {
    public:
        virtual HRESULT __stdcall RegisterFilter( 
            /* [in] */ CLSID clsid,
            /* [in] */ LPCWSTR Name,
            /* [in] */ DWORD dwMerit) = 0;
        
        virtual HRESULT __stdcall RegisterFilterInstance( 
            /* [in] */ CLSID clsid,
            /* [in] */ LPCWSTR Name,
            /* [out] */ CLSID __RPC_FAR *MRId) = 0;
        
        virtual HRESULT __stdcall RegisterPin( 
            /* [in] */ CLSID Filter,
            /* [in] */ LPCWSTR Name,
            /* [in] */ BOOL bRendered,
            /* [in] */ BOOL bOutput,
            /* [in] */ BOOL bZero,
            /* [in] */ BOOL bMany,
            /* [in] */ CLSID ConnectsToFilter,
            /* [in] */ LPCWSTR ConnectsToPin) = 0;
        
        virtual HRESULT __stdcall RegisterPinType( 
            /* [in] */ CLSID clsFilter,
            /* [in] */ LPCWSTR strName,
            /* [in] */ CLSID clsMajorType,
            /* [in] */ CLSID clsSubType) = 0;
        
        virtual HRESULT __stdcall UnregisterFilter( 
            /* [in] */ CLSID Filter) = 0;
        
        virtual HRESULT __stdcall UnregisterFilterInstance( 
            /* [in] */ CLSID MRId) = 0;
        
        virtual HRESULT __stdcall UnregisterPin( 
            /* [in] */ CLSID Filter,
            /* [in] */ LPCWSTR Name) = 0;
        
        virtual HRESULT __stdcall EnumMatchingFilters( 
            /* [out] */ IEnumRegFilters __RPC_FAR *__RPC_FAR *ppEnum,
            /* [in] */ DWORD dwMerit,
            /* [in] */ BOOL bInputNeeded,
            /* [in] */ CLSID clsInMaj,
            /* [in] */ CLSID clsInSub,
            /* [in] */ BOOL bRender,
            /* [in] */ BOOL bOututNeeded,
            /* [in] */ CLSID clsOutMaj,
            /* [in] */ CLSID clsOutSub) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFilterMapperVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IFilterMapper __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IFilterMapper __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterFilter )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ CLSID clsid,
            /* [in] */ LPCWSTR Name,
            /* [in] */ DWORD dwMerit);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterFilterInstance )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ CLSID clsid,
            /* [in] */ LPCWSTR Name,
            /* [out] */ CLSID __RPC_FAR *MRId);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterPin )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ CLSID Filter,
            /* [in] */ LPCWSTR Name,
            /* [in] */ BOOL bRendered,
            /* [in] */ BOOL bOutput,
            /* [in] */ BOOL bZero,
            /* [in] */ BOOL bMany,
            /* [in] */ CLSID ConnectsToFilter,
            /* [in] */ LPCWSTR ConnectsToPin);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterPinType )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ CLSID clsFilter,
            /* [in] */ LPCWSTR strName,
            /* [in] */ CLSID clsMajorType,
            /* [in] */ CLSID clsSubType);
        
        HRESULT ( __stdcall __RPC_FAR *UnregisterFilter )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ CLSID Filter);
        
        HRESULT ( __stdcall __RPC_FAR *UnregisterFilterInstance )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ CLSID MRId);
        
        HRESULT ( __stdcall __RPC_FAR *UnregisterPin )( 
            IFilterMapper __RPC_FAR * This,
            /* [in] */ CLSID Filter,
            /* [in] */ LPCWSTR Name);
        
        HRESULT ( __stdcall __RPC_FAR *EnumMatchingFilters )( 
            IFilterMapper __RPC_FAR * This,
            /* [out] */ IEnumRegFilters __RPC_FAR *__RPC_FAR *ppEnum,
            /* [in] */ DWORD dwMerit,
            /* [in] */ BOOL bInputNeeded,
            /* [in] */ CLSID clsInMaj,
            /* [in] */ CLSID clsInSub,
            /* [in] */ BOOL bRender,
            /* [in] */ BOOL bOututNeeded,
            /* [in] */ CLSID clsOutMaj,
            /* [in] */ CLSID clsOutSub);
        
    } IFilterMapperVtbl;

    interface IFilterMapper
    {
        CONST_VTBL struct IFilterMapperVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFilterMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFilterMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFilterMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFilterMapper_RegisterFilter(This,clsid,Name,dwMerit)	\
    (This)->lpVtbl -> RegisterFilter(This,clsid,Name,dwMerit)

#define IFilterMapper_RegisterFilterInstance(This,clsid,Name,MRId)	\
    (This)->lpVtbl -> RegisterFilterInstance(This,clsid,Name,MRId)

#define IFilterMapper_RegisterPin(This,Filter,Name,bRendered,bOutput,bZero,bMany,ConnectsToFilter,ConnectsToPin)	\
    (This)->lpVtbl -> RegisterPin(This,Filter,Name,bRendered,bOutput,bZero,bMany,ConnectsToFilter,ConnectsToPin)

#define IFilterMapper_RegisterPinType(This,clsFilter,strName,clsMajorType,clsSubType)	\
    (This)->lpVtbl -> RegisterPinType(This,clsFilter,strName,clsMajorType,clsSubType)

#define IFilterMapper_UnregisterFilter(This,Filter)	\
    (This)->lpVtbl -> UnregisterFilter(This,Filter)

#define IFilterMapper_UnregisterFilterInstance(This,MRId)	\
    (This)->lpVtbl -> UnregisterFilterInstance(This,MRId)

#define IFilterMapper_UnregisterPin(This,Filter,Name)	\
    (This)->lpVtbl -> UnregisterPin(This,Filter,Name)

#define IFilterMapper_EnumMatchingFilters(This,ppEnum,dwMerit,bInputNeeded,clsInMaj,clsInSub,bRender,bOututNeeded,clsOutMaj,clsOutSub)	\
    (This)->lpVtbl -> EnumMatchingFilters(This,ppEnum,dwMerit,bInputNeeded,clsInMaj,clsInSub,bRender,bOututNeeded,clsOutMaj,clsOutSub)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IFilterMapper_RegisterFilter_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [in] */ CLSID clsid,
    /* [in] */ LPCWSTR Name,
    /* [in] */ DWORD dwMerit);


void __RPC_STUB IFilterMapper_RegisterFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterMapper_RegisterFilterInstance_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [in] */ CLSID clsid,
    /* [in] */ LPCWSTR Name,
    /* [out] */ CLSID __RPC_FAR *MRId);


void __RPC_STUB IFilterMapper_RegisterFilterInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterMapper_RegisterPin_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [in] */ CLSID Filter,
    /* [in] */ LPCWSTR Name,
    /* [in] */ BOOL bRendered,
    /* [in] */ BOOL bOutput,
    /* [in] */ BOOL bZero,
    /* [in] */ BOOL bMany,
    /* [in] */ CLSID ConnectsToFilter,
    /* [in] */ LPCWSTR ConnectsToPin);


void __RPC_STUB IFilterMapper_RegisterPin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterMapper_RegisterPinType_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [in] */ CLSID clsFilter,
    /* [in] */ LPCWSTR strName,
    /* [in] */ CLSID clsMajorType,
    /* [in] */ CLSID clsSubType);


void __RPC_STUB IFilterMapper_RegisterPinType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterMapper_UnregisterFilter_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [in] */ CLSID Filter);


void __RPC_STUB IFilterMapper_UnregisterFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterMapper_UnregisterFilterInstance_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [in] */ CLSID MRId);


void __RPC_STUB IFilterMapper_UnregisterFilterInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterMapper_UnregisterPin_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [in] */ CLSID Filter,
    /* [in] */ LPCWSTR Name);


void __RPC_STUB IFilterMapper_UnregisterPin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFilterMapper_EnumMatchingFilters_Proxy( 
    IFilterMapper __RPC_FAR * This,
    /* [out] */ IEnumRegFilters __RPC_FAR *__RPC_FAR *ppEnum,
    /* [in] */ DWORD dwMerit,
    /* [in] */ BOOL bInputNeeded,
    /* [in] */ CLSID clsInMaj,
    /* [in] */ CLSID clsInSub,
    /* [in] */ BOOL bRender,
    /* [in] */ BOOL bOututNeeded,
    /* [in] */ CLSID clsOutMaj,
    /* [in] */ CLSID clsOutSub);


void __RPC_STUB IFilterMapper_EnumMatchingFilters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFilterMapper_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0058
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 2 */
typedef 
enum __MIDL___MIDL__intf_0058_0001
    {	Famine	= 0,
	Flood	= Famine + 1
    }	QualityMessageType;

			/* size is 24 */
typedef struct  __MIDL___MIDL__intf_0058_0002
    {
    QualityMessageType Type;
    long Proportion;
    REFERENCE_TIME Late;
    REFERENCE_TIME TimeStamp;
    }	Quality;

			/* size is 4 */
typedef IQualityControl __RPC_FAR *PQUALITYCONTROL;



extern RPC_IF_HANDLE __MIDL__intf_0058_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0058_v0_0_s_ifspec;

#ifndef __IQualityControl_INTERFACE_DEFINED__
#define __IQualityControl_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IQualityControl
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IQualityControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IQualityControl : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Notify( 
            /* [in] */ IFilter __RPC_FAR *pSelf,
            /* [in] */ Quality q) = 0;
        
        virtual HRESULT __stdcall SetSink( 
            /* [in] */ IQualityControl __RPC_FAR *piqc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IQualityControlVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IQualityControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IQualityControl __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IQualityControl __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Notify )( 
            IQualityControl __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pSelf,
            /* [in] */ Quality q);
        
        HRESULT ( __stdcall __RPC_FAR *SetSink )( 
            IQualityControl __RPC_FAR * This,
            /* [in] */ IQualityControl __RPC_FAR *piqc);
        
    } IQualityControlVtbl;

    interface IQualityControl
    {
        CONST_VTBL struct IQualityControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IQualityControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IQualityControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IQualityControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IQualityControl_Notify(This,pSelf,q)	\
    (This)->lpVtbl -> Notify(This,pSelf,q)

#define IQualityControl_SetSink(This,piqc)	\
    (This)->lpVtbl -> SetSink(This,piqc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IQualityControl_Notify_Proxy( 
    IQualityControl __RPC_FAR * This,
    /* [in] */ IFilter __RPC_FAR *pSelf,
    /* [in] */ Quality q);


void __RPC_STUB IQualityControl_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IQualityControl_SetSink_Proxy( 
    IQualityControl __RPC_FAR * This,
    /* [in] */ IQualityControl __RPC_FAR *piqc);


void __RPC_STUB IQualityControl_SetSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IQualityControl_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0059
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 2 */

enum __MIDL___MIDL__intf_0059_0001
    {	CK_NOCOLORKEY	= 0,
	CK_INDEX	= 0x1,
	CK_RGB	= 0x2
    };
			/* size is 16 */
typedef struct  tagCOLORKEY
    {
    DWORD KeyType;
    DWORD PaletteIndex;
    COLORREF LowColorValue;
    COLORREF HighColorValue;
    }	COLORKEY;

			/* size is 2 */

enum __MIDL___MIDL__intf_0059_0002
    {	ADVISE_NONE	= 0,
	ADVISE_CLIPPING	= 0x1,
	ADVISE_PALETTE	= 0x2,
	ADVISE_COLORKEY	= 0x4,
	ADVISE_POSITION	= 0x8
    };
			/* size is 4 */
#define	ADVISE_ALL	( ADVISE_CLIPPING | ADVISE_PALETTE | ADVISE_COLORKEY | ADVISE_POSITION )

#ifndef _WINGDI_
			/* size is 32 */
typedef struct  _RGNDATAHEADER
    {
    DWORD dwSize;
    DWORD iType;
    DWORD nCount;
    DWORD nRgnSize;
    RECT rcBound;
    }	RGNDATAHEADER;

			/* size is 33 */
typedef struct  _RGNDATA
    {
    RGNDATAHEADER rdh;
    char Buffer[ 1 ];
    }	RGNDATA;

#endif


extern RPC_IF_HANDLE __MIDL__intf_0059_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0059_v0_0_s_ifspec;

#ifndef __IOverlayNotify_INTERFACE_DEFINED__
#define __IOverlayNotify_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOverlayNotify
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][local][object] */ 



EXTERN_C const IID IID_IOverlayNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOverlayNotify : public IUnknown
    {
    public:
        virtual HRESULT __stdcall OnPaletteChange( 
            /* [in] */ DWORD dwColors,
            /* [in] */ const PALETTEENTRY __RPC_FAR *pPalette) = 0;
        
        virtual HRESULT __stdcall OnClipChange( 
            /* [in] */ const RECT __RPC_FAR *pSourceRect,
            /* [in] */ const RECT __RPC_FAR *pDestinationRect,
            /* [in] */ const RGNDATA __RPC_FAR *pRgnData) = 0;
        
        virtual HRESULT __stdcall OnColorKeyChange( 
            /* [in] */ const COLORKEY __RPC_FAR *pColorKey) = 0;
        
        virtual HRESULT __stdcall OnPositionChange( 
            /* [in] */ const RECT __RPC_FAR *pSourceRect,
            /* [in] */ const RECT __RPC_FAR *pDestinationRect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOverlayNotifyVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IOverlayNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IOverlayNotify __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IOverlayNotify __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *OnPaletteChange )( 
            IOverlayNotify __RPC_FAR * This,
            /* [in] */ DWORD dwColors,
            /* [in] */ const PALETTEENTRY __RPC_FAR *pPalette);
        
        HRESULT ( __stdcall __RPC_FAR *OnClipChange )( 
            IOverlayNotify __RPC_FAR * This,
            /* [in] */ const RECT __RPC_FAR *pSourceRect,
            /* [in] */ const RECT __RPC_FAR *pDestinationRect,
            /* [in] */ const RGNDATA __RPC_FAR *pRgnData);
        
        HRESULT ( __stdcall __RPC_FAR *OnColorKeyChange )( 
            IOverlayNotify __RPC_FAR * This,
            /* [in] */ const COLORKEY __RPC_FAR *pColorKey);
        
        HRESULT ( __stdcall __RPC_FAR *OnPositionChange )( 
            IOverlayNotify __RPC_FAR * This,
            /* [in] */ const RECT __RPC_FAR *pSourceRect,
            /* [in] */ const RECT __RPC_FAR *pDestinationRect);
        
    } IOverlayNotifyVtbl;

    interface IOverlayNotify
    {
        CONST_VTBL struct IOverlayNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOverlayNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOverlayNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOverlayNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOverlayNotify_OnPaletteChange(This,dwColors,pPalette)	\
    (This)->lpVtbl -> OnPaletteChange(This,dwColors,pPalette)

#define IOverlayNotify_OnClipChange(This,pSourceRect,pDestinationRect,pRgnData)	\
    (This)->lpVtbl -> OnClipChange(This,pSourceRect,pDestinationRect,pRgnData)

#define IOverlayNotify_OnColorKeyChange(This,pColorKey)	\
    (This)->lpVtbl -> OnColorKeyChange(This,pColorKey)

#define IOverlayNotify_OnPositionChange(This,pSourceRect,pDestinationRect)	\
    (This)->lpVtbl -> OnPositionChange(This,pSourceRect,pDestinationRect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IOverlayNotify_OnPaletteChange_Proxy( 
    IOverlayNotify __RPC_FAR * This,
    /* [in] */ DWORD dwColors,
    /* [in] */ const PALETTEENTRY __RPC_FAR *pPalette);


void __RPC_STUB IOverlayNotify_OnPaletteChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlayNotify_OnClipChange_Proxy( 
    IOverlayNotify __RPC_FAR * This,
    /* [in] */ const RECT __RPC_FAR *pSourceRect,
    /* [in] */ const RECT __RPC_FAR *pDestinationRect,
    /* [in] */ const RGNDATA __RPC_FAR *pRgnData);


void __RPC_STUB IOverlayNotify_OnClipChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlayNotify_OnColorKeyChange_Proxy( 
    IOverlayNotify __RPC_FAR * This,
    /* [in] */ const COLORKEY __RPC_FAR *pColorKey);


void __RPC_STUB IOverlayNotify_OnColorKeyChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlayNotify_OnPositionChange_Proxy( 
    IOverlayNotify __RPC_FAR * This,
    /* [in] */ const RECT __RPC_FAR *pSourceRect,
    /* [in] */ const RECT __RPC_FAR *pDestinationRect);


void __RPC_STUB IOverlayNotify_OnPositionChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOverlayNotify_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0060
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IOverlayNotify __RPC_FAR *POVERLAYNOTIFY;



extern RPC_IF_HANDLE __MIDL__intf_0060_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0060_v0_0_s_ifspec;

#ifndef __IOverlay_INTERFACE_DEFINED__
#define __IOverlay_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOverlay
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][local][object] */ 



EXTERN_C const IID IID_IOverlay;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOverlay : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetPalette( 
            /* [out] */ DWORD __RPC_FAR *pdwColors,
            /* [out] */ PALETTEENTRY __RPC_FAR *__RPC_FAR *ppPalette) = 0;
        
        virtual HRESULT __stdcall SetPalette( 
            /* [in] */ DWORD dwColors,
            /* [in] */ PALETTEENTRY __RPC_FAR *pPalette) = 0;
        
        virtual HRESULT __stdcall GetDefaultColorKey( 
            /* [out] */ COLORKEY __RPC_FAR *pColorKey) = 0;
        
        virtual HRESULT __stdcall GetColorKey( 
            /* [out] */ COLORKEY __RPC_FAR *pColorKey) = 0;
        
        virtual HRESULT __stdcall SetColorKey( 
            /* [out][in] */ COLORKEY __RPC_FAR *pColorKey) = 0;
        
        virtual HRESULT __stdcall GetWindowHandle( 
            /* [out] */ HWND __RPC_FAR *pHwnd) = 0;
        
        virtual HRESULT __stdcall GetClipList( 
            /* [out] */ RECT __RPC_FAR *pSourceRect,
            /* [out] */ RECT __RPC_FAR *pDestinationRect,
            /* [out] */ RGNDATA __RPC_FAR *__RPC_FAR *ppRgnData) = 0;
        
        virtual HRESULT __stdcall GetVideoPosition( 
            /* [out] */ RECT __RPC_FAR *pSourceRect,
            /* [out] */ RECT __RPC_FAR *pDestinationRect) = 0;
        
        virtual HRESULT __stdcall Advise( 
            /* [in] */ IOverlayNotify __RPC_FAR *pOverlayNotify,
            /* [in] */ DWORD dwInterests) = 0;
        
        virtual HRESULT __stdcall Unadvise( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOverlayVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IOverlay __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IOverlay __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IOverlay __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetPalette )( 
            IOverlay __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwColors,
            /* [out] */ PALETTEENTRY __RPC_FAR *__RPC_FAR *ppPalette);
        
        HRESULT ( __stdcall __RPC_FAR *SetPalette )( 
            IOverlay __RPC_FAR * This,
            /* [in] */ DWORD dwColors,
            /* [in] */ PALETTEENTRY __RPC_FAR *pPalette);
        
        HRESULT ( __stdcall __RPC_FAR *GetDefaultColorKey )( 
            IOverlay __RPC_FAR * This,
            /* [out] */ COLORKEY __RPC_FAR *pColorKey);
        
        HRESULT ( __stdcall __RPC_FAR *GetColorKey )( 
            IOverlay __RPC_FAR * This,
            /* [out] */ COLORKEY __RPC_FAR *pColorKey);
        
        HRESULT ( __stdcall __RPC_FAR *SetColorKey )( 
            IOverlay __RPC_FAR * This,
            /* [out][in] */ COLORKEY __RPC_FAR *pColorKey);
        
        HRESULT ( __stdcall __RPC_FAR *GetWindowHandle )( 
            IOverlay __RPC_FAR * This,
            /* [out] */ HWND __RPC_FAR *pHwnd);
        
        HRESULT ( __stdcall __RPC_FAR *GetClipList )( 
            IOverlay __RPC_FAR * This,
            /* [out] */ RECT __RPC_FAR *pSourceRect,
            /* [out] */ RECT __RPC_FAR *pDestinationRect,
            /* [out] */ RGNDATA __RPC_FAR *__RPC_FAR *ppRgnData);
        
        HRESULT ( __stdcall __RPC_FAR *GetVideoPosition )( 
            IOverlay __RPC_FAR * This,
            /* [out] */ RECT __RPC_FAR *pSourceRect,
            /* [out] */ RECT __RPC_FAR *pDestinationRect);
        
        HRESULT ( __stdcall __RPC_FAR *Advise )( 
            IOverlay __RPC_FAR * This,
            /* [in] */ IOverlayNotify __RPC_FAR *pOverlayNotify,
            /* [in] */ DWORD dwInterests);
        
        HRESULT ( __stdcall __RPC_FAR *Unadvise )( 
            IOverlay __RPC_FAR * This);
        
    } IOverlayVtbl;

    interface IOverlay
    {
        CONST_VTBL struct IOverlayVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOverlay_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOverlay_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOverlay_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOverlay_GetPalette(This,pdwColors,ppPalette)	\
    (This)->lpVtbl -> GetPalette(This,pdwColors,ppPalette)

#define IOverlay_SetPalette(This,dwColors,pPalette)	\
    (This)->lpVtbl -> SetPalette(This,dwColors,pPalette)

#define IOverlay_GetDefaultColorKey(This,pColorKey)	\
    (This)->lpVtbl -> GetDefaultColorKey(This,pColorKey)

#define IOverlay_GetColorKey(This,pColorKey)	\
    (This)->lpVtbl -> GetColorKey(This,pColorKey)

#define IOverlay_SetColorKey(This,pColorKey)	\
    (This)->lpVtbl -> SetColorKey(This,pColorKey)

#define IOverlay_GetWindowHandle(This,pHwnd)	\
    (This)->lpVtbl -> GetWindowHandle(This,pHwnd)

#define IOverlay_GetClipList(This,pSourceRect,pDestinationRect,ppRgnData)	\
    (This)->lpVtbl -> GetClipList(This,pSourceRect,pDestinationRect,ppRgnData)

#define IOverlay_GetVideoPosition(This,pSourceRect,pDestinationRect)	\
    (This)->lpVtbl -> GetVideoPosition(This,pSourceRect,pDestinationRect)

#define IOverlay_Advise(This,pOverlayNotify,dwInterests)	\
    (This)->lpVtbl -> Advise(This,pOverlayNotify,dwInterests)

#define IOverlay_Unadvise(This)	\
    (This)->lpVtbl -> Unadvise(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IOverlay_GetPalette_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwColors,
    /* [out] */ PALETTEENTRY __RPC_FAR *__RPC_FAR *ppPalette);


void __RPC_STUB IOverlay_GetPalette_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_SetPalette_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [in] */ DWORD dwColors,
    /* [in] */ PALETTEENTRY __RPC_FAR *pPalette);


void __RPC_STUB IOverlay_SetPalette_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_GetDefaultColorKey_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [out] */ COLORKEY __RPC_FAR *pColorKey);


void __RPC_STUB IOverlay_GetDefaultColorKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_GetColorKey_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [out] */ COLORKEY __RPC_FAR *pColorKey);


void __RPC_STUB IOverlay_GetColorKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_SetColorKey_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [out][in] */ COLORKEY __RPC_FAR *pColorKey);


void __RPC_STUB IOverlay_SetColorKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_GetWindowHandle_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [out] */ HWND __RPC_FAR *pHwnd);


void __RPC_STUB IOverlay_GetWindowHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_GetClipList_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [out] */ RECT __RPC_FAR *pSourceRect,
    /* [out] */ RECT __RPC_FAR *pDestinationRect,
    /* [out] */ RGNDATA __RPC_FAR *__RPC_FAR *ppRgnData);


void __RPC_STUB IOverlay_GetClipList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_GetVideoPosition_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [out] */ RECT __RPC_FAR *pSourceRect,
    /* [out] */ RECT __RPC_FAR *pDestinationRect);


void __RPC_STUB IOverlay_GetVideoPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_Advise_Proxy( 
    IOverlay __RPC_FAR * This,
    /* [in] */ IOverlayNotify __RPC_FAR *pOverlayNotify,
    /* [in] */ DWORD dwInterests);


void __RPC_STUB IOverlay_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IOverlay_Unadvise_Proxy( 
    IOverlay __RPC_FAR * This);


void __RPC_STUB IOverlay_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOverlay_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0061
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IOverlay __RPC_FAR *POVERLAY;



extern RPC_IF_HANDLE __MIDL__intf_0061_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0061_v0_0_s_ifspec;

#ifndef __IMediaEventSink_INTERFACE_DEFINED__
#define __IMediaEventSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMediaEventSink
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMediaEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMediaEventSink : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Notify( 
            /* [in] */ long EventCode,
            /* [in] */ long EventParam1,
            /* [in] */ long EventParam2) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMediaEventSinkVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMediaEventSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMediaEventSink __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMediaEventSink __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Notify )( 
            IMediaEventSink __RPC_FAR * This,
            /* [in] */ long EventCode,
            /* [in] */ long EventParam1,
            /* [in] */ long EventParam2);
        
    } IMediaEventSinkVtbl;

    interface IMediaEventSink
    {
        CONST_VTBL struct IMediaEventSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMediaEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMediaEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMediaEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMediaEventSink_Notify(This,EventCode,EventParam1,EventParam2)	\
    (This)->lpVtbl -> Notify(This,EventCode,EventParam1,EventParam2)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMediaEventSink_Notify_Proxy( 
    IMediaEventSink __RPC_FAR * This,
    /* [in] */ long EventCode,
    /* [in] */ long EventParam1,
    /* [in] */ long EventParam2);


void __RPC_STUB IMediaEventSink_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMediaEventSink_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0062
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IMediaEventSink __RPC_FAR *PMEDIAEVENTSINK;



extern RPC_IF_HANDLE __MIDL__intf_0062_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0062_v0_0_s_ifspec;

#ifndef __IFileSourceFilter_INTERFACE_DEFINED__
#define __IFileSourceFilter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFileSourceFilter
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IFileSourceFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IFileSourceFilter : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Load( 
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
        virtual HRESULT __stdcall GetCurFile( 
            /* [out] */ LPOLESTR __RPC_FAR *ppszFileName,
            /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileSourceFilterVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IFileSourceFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IFileSourceFilter __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IFileSourceFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Load )( 
            IFileSourceFilter __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *GetCurFile )( 
            IFileSourceFilter __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszFileName,
            /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt);
        
    } IFileSourceFilterVtbl;

    interface IFileSourceFilter
    {
        CONST_VTBL struct IFileSourceFilterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileSourceFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFileSourceFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFileSourceFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFileSourceFilter_Load(This,pszFileName,pmt)	\
    (This)->lpVtbl -> Load(This,pszFileName,pmt)

#define IFileSourceFilter_GetCurFile(This,ppszFileName,pmt)	\
    (This)->lpVtbl -> GetCurFile(This,ppszFileName,pmt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IFileSourceFilter_Load_Proxy( 
    IFileSourceFilter __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszFileName,
    /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IFileSourceFilter_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileSourceFilter_GetCurFile_Proxy( 
    IFileSourceFilter __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppszFileName,
    /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IFileSourceFilter_GetCurFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFileSourceFilter_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0063
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IFileSourceFilter __RPC_FAR *PFILTERFILESOURCE;



extern RPC_IF_HANDLE __MIDL__intf_0063_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0063_v0_0_s_ifspec;

#ifndef __IFileSinkFilter_INTERFACE_DEFINED__
#define __IFileSinkFilter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFileSinkFilter
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IFileSinkFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IFileSinkFilter : public IUnknown
    {
    public:
        virtual HRESULT __stdcall SetFileName( 
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
        virtual HRESULT __stdcall GetCurFile( 
            /* [out] */ LPOLESTR __RPC_FAR *ppszFileName,
            /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileSinkFilterVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IFileSinkFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IFileSinkFilter __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IFileSinkFilter __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *SetFileName )( 
            IFileSinkFilter __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *GetCurFile )( 
            IFileSinkFilter __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszFileName,
            /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt);
        
    } IFileSinkFilterVtbl;

    interface IFileSinkFilter
    {
        CONST_VTBL struct IFileSinkFilterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileSinkFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFileSinkFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFileSinkFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFileSinkFilter_SetFileName(This,pszFileName,pmt)	\
    (This)->lpVtbl -> SetFileName(This,pszFileName,pmt)

#define IFileSinkFilter_GetCurFile(This,ppszFileName,pmt)	\
    (This)->lpVtbl -> GetCurFile(This,ppszFileName,pmt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IFileSinkFilter_SetFileName_Proxy( 
    IFileSinkFilter __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszFileName,
    /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IFileSinkFilter_SetFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileSinkFilter_GetCurFile_Proxy( 
    IFileSinkFilter __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppszFileName,
    /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pmt);


void __RPC_STUB IFileSinkFilter_GetCurFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFileSinkFilter_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0064
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IFileSinkFilter __RPC_FAR *PFILTERFILESINK;

			/* size is 48 */
typedef struct  _AsyncIOReq
    {
    DWORD engine[ 4 ];
    /* [size_is] */ BYTE __RPC_FAR *lpv;
    DWORD cb;
    DWORD dwError;
    DWORD cbDone;
    LARGE_INTEGER liPos;
    DWORD hEvent;
    DWORD dwUser;
    }	AsyncIOReq;



extern RPC_IF_HANDLE __MIDL__intf_0064_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0064_v0_0_s_ifspec;

#ifndef __IFileAsyncIO_INTERFACE_DEFINED__
#define __IFileAsyncIO_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFileAsyncIO
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IFileAsyncIO;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IFileAsyncIO : public IUnknown
    {
    public:
        virtual HRESULT __stdcall QueryAlignment( 
            /* [out] */ LPDWORD pdwAlign) = 0;
        
        virtual HRESULT __stdcall Read( 
            /* [in] */ AsyncIOReq __RPC_FAR *pReq) = 0;
        
        virtual HRESULT __stdcall Write( 
            /* [in] */ AsyncIOReq __RPC_FAR *pReq) = 0;
        
        virtual HRESULT __stdcall WaitForNext( 
            /* [out] */ AsyncIOReq __RPC_FAR *__RPC_FAR *ppReq,
            /* [in] */ DWORD dwTimeout) = 0;
        
        virtual HRESULT __stdcall WaitForSpecific( 
            /* [out] */ AsyncIOReq __RPC_FAR *pReq,
            /* [in] */ DWORD dwTimeout) = 0;
        
        virtual HRESULT __stdcall DiscardPending( void) = 0;
        
        virtual HRESULT __stdcall Flush( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileAsyncIOVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IFileAsyncIO __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IFileAsyncIO __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IFileAsyncIO __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *QueryAlignment )( 
            IFileAsyncIO __RPC_FAR * This,
            /* [out] */ LPDWORD pdwAlign);
        
        HRESULT ( __stdcall __RPC_FAR *Read )( 
            IFileAsyncIO __RPC_FAR * This,
            /* [in] */ AsyncIOReq __RPC_FAR *pReq);
        
        HRESULT ( __stdcall __RPC_FAR *Write )( 
            IFileAsyncIO __RPC_FAR * This,
            /* [in] */ AsyncIOReq __RPC_FAR *pReq);
        
        HRESULT ( __stdcall __RPC_FAR *WaitForNext )( 
            IFileAsyncIO __RPC_FAR * This,
            /* [out] */ AsyncIOReq __RPC_FAR *__RPC_FAR *ppReq,
            /* [in] */ DWORD dwTimeout);
        
        HRESULT ( __stdcall __RPC_FAR *WaitForSpecific )( 
            IFileAsyncIO __RPC_FAR * This,
            /* [out] */ AsyncIOReq __RPC_FAR *pReq,
            /* [in] */ DWORD dwTimeout);
        
        HRESULT ( __stdcall __RPC_FAR *DiscardPending )( 
            IFileAsyncIO __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Flush )( 
            IFileAsyncIO __RPC_FAR * This);
        
    } IFileAsyncIOVtbl;

    interface IFileAsyncIO
    {
        CONST_VTBL struct IFileAsyncIOVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileAsyncIO_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFileAsyncIO_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFileAsyncIO_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFileAsyncIO_QueryAlignment(This,pdwAlign)	\
    (This)->lpVtbl -> QueryAlignment(This,pdwAlign)

#define IFileAsyncIO_Read(This,pReq)	\
    (This)->lpVtbl -> Read(This,pReq)

#define IFileAsyncIO_Write(This,pReq)	\
    (This)->lpVtbl -> Write(This,pReq)

#define IFileAsyncIO_WaitForNext(This,ppReq,dwTimeout)	\
    (This)->lpVtbl -> WaitForNext(This,ppReq,dwTimeout)

#define IFileAsyncIO_WaitForSpecific(This,pReq,dwTimeout)	\
    (This)->lpVtbl -> WaitForSpecific(This,pReq,dwTimeout)

#define IFileAsyncIO_DiscardPending(This)	\
    (This)->lpVtbl -> DiscardPending(This)

#define IFileAsyncIO_Flush(This)	\
    (This)->lpVtbl -> Flush(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IFileAsyncIO_QueryAlignment_Proxy( 
    IFileAsyncIO __RPC_FAR * This,
    /* [out] */ LPDWORD pdwAlign);


void __RPC_STUB IFileAsyncIO_QueryAlignment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileAsyncIO_Read_Proxy( 
    IFileAsyncIO __RPC_FAR * This,
    /* [in] */ AsyncIOReq __RPC_FAR *pReq);


void __RPC_STUB IFileAsyncIO_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileAsyncIO_Write_Proxy( 
    IFileAsyncIO __RPC_FAR * This,
    /* [in] */ AsyncIOReq __RPC_FAR *pReq);


void __RPC_STUB IFileAsyncIO_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileAsyncIO_WaitForNext_Proxy( 
    IFileAsyncIO __RPC_FAR * This,
    /* [out] */ AsyncIOReq __RPC_FAR *__RPC_FAR *ppReq,
    /* [in] */ DWORD dwTimeout);


void __RPC_STUB IFileAsyncIO_WaitForNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileAsyncIO_WaitForSpecific_Proxy( 
    IFileAsyncIO __RPC_FAR * This,
    /* [out] */ AsyncIOReq __RPC_FAR *pReq,
    /* [in] */ DWORD dwTimeout);


void __RPC_STUB IFileAsyncIO_WaitForSpecific_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileAsyncIO_DiscardPending_Proxy( 
    IFileAsyncIO __RPC_FAR * This);


void __RPC_STUB IFileAsyncIO_DiscardPending_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IFileAsyncIO_Flush_Proxy( 
    IFileAsyncIO __RPC_FAR * This);


void __RPC_STUB IFileAsyncIO_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFileAsyncIO_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0065
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef IFileAsyncIO __RPC_FAR *PFILEASYNCIO;



extern RPC_IF_HANDLE __MIDL__intf_0065_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0065_v0_0_s_ifspec;

#ifndef __IGraphBuilder_INTERFACE_DEFINED__
#define __IGraphBuilder_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGraphBuilder
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][local][object] */ 



EXTERN_C const IID IID_IGraphBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGraphBuilder : public IFilterGraph
    {
    public:
        virtual HRESULT __stdcall Connect( 
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn) = 0;
        
        virtual HRESULT __stdcall Render( 
            /* [in] */ IPin __RPC_FAR *ppinOut) = 0;
        
        virtual HRESULT __stdcall RenderFile( 
            /* [in] */ LPCWSTR lpcwstrFile,
            /* [in] */ LPCWSTR lpcwstrPlayList) = 0;
        
        virtual HRESULT __stdcall AddSourceFilter( 
            /* [in] */ LPCWSTR lpcwstrFileName,
            /* [in] */ LPCWSTR lpcwstrFilterName,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter) = 0;
        
        virtual HRESULT __stdcall SetLogFile( 
            /* [in] */ HANDLE hFile) = 0;
        
        virtual HRESULT __stdcall Abort( void) = 0;
        
        virtual HRESULT __stdcall ShouldOperationContinue( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGraphBuilderVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IGraphBuilder __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IGraphBuilder __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *AddFilter )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pFilter,
            /* [string][in] */ LPCWSTR pName);
        
        HRESULT ( __stdcall __RPC_FAR *RemoveFilter )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pFilter);
        
        HRESULT ( __stdcall __RPC_FAR *EnumFilters )( 
            IGraphBuilder __RPC_FAR * This,
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *FindFilterByName )( 
            IGraphBuilder __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pName,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter);
        
        HRESULT ( __stdcall __RPC_FAR *ConnectDirect )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *Reconnect )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppin);
        
        HRESULT ( __stdcall __RPC_FAR *Disconnect )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppin);
        
        HRESULT ( __stdcall __RPC_FAR *SetDefaultSyncSource )( 
            IGraphBuilder __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Connect )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn);
        
        HRESULT ( __stdcall __RPC_FAR *Render )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut);
        
        HRESULT ( __stdcall __RPC_FAR *RenderFile )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ LPCWSTR lpcwstrFile,
            /* [in] */ LPCWSTR lpcwstrPlayList);
        
        HRESULT ( __stdcall __RPC_FAR *AddSourceFilter )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ LPCWSTR lpcwstrFileName,
            /* [in] */ LPCWSTR lpcwstrFilterName,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter);
        
        HRESULT ( __stdcall __RPC_FAR *SetLogFile )( 
            IGraphBuilder __RPC_FAR * This,
            /* [in] */ HANDLE hFile);
        
        HRESULT ( __stdcall __RPC_FAR *Abort )( 
            IGraphBuilder __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *ShouldOperationContinue )( 
            IGraphBuilder __RPC_FAR * This);
        
    } IGraphBuilderVtbl;

    interface IGraphBuilder
    {
        CONST_VTBL struct IGraphBuilderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGraphBuilder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGraphBuilder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGraphBuilder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGraphBuilder_AddFilter(This,pFilter,pName)	\
    (This)->lpVtbl -> AddFilter(This,pFilter,pName)

#define IGraphBuilder_RemoveFilter(This,pFilter)	\
    (This)->lpVtbl -> RemoveFilter(This,pFilter)

#define IGraphBuilder_EnumFilters(This,ppEnum)	\
    (This)->lpVtbl -> EnumFilters(This,ppEnum)

#define IGraphBuilder_FindFilterByName(This,pName,ppFilter)	\
    (This)->lpVtbl -> FindFilterByName(This,pName,ppFilter)

#define IGraphBuilder_ConnectDirect(This,ppinOut,ppinIn,pmt)	\
    (This)->lpVtbl -> ConnectDirect(This,ppinOut,ppinIn,pmt)

#define IGraphBuilder_Reconnect(This,ppin)	\
    (This)->lpVtbl -> Reconnect(This,ppin)

#define IGraphBuilder_Disconnect(This,ppin)	\
    (This)->lpVtbl -> Disconnect(This,ppin)

#define IGraphBuilder_SetDefaultSyncSource(This)	\
    (This)->lpVtbl -> SetDefaultSyncSource(This)


#define IGraphBuilder_Connect(This,ppinOut,ppinIn)	\
    (This)->lpVtbl -> Connect(This,ppinOut,ppinIn)

#define IGraphBuilder_Render(This,ppinOut)	\
    (This)->lpVtbl -> Render(This,ppinOut)

#define IGraphBuilder_RenderFile(This,lpcwstrFile,lpcwstrPlayList)	\
    (This)->lpVtbl -> RenderFile(This,lpcwstrFile,lpcwstrPlayList)

#define IGraphBuilder_AddSourceFilter(This,lpcwstrFileName,lpcwstrFilterName,ppFilter)	\
    (This)->lpVtbl -> AddSourceFilter(This,lpcwstrFileName,lpcwstrFilterName,ppFilter)

#define IGraphBuilder_SetLogFile(This,hFile)	\
    (This)->lpVtbl -> SetLogFile(This,hFile)

#define IGraphBuilder_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IGraphBuilder_ShouldOperationContinue(This)	\
    (This)->lpVtbl -> ShouldOperationContinue(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IGraphBuilder_Connect_Proxy( 
    IGraphBuilder __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppinOut,
    /* [in] */ IPin __RPC_FAR *ppinIn);


void __RPC_STUB IGraphBuilder_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IGraphBuilder_Render_Proxy( 
    IGraphBuilder __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppinOut);


void __RPC_STUB IGraphBuilder_Render_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IGraphBuilder_RenderFile_Proxy( 
    IGraphBuilder __RPC_FAR * This,
    /* [in] */ LPCWSTR lpcwstrFile,
    /* [in] */ LPCWSTR lpcwstrPlayList);


void __RPC_STUB IGraphBuilder_RenderFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IGraphBuilder_AddSourceFilter_Proxy( 
    IGraphBuilder __RPC_FAR * This,
    /* [in] */ LPCWSTR lpcwstrFileName,
    /* [in] */ LPCWSTR lpcwstrFilterName,
    /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter);


void __RPC_STUB IGraphBuilder_AddSourceFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IGraphBuilder_SetLogFile_Proxy( 
    IGraphBuilder __RPC_FAR * This,
    /* [in] */ HANDLE hFile);


void __RPC_STUB IGraphBuilder_SetLogFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IGraphBuilder_Abort_Proxy( 
    IGraphBuilder __RPC_FAR * This);


void __RPC_STUB IGraphBuilder_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IGraphBuilder_ShouldOperationContinue_Proxy( 
    IGraphBuilder __RPC_FAR * This);


void __RPC_STUB IGraphBuilder_ShouldOperationContinue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGraphBuilder_INTERFACE_DEFINED__ */


#ifndef __IStreamBuilder_INTERFACE_DEFINED__
#define __IStreamBuilder_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IStreamBuilder
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][local][object] */ 



EXTERN_C const IID IID_IStreamBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IStreamBuilder : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Render( 
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IGraphBuilder __RPC_FAR *pGraph) = 0;
        
        virtual HRESULT __stdcall Backout( 
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IGraphBuilder __RPC_FAR *pGraph) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStreamBuilderVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IStreamBuilder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IStreamBuilder __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IStreamBuilder __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Render )( 
            IStreamBuilder __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IGraphBuilder __RPC_FAR *pGraph);
        
        HRESULT ( __stdcall __RPC_FAR *Backout )( 
            IStreamBuilder __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IGraphBuilder __RPC_FAR *pGraph);
        
    } IStreamBuilderVtbl;

    interface IStreamBuilder
    {
        CONST_VTBL struct IStreamBuilderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStreamBuilder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStreamBuilder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStreamBuilder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStreamBuilder_Render(This,ppinOut,pGraph)	\
    (This)->lpVtbl -> Render(This,ppinOut,pGraph)

#define IStreamBuilder_Backout(This,ppinOut,pGraph)	\
    (This)->lpVtbl -> Backout(This,ppinOut,pGraph)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IStreamBuilder_Render_Proxy( 
    IStreamBuilder __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppinOut,
    /* [in] */ IGraphBuilder __RPC_FAR *pGraph);


void __RPC_STUB IStreamBuilder_Render_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IStreamBuilder_Backout_Proxy( 
    IStreamBuilder __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppinOut,
    /* [in] */ IGraphBuilder __RPC_FAR *pGraph);


void __RPC_STUB IStreamBuilder_Backout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStreamBuilder_INTERFACE_DEFINED__ */


#ifndef __IAsyncReader_INTERFACE_DEFINED__
#define __IAsyncReader_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAsyncReader
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IAsyncReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAsyncReader : public IUnknown
    {
    public:
        virtual HRESULT __stdcall RequestAllocator( 
            /* [in] */ IMemAllocator __RPC_FAR *pPreferred,
            /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps,
            /* [out] */ IMemAllocator __RPC_FAR *__RPC_FAR *ppActual) = 0;
        
        virtual HRESULT __stdcall Request( 
            /* [in] */ IMediaSample __RPC_FAR *pSample,
            /* [in] */ DWORD dwUser) = 0;
        
        virtual HRESULT __stdcall WaitForNext( 
            /* [in] */ DWORD dwTimeout,
            /* [out] */ IMediaSample __RPC_FAR *__RPC_FAR *ppSample,
            /* [out] */ DWORD __RPC_FAR *pdwUser) = 0;
        
        virtual HRESULT __stdcall SyncReadAligned( 
            /* [in] */ IMediaSample __RPC_FAR *pSample) = 0;
        
        virtual HRESULT __stdcall SyncRead( 
            /* [in] */ LONGLONG llPosition,
            /* [in] */ LONG lLength,
            /* [size_is][out] */ BYTE __RPC_FAR *pBuffer) = 0;
        
        virtual HRESULT __stdcall Length( 
            /* [out] */ LONGLONG __RPC_FAR *pTotal,
            /* [out] */ LONGLONG __RPC_FAR *pAvailable) = 0;
        
        virtual HRESULT __stdcall BeginFlush( void) = 0;
        
        virtual HRESULT __stdcall EndFlush( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAsyncReaderVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IAsyncReader __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IAsyncReader __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IAsyncReader __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *RequestAllocator )( 
            IAsyncReader __RPC_FAR * This,
            /* [in] */ IMemAllocator __RPC_FAR *pPreferred,
            /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps,
            /* [out] */ IMemAllocator __RPC_FAR *__RPC_FAR *ppActual);
        
        HRESULT ( __stdcall __RPC_FAR *Request )( 
            IAsyncReader __RPC_FAR * This,
            /* [in] */ IMediaSample __RPC_FAR *pSample,
            /* [in] */ DWORD dwUser);
        
        HRESULT ( __stdcall __RPC_FAR *WaitForNext )( 
            IAsyncReader __RPC_FAR * This,
            /* [in] */ DWORD dwTimeout,
            /* [out] */ IMediaSample __RPC_FAR *__RPC_FAR *ppSample,
            /* [out] */ DWORD __RPC_FAR *pdwUser);
        
        HRESULT ( __stdcall __RPC_FAR *SyncReadAligned )( 
            IAsyncReader __RPC_FAR * This,
            /* [in] */ IMediaSample __RPC_FAR *pSample);
        
        HRESULT ( __stdcall __RPC_FAR *SyncRead )( 
            IAsyncReader __RPC_FAR * This,
            /* [in] */ LONGLONG llPosition,
            /* [in] */ LONG lLength,
            /* [size_is][out] */ BYTE __RPC_FAR *pBuffer);
        
        HRESULT ( __stdcall __RPC_FAR *Length )( 
            IAsyncReader __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pTotal,
            /* [out] */ LONGLONG __RPC_FAR *pAvailable);
        
        HRESULT ( __stdcall __RPC_FAR *BeginFlush )( 
            IAsyncReader __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *EndFlush )( 
            IAsyncReader __RPC_FAR * This);
        
    } IAsyncReaderVtbl;

    interface IAsyncReader
    {
        CONST_VTBL struct IAsyncReaderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAsyncReader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAsyncReader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAsyncReader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAsyncReader_RequestAllocator(This,pPreferred,pProps,ppActual)	\
    (This)->lpVtbl -> RequestAllocator(This,pPreferred,pProps,ppActual)

#define IAsyncReader_Request(This,pSample,dwUser)	\
    (This)->lpVtbl -> Request(This,pSample,dwUser)

#define IAsyncReader_WaitForNext(This,dwTimeout,ppSample,pdwUser)	\
    (This)->lpVtbl -> WaitForNext(This,dwTimeout,ppSample,pdwUser)

#define IAsyncReader_SyncReadAligned(This,pSample)	\
    (This)->lpVtbl -> SyncReadAligned(This,pSample)

#define IAsyncReader_SyncRead(This,llPosition,lLength,pBuffer)	\
    (This)->lpVtbl -> SyncRead(This,llPosition,lLength,pBuffer)

#define IAsyncReader_Length(This,pTotal,pAvailable)	\
    (This)->lpVtbl -> Length(This,pTotal,pAvailable)

#define IAsyncReader_BeginFlush(This)	\
    (This)->lpVtbl -> BeginFlush(This)

#define IAsyncReader_EndFlush(This)	\
    (This)->lpVtbl -> EndFlush(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IAsyncReader_RequestAllocator_Proxy( 
    IAsyncReader __RPC_FAR * This,
    /* [in] */ IMemAllocator __RPC_FAR *pPreferred,
    /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps,
    /* [out] */ IMemAllocator __RPC_FAR *__RPC_FAR *ppActual);


void __RPC_STUB IAsyncReader_RequestAllocator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAsyncReader_Request_Proxy( 
    IAsyncReader __RPC_FAR * This,
    /* [in] */ IMediaSample __RPC_FAR *pSample,
    /* [in] */ DWORD dwUser);


void __RPC_STUB IAsyncReader_Request_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAsyncReader_WaitForNext_Proxy( 
    IAsyncReader __RPC_FAR * This,
    /* [in] */ DWORD dwTimeout,
    /* [out] */ IMediaSample __RPC_FAR *__RPC_FAR *ppSample,
    /* [out] */ DWORD __RPC_FAR *pdwUser);


void __RPC_STUB IAsyncReader_WaitForNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAsyncReader_SyncReadAligned_Proxy( 
    IAsyncReader __RPC_FAR * This,
    /* [in] */ IMediaSample __RPC_FAR *pSample);


void __RPC_STUB IAsyncReader_SyncReadAligned_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAsyncReader_SyncRead_Proxy( 
    IAsyncReader __RPC_FAR * This,
    /* [in] */ LONGLONG llPosition,
    /* [in] */ LONG lLength,
    /* [size_is][out] */ BYTE __RPC_FAR *pBuffer);


void __RPC_STUB IAsyncReader_SyncRead_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAsyncReader_Length_Proxy( 
    IAsyncReader __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pTotal,
    /* [out] */ LONGLONG __RPC_FAR *pAvailable);


void __RPC_STUB IAsyncReader_Length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAsyncReader_BeginFlush_Proxy( 
    IAsyncReader __RPC_FAR * This);


void __RPC_STUB IAsyncReader_BeginFlush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAsyncReader_EndFlush_Proxy( 
    IAsyncReader __RPC_FAR * This);


void __RPC_STUB IAsyncReader_EndFlush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAsyncReader_INTERFACE_DEFINED__ */


#ifndef __IGraphVersion_INTERFACE_DEFINED__
#define __IGraphVersion_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGraphVersion
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IGraphVersion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGraphVersion : public IUnknown
    {
    public:
        virtual HRESULT __stdcall QueryVersion( 
            LONG __RPC_FAR *pVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGraphVersionVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IGraphVersion __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IGraphVersion __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IGraphVersion __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *QueryVersion )( 
            IGraphVersion __RPC_FAR * This,
            LONG __RPC_FAR *pVersion);
        
    } IGraphVersionVtbl;

    interface IGraphVersion
    {
        CONST_VTBL struct IGraphVersionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGraphVersion_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGraphVersion_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGraphVersion_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGraphVersion_QueryVersion(This,pVersion)	\
    (This)->lpVtbl -> QueryVersion(This,pVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IGraphVersion_QueryVersion_Proxy( 
    IGraphVersion __RPC_FAR * This,
    LONG __RPC_FAR *pVersion);


void __RPC_STUB IGraphVersion_QueryVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGraphVersion_INTERFACE_DEFINED__ */


#ifndef __IResourceConsumer_INTERFACE_DEFINED__
#define __IResourceConsumer_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResourceConsumer
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IResourceConsumer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IResourceConsumer : public IUnknown
    {
    public:
        virtual HRESULT __stdcall AcquireResource( 
            /* [in] */ LONG idResource) = 0;
        
        virtual HRESULT __stdcall ReleaseResource( 
            /* [in] */ LONG idResource) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResourceConsumerVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IResourceConsumer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IResourceConsumer __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IResourceConsumer __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *AcquireResource )( 
            IResourceConsumer __RPC_FAR * This,
            /* [in] */ LONG idResource);
        
        HRESULT ( __stdcall __RPC_FAR *ReleaseResource )( 
            IResourceConsumer __RPC_FAR * This,
            /* [in] */ LONG idResource);
        
    } IResourceConsumerVtbl;

    interface IResourceConsumer
    {
        CONST_VTBL struct IResourceConsumerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResourceConsumer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResourceConsumer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResourceConsumer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResourceConsumer_AcquireResource(This,idResource)	\
    (This)->lpVtbl -> AcquireResource(This,idResource)

#define IResourceConsumer_ReleaseResource(This,idResource)	\
    (This)->lpVtbl -> ReleaseResource(This,idResource)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IResourceConsumer_AcquireResource_Proxy( 
    IResourceConsumer __RPC_FAR * This,
    /* [in] */ LONG idResource);


void __RPC_STUB IResourceConsumer_AcquireResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceConsumer_ReleaseResource_Proxy( 
    IResourceConsumer __RPC_FAR * This,
    /* [in] */ LONG idResource);


void __RPC_STUB IResourceConsumer_ReleaseResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResourceConsumer_INTERFACE_DEFINED__ */


#ifndef __IResourceManager_INTERFACE_DEFINED__
#define __IResourceManager_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResourceManager
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IResourceManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IResourceManager : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Register( 
            /* [in] */ LPCWSTR pName,
            /* [in] */ LONG cResource,
            /* [out] */ LONG __RPC_FAR *plToken) = 0;
        
        virtual HRESULT __stdcall RegisterGroup( 
            /* [in] */ LPCWSTR pName,
            /* [in] */ LONG cResource,
            /* [size_is][in] */ LONG __RPC_FAR *palTokens,
            /* [out] */ LONG __RPC_FAR *plToken) = 0;
        
        virtual HRESULT __stdcall RequestResource( 
            /* [in] */ LONG idResource,
            /* [in] */ IUnknown __RPC_FAR *pFocusObject,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer) = 0;
        
        virtual HRESULT __stdcall NotifyAcquire( 
            /* [in] */ LONG idResource,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT __stdcall NotifyRelease( 
            /* [in] */ LONG idResource,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer,
            /* [in] */ BOOL bStillWant) = 0;
        
        virtual HRESULT __stdcall CancelRequest( 
            /* [in] */ LONG idResource,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer) = 0;
        
        virtual HRESULT __stdcall SetFocus( 
            /* [in] */ IUnknown __RPC_FAR *pFocusObject) = 0;
        
        virtual HRESULT __stdcall ReleaseFocus( 
            /* [in] */ IUnknown __RPC_FAR *pFocusObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResourceManagerVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IResourceManager __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IResourceManager __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Register )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ LPCWSTR pName,
            /* [in] */ LONG cResource,
            /* [out] */ LONG __RPC_FAR *plToken);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterGroup )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ LPCWSTR pName,
            /* [in] */ LONG cResource,
            /* [size_is][in] */ LONG __RPC_FAR *palTokens,
            /* [out] */ LONG __RPC_FAR *plToken);
        
        HRESULT ( __stdcall __RPC_FAR *RequestResource )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ LONG idResource,
            /* [in] */ IUnknown __RPC_FAR *pFocusObject,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer);
        
        HRESULT ( __stdcall __RPC_FAR *NotifyAcquire )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ LONG idResource,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer,
            /* [in] */ HRESULT hr);
        
        HRESULT ( __stdcall __RPC_FAR *NotifyRelease )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ LONG idResource,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer,
            /* [in] */ BOOL bStillWant);
        
        HRESULT ( __stdcall __RPC_FAR *CancelRequest )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ LONG idResource,
            /* [in] */ IResourceConsumer __RPC_FAR *pConsumer);
        
        HRESULT ( __stdcall __RPC_FAR *SetFocus )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pFocusObject);
        
        HRESULT ( __stdcall __RPC_FAR *ReleaseFocus )( 
            IResourceManager __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pFocusObject);
        
    } IResourceManagerVtbl;

    interface IResourceManager
    {
        CONST_VTBL struct IResourceManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResourceManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResourceManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResourceManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResourceManager_Register(This,pName,cResource,plToken)	\
    (This)->lpVtbl -> Register(This,pName,cResource,plToken)

#define IResourceManager_RegisterGroup(This,pName,cResource,palTokens,plToken)	\
    (This)->lpVtbl -> RegisterGroup(This,pName,cResource,palTokens,plToken)

#define IResourceManager_RequestResource(This,idResource,pFocusObject,pConsumer)	\
    (This)->lpVtbl -> RequestResource(This,idResource,pFocusObject,pConsumer)

#define IResourceManager_NotifyAcquire(This,idResource,pConsumer,hr)	\
    (This)->lpVtbl -> NotifyAcquire(This,idResource,pConsumer,hr)

#define IResourceManager_NotifyRelease(This,idResource,pConsumer,bStillWant)	\
    (This)->lpVtbl -> NotifyRelease(This,idResource,pConsumer,bStillWant)

#define IResourceManager_CancelRequest(This,idResource,pConsumer)	\
    (This)->lpVtbl -> CancelRequest(This,idResource,pConsumer)

#define IResourceManager_SetFocus(This,pFocusObject)	\
    (This)->lpVtbl -> SetFocus(This,pFocusObject)

#define IResourceManager_ReleaseFocus(This,pFocusObject)	\
    (This)->lpVtbl -> ReleaseFocus(This,pFocusObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IResourceManager_Register_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ LPCWSTR pName,
    /* [in] */ LONG cResource,
    /* [out] */ LONG __RPC_FAR *plToken);


void __RPC_STUB IResourceManager_Register_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceManager_RegisterGroup_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ LPCWSTR pName,
    /* [in] */ LONG cResource,
    /* [size_is][in] */ LONG __RPC_FAR *palTokens,
    /* [out] */ LONG __RPC_FAR *plToken);


void __RPC_STUB IResourceManager_RegisterGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceManager_RequestResource_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ LONG idResource,
    /* [in] */ IUnknown __RPC_FAR *pFocusObject,
    /* [in] */ IResourceConsumer __RPC_FAR *pConsumer);


void __RPC_STUB IResourceManager_RequestResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceManager_NotifyAcquire_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ LONG idResource,
    /* [in] */ IResourceConsumer __RPC_FAR *pConsumer,
    /* [in] */ HRESULT hr);


void __RPC_STUB IResourceManager_NotifyAcquire_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceManager_NotifyRelease_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ LONG idResource,
    /* [in] */ IResourceConsumer __RPC_FAR *pConsumer,
    /* [in] */ BOOL bStillWant);


void __RPC_STUB IResourceManager_NotifyRelease_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceManager_CancelRequest_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ LONG idResource,
    /* [in] */ IResourceConsumer __RPC_FAR *pConsumer);


void __RPC_STUB IResourceManager_CancelRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceManager_SetFocus_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pFocusObject);


void __RPC_STUB IResourceManager_SetFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IResourceManager_ReleaseFocus_Proxy( 
    IResourceManager __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pFocusObject);


void __RPC_STUB IResourceManager_ReleaseFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResourceManager_INTERFACE_DEFINED__ */


#ifndef __IDistributorNotify_INTERFACE_DEFINED__
#define __IDistributorNotify_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDistributorNotify
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDistributorNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDistributorNotify : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Stop( void) = 0;
        
        virtual HRESULT __stdcall Pause( void) = 0;
        
        virtual HRESULT __stdcall Run( 
            REFERENCE_TIME tStart) = 0;
        
        virtual HRESULT __stdcall SetSyncSource( 
            /* [in] */ IReferenceClock __RPC_FAR *pClock) = 0;
        
        virtual HRESULT __stdcall NotifyGraphChange( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDistributorNotifyVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IDistributorNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IDistributorNotify __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IDistributorNotify __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Stop )( 
            IDistributorNotify __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Pause )( 
            IDistributorNotify __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Run )( 
            IDistributorNotify __RPC_FAR * This,
            REFERENCE_TIME tStart);
        
        HRESULT ( __stdcall __RPC_FAR *SetSyncSource )( 
            IDistributorNotify __RPC_FAR * This,
            /* [in] */ IReferenceClock __RPC_FAR *pClock);
        
        HRESULT ( __stdcall __RPC_FAR *NotifyGraphChange )( 
            IDistributorNotify __RPC_FAR * This);
        
    } IDistributorNotifyVtbl;

    interface IDistributorNotify
    {
        CONST_VTBL struct IDistributorNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDistributorNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDistributorNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDistributorNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDistributorNotify_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IDistributorNotify_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IDistributorNotify_Run(This,tStart)	\
    (This)->lpVtbl -> Run(This,tStart)

#define IDistributorNotify_SetSyncSource(This,pClock)	\
    (This)->lpVtbl -> SetSyncSource(This,pClock)

#define IDistributorNotify_NotifyGraphChange(This)	\
    (This)->lpVtbl -> NotifyGraphChange(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IDistributorNotify_Stop_Proxy( 
    IDistributorNotify __RPC_FAR * This);


void __RPC_STUB IDistributorNotify_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IDistributorNotify_Pause_Proxy( 
    IDistributorNotify __RPC_FAR * This);


void __RPC_STUB IDistributorNotify_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IDistributorNotify_Run_Proxy( 
    IDistributorNotify __RPC_FAR * This,
    REFERENCE_TIME tStart);


void __RPC_STUB IDistributorNotify_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IDistributorNotify_SetSyncSource_Proxy( 
    IDistributorNotify __RPC_FAR * This,
    /* [in] */ IReferenceClock __RPC_FAR *pClock);


void __RPC_STUB IDistributorNotify_SetSyncSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IDistributorNotify_NotifyGraphChange_Proxy( 
    IDistributorNotify __RPC_FAR * This);


void __RPC_STUB IDistributorNotify_NotifyGraphChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDistributorNotify_INTERFACE_DEFINED__ */


#ifndef __IAMovie_INTERFACE_DEFINED__
#define __IAMovie_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAMovie
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IAMovie;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAMovie : public IFilterGraph
    {
    public:
        virtual HRESULT __stdcall Connect( 
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn) = 0;
        
        virtual HRESULT __stdcall Render( 
            /* [in] */ IPin __RPC_FAR *ppinOut) = 0;
        
        virtual HRESULT __stdcall Run( void) = 0;
        
        virtual HRESULT __stdcall Pause( void) = 0;
        
        virtual HRESULT __stdcall Stop( void) = 0;
        
        virtual HRESULT __stdcall GetState( 
            /* [in] */ LONG msTimeout,
            /* [out] */ FILTER_STATE __RPC_FAR *pfs) = 0;
        
        virtual HRESULT __stdcall RenderFile( 
            /* [in] */ LPCWSTR strFilename) = 0;
        
        virtual HRESULT __stdcall AddSourceFilter( 
            /* [in] */ LPCWSTR strFilename,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppUnk) = 0;
        
        virtual HRESULT __stdcall GetEventHandle( 
            /* [out] */ HEVENT __RPC_FAR *hEvent) = 0;
        
        virtual HRESULT __stdcall GetEvent( 
            /* [out] */ long __RPC_FAR *lEventCode,
            /* [out] */ long __RPC_FAR *lParam1,
            /* [out] */ long __RPC_FAR *lParam2,
            /* [in] */ long msTimeout) = 0;
        
        virtual HRESULT __stdcall WaitForCompletion( 
            /* [in] */ long msTimeout,
            /* [out] */ long __RPC_FAR *pEvCode) = 0;
        
        virtual HRESULT __stdcall CancelDefaultHandling( 
            /* [in] */ long lEvCode) = 0;
        
        virtual HRESULT __stdcall RestoreDefaultHandling( 
            /* [in] */ long lEvCode) = 0;
        
        virtual HRESULT __stdcall get_Duration( 
            /* [out] */ REFTIME __RPC_FAR *plength) = 0;
        
        virtual HRESULT __stdcall put_CurrentPosition( 
            /* [in] */ REFTIME llTime) = 0;
        
        virtual HRESULT __stdcall get_CurrentPosition( 
            /* [out] */ REFTIME __RPC_FAR *pllTime) = 0;
        
        virtual HRESULT __stdcall get_StopTime( 
            /* [out] */ REFTIME __RPC_FAR *pllTime) = 0;
        
        virtual HRESULT __stdcall put_StopTime( 
            /* [in] */ REFTIME llTime) = 0;
        
        virtual HRESULT __stdcall get_PrerollTime( 
            /* [out] */ REFTIME __RPC_FAR *pllTime) = 0;
        
        virtual HRESULT __stdcall put_PrerollTime( 
            /* [in] */ REFTIME llTime) = 0;
        
        virtual HRESULT __stdcall put_Rate( 
            /* [in] */ double dRate) = 0;
        
        virtual HRESULT __stdcall get_Rate( 
            /* [out] */ double __RPC_FAR *pdRate) = 0;
        
        virtual HRESULT __stdcall RemoveAllFilters( void) = 0;
        
        virtual HRESULT __stdcall Play( void) = 0;
        
        virtual HRESULT __stdcall PlayFile( 
            /* [in] */ LPCWSTR strFilename) = 0;
        
        virtual HRESULT __stdcall EnumFiltersByInterface( 
            /* [in] */ REFIID riid,
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT __stdcall EnumPins( 
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT __stdcall EnumPinsIn( 
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT __stdcall EnumPinsOut( 
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT __stdcall RenderAll( void) = 0;
        
        virtual HRESULT __stdcall RenderNewFile( 
            /* [in] */ LPCWSTR strFilename) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAMovieVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IAMovie __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *AddFilter )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pFilter,
            /* [string][in] */ LPCWSTR pName);
        
        HRESULT ( __stdcall __RPC_FAR *RemoveFilter )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pFilter);
        
        HRESULT ( __stdcall __RPC_FAR *EnumFilters )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *FindFilterByName )( 
            IAMovie __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pName,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppFilter);
        
        HRESULT ( __stdcall __RPC_FAR *ConnectDirect )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        HRESULT ( __stdcall __RPC_FAR *Reconnect )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppin);
        
        HRESULT ( __stdcall __RPC_FAR *Disconnect )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppin);
        
        HRESULT ( __stdcall __RPC_FAR *SetDefaultSyncSource )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Connect )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut,
            /* [in] */ IPin __RPC_FAR *ppinIn);
        
        HRESULT ( __stdcall __RPC_FAR *Render )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ IPin __RPC_FAR *ppinOut);
        
        HRESULT ( __stdcall __RPC_FAR *Run )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Pause )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Stop )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetState )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ LONG msTimeout,
            /* [out] */ FILTER_STATE __RPC_FAR *pfs);
        
        HRESULT ( __stdcall __RPC_FAR *RenderFile )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ LPCWSTR strFilename);
        
        HRESULT ( __stdcall __RPC_FAR *AddSourceFilter )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ LPCWSTR strFilename,
            /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppUnk);
        
        HRESULT ( __stdcall __RPC_FAR *GetEventHandle )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ HEVENT __RPC_FAR *hEvent);
        
        HRESULT ( __stdcall __RPC_FAR *GetEvent )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *lEventCode,
            /* [out] */ long __RPC_FAR *lParam1,
            /* [out] */ long __RPC_FAR *lParam2,
            /* [in] */ long msTimeout);
        
        HRESULT ( __stdcall __RPC_FAR *WaitForCompletion )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ long msTimeout,
            /* [out] */ long __RPC_FAR *pEvCode);
        
        HRESULT ( __stdcall __RPC_FAR *CancelDefaultHandling )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ long lEvCode);
        
        HRESULT ( __stdcall __RPC_FAR *RestoreDefaultHandling )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ long lEvCode);
        
        HRESULT ( __stdcall __RPC_FAR *get_Duration )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ REFTIME __RPC_FAR *plength);
        
        HRESULT ( __stdcall __RPC_FAR *put_CurrentPosition )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ REFTIME llTime);
        
        HRESULT ( __stdcall __RPC_FAR *get_CurrentPosition )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ REFTIME __RPC_FAR *pllTime);
        
        HRESULT ( __stdcall __RPC_FAR *get_StopTime )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ REFTIME __RPC_FAR *pllTime);
        
        HRESULT ( __stdcall __RPC_FAR *put_StopTime )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ REFTIME llTime);
        
        HRESULT ( __stdcall __RPC_FAR *get_PrerollTime )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ REFTIME __RPC_FAR *pllTime);
        
        HRESULT ( __stdcall __RPC_FAR *put_PrerollTime )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ REFTIME llTime);
        
        HRESULT ( __stdcall __RPC_FAR *put_Rate )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ double dRate);
        
        HRESULT ( __stdcall __RPC_FAR *get_Rate )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ double __RPC_FAR *pdRate);
        
        HRESULT ( __stdcall __RPC_FAR *RemoveAllFilters )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Play )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *PlayFile )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ LPCWSTR strFilename);
        
        HRESULT ( __stdcall __RPC_FAR *EnumFiltersByInterface )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *EnumPins )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *EnumPinsIn )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *EnumPinsOut )( 
            IAMovie __RPC_FAR * This,
            /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( __stdcall __RPC_FAR *RenderAll )( 
            IAMovie __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *RenderNewFile )( 
            IAMovie __RPC_FAR * This,
            /* [in] */ LPCWSTR strFilename);
        
    } IAMovieVtbl;

    interface IAMovie
    {
        CONST_VTBL struct IAMovieVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAMovie_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAMovie_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAMovie_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAMovie_AddFilter(This,pFilter,pName)	\
    (This)->lpVtbl -> AddFilter(This,pFilter,pName)

#define IAMovie_RemoveFilter(This,pFilter)	\
    (This)->lpVtbl -> RemoveFilter(This,pFilter)

#define IAMovie_EnumFilters(This,ppEnum)	\
    (This)->lpVtbl -> EnumFilters(This,ppEnum)

#define IAMovie_FindFilterByName(This,pName,ppFilter)	\
    (This)->lpVtbl -> FindFilterByName(This,pName,ppFilter)

#define IAMovie_ConnectDirect(This,ppinOut,ppinIn,pmt)	\
    (This)->lpVtbl -> ConnectDirect(This,ppinOut,ppinIn,pmt)

#define IAMovie_Reconnect(This,ppin)	\
    (This)->lpVtbl -> Reconnect(This,ppin)

#define IAMovie_Disconnect(This,ppin)	\
    (This)->lpVtbl -> Disconnect(This,ppin)

#define IAMovie_SetDefaultSyncSource(This)	\
    (This)->lpVtbl -> SetDefaultSyncSource(This)


#define IAMovie_Connect(This,ppinOut,ppinIn)	\
    (This)->lpVtbl -> Connect(This,ppinOut,ppinIn)

#define IAMovie_Render(This,ppinOut)	\
    (This)->lpVtbl -> Render(This,ppinOut)

#define IAMovie_Run(This)	\
    (This)->lpVtbl -> Run(This)

#define IAMovie_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IAMovie_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IAMovie_GetState(This,msTimeout,pfs)	\
    (This)->lpVtbl -> GetState(This,msTimeout,pfs)

#define IAMovie_RenderFile(This,strFilename)	\
    (This)->lpVtbl -> RenderFile(This,strFilename)

#define IAMovie_AddSourceFilter(This,strFilename,ppUnk)	\
    (This)->lpVtbl -> AddSourceFilter(This,strFilename,ppUnk)

#define IAMovie_GetEventHandle(This,hEvent)	\
    (This)->lpVtbl -> GetEventHandle(This,hEvent)

#define IAMovie_GetEvent(This,lEventCode,lParam1,lParam2,msTimeout)	\
    (This)->lpVtbl -> GetEvent(This,lEventCode,lParam1,lParam2,msTimeout)

#define IAMovie_WaitForCompletion(This,msTimeout,pEvCode)	\
    (This)->lpVtbl -> WaitForCompletion(This,msTimeout,pEvCode)

#define IAMovie_CancelDefaultHandling(This,lEvCode)	\
    (This)->lpVtbl -> CancelDefaultHandling(This,lEvCode)

#define IAMovie_RestoreDefaultHandling(This,lEvCode)	\
    (This)->lpVtbl -> RestoreDefaultHandling(This,lEvCode)

#define IAMovie_get_Duration(This,plength)	\
    (This)->lpVtbl -> get_Duration(This,plength)

#define IAMovie_put_CurrentPosition(This,llTime)	\
    (This)->lpVtbl -> put_CurrentPosition(This,llTime)

#define IAMovie_get_CurrentPosition(This,pllTime)	\
    (This)->lpVtbl -> get_CurrentPosition(This,pllTime)

#define IAMovie_get_StopTime(This,pllTime)	\
    (This)->lpVtbl -> get_StopTime(This,pllTime)

#define IAMovie_put_StopTime(This,llTime)	\
    (This)->lpVtbl -> put_StopTime(This,llTime)

#define IAMovie_get_PrerollTime(This,pllTime)	\
    (This)->lpVtbl -> get_PrerollTime(This,pllTime)

#define IAMovie_put_PrerollTime(This,llTime)	\
    (This)->lpVtbl -> put_PrerollTime(This,llTime)

#define IAMovie_put_Rate(This,dRate)	\
    (This)->lpVtbl -> put_Rate(This,dRate)

#define IAMovie_get_Rate(This,pdRate)	\
    (This)->lpVtbl -> get_Rate(This,pdRate)

#define IAMovie_RemoveAllFilters(This)	\
    (This)->lpVtbl -> RemoveAllFilters(This)

#define IAMovie_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IAMovie_PlayFile(This,strFilename)	\
    (This)->lpVtbl -> PlayFile(This,strFilename)

#define IAMovie_EnumFiltersByInterface(This,riid,ppEnum)	\
    (This)->lpVtbl -> EnumFiltersByInterface(This,riid,ppEnum)

#define IAMovie_EnumPins(This,ppEnum)	\
    (This)->lpVtbl -> EnumPins(This,ppEnum)

#define IAMovie_EnumPinsIn(This,ppEnum)	\
    (This)->lpVtbl -> EnumPinsIn(This,ppEnum)

#define IAMovie_EnumPinsOut(This,ppEnum)	\
    (This)->lpVtbl -> EnumPinsOut(This,ppEnum)

#define IAMovie_RenderAll(This)	\
    (This)->lpVtbl -> RenderAll(This)

#define IAMovie_RenderNewFile(This,strFilename)	\
    (This)->lpVtbl -> RenderNewFile(This,strFilename)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IAMovie_Connect_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppinOut,
    /* [in] */ IPin __RPC_FAR *ppinIn);


void __RPC_STUB IAMovie_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_Render_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ IPin __RPC_FAR *ppinOut);


void __RPC_STUB IAMovie_Render_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_Run_Proxy( 
    IAMovie __RPC_FAR * This);


void __RPC_STUB IAMovie_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_Pause_Proxy( 
    IAMovie __RPC_FAR * This);


void __RPC_STUB IAMovie_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_Stop_Proxy( 
    IAMovie __RPC_FAR * This);


void __RPC_STUB IAMovie_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_GetState_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ LONG msTimeout,
    /* [out] */ FILTER_STATE __RPC_FAR *pfs);


void __RPC_STUB IAMovie_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_RenderFile_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ LPCWSTR strFilename);


void __RPC_STUB IAMovie_RenderFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_AddSourceFilter_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ LPCWSTR strFilename,
    /* [out] */ IFilter __RPC_FAR *__RPC_FAR *ppUnk);


void __RPC_STUB IAMovie_AddSourceFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_GetEventHandle_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ HEVENT __RPC_FAR *hEvent);


void __RPC_STUB IAMovie_GetEventHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_GetEvent_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *lEventCode,
    /* [out] */ long __RPC_FAR *lParam1,
    /* [out] */ long __RPC_FAR *lParam2,
    /* [in] */ long msTimeout);


void __RPC_STUB IAMovie_GetEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_WaitForCompletion_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ long msTimeout,
    /* [out] */ long __RPC_FAR *pEvCode);


void __RPC_STUB IAMovie_WaitForCompletion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_CancelDefaultHandling_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ long lEvCode);


void __RPC_STUB IAMovie_CancelDefaultHandling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_RestoreDefaultHandling_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ long lEvCode);


void __RPC_STUB IAMovie_RestoreDefaultHandling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_get_Duration_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ REFTIME __RPC_FAR *plength);


void __RPC_STUB IAMovie_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_put_CurrentPosition_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ REFTIME llTime);


void __RPC_STUB IAMovie_put_CurrentPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_get_CurrentPosition_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ REFTIME __RPC_FAR *pllTime);


void __RPC_STUB IAMovie_get_CurrentPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_get_StopTime_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ REFTIME __RPC_FAR *pllTime);


void __RPC_STUB IAMovie_get_StopTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_put_StopTime_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ REFTIME llTime);


void __RPC_STUB IAMovie_put_StopTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_get_PrerollTime_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ REFTIME __RPC_FAR *pllTime);


void __RPC_STUB IAMovie_get_PrerollTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_put_PrerollTime_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ REFTIME llTime);


void __RPC_STUB IAMovie_put_PrerollTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_put_Rate_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ double dRate);


void __RPC_STUB IAMovie_put_Rate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_get_Rate_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ double __RPC_FAR *pdRate);


void __RPC_STUB IAMovie_get_Rate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_RemoveAllFilters_Proxy( 
    IAMovie __RPC_FAR * This);


void __RPC_STUB IAMovie_RemoveAllFilters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_Play_Proxy( 
    IAMovie __RPC_FAR * This);


void __RPC_STUB IAMovie_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_PlayFile_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ LPCWSTR strFilename);


void __RPC_STUB IAMovie_PlayFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_EnumFiltersByInterface_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [out] */ IEnumFilters __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IAMovie_EnumFiltersByInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_EnumPins_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IAMovie_EnumPins_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_EnumPinsIn_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IAMovie_EnumPinsIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_EnumPinsOut_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [out] */ IEnumPins __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IAMovie_EnumPinsOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_RenderAll_Proxy( 
    IAMovie __RPC_FAR * This);


void __RPC_STUB IAMovie_RenderAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IAMovie_RenderNewFile_Proxy( 
    IAMovie __RPC_FAR * This,
    /* [in] */ LPCWSTR strFilename);


void __RPC_STUB IAMovie_RenderNewFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAMovie_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0073
 * at Wed Jun 26 01:38:28 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


#pragma warning(default: 4152 4209 4211 4214 4310 4514)


extern RPC_IF_HANDLE __MIDL__intf_0073_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0073_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
