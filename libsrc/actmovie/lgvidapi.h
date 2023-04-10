#ifndef __LGVIDAPI_H
#define __LGVIDAPI_H

#include <strmif.h>
#include <mtype.h>

/*
  Interface that must be provided by the app that wants to use the 
  LG Video Renderer.
*/
DECLARE_INTERFACE_(IImageExtern, IUnknown)
{
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)(THIS) PURE;
   STDMETHOD_(ULONG,Release)(THIS) PURE;

   STDMETHOD_(BOOL, DrawImage) (THIS_ IMediaSample *pMediaSample) PURE;
   //   STDMETHOD_(void, SetTargetRect) (THIS_ RECT *pTargetRect) PURE;
   STDMETHOD_(void, SetSourceRect) (THIS_ RECT *pSourceRect) PURE;
   STDMETHOD_(void, GetTargetRect) (THIS_ RECT *pTargetRect) PURE;
   STDMETHOD_(void, GetSourceRect) (THIS_ RECT *pSourceRect) PURE;

   STDMETHOD(CheckMediaType) (THIS_ const CMediaType *pmtIn) PURE; 
   STDMETHOD(NotifyMediaType) (THIS_ CMediaType *pmt) PURE;
   STDMETHOD(GetMediaType) (THIS_ int iPosition, CMediaType *pMediaType) PURE;

   STDMETHOD(GetAllocator) (THIS_ IMemAllocator **ppAllocator) PURE;
   STDMETHOD(NotifyAllocator) (THIS_ IMemAllocator *pAllocator, BOOL bReadOnly) PURE;
};

/*
  Interface provided by the LG VideoRenderer to be used by the app
  to inform the renderer of its provided IImageExtern interface.
*/
DECLARE_INTERFACE_(IVideoRenderer, IUnknown)
{
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)(THIS) PURE;
   STDMETHOD_(ULONG,Release)(THIS) PURE;

   STDMETHOD(SetImageExtern) (THIS_ IImageExtern *pImageExtern) PURE;
};

#endif
        






