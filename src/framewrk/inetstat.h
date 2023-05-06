/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/inetstat.h,v 1.3 1999/08/05 16:58:05 Justin Exp $

#ifndef __INETSTAT_H
#pragma once
#define __INETSTAT_H

#ifdef PLAYTEST

F_DECLARE_INTERFACE(INetStats);

#undef INTERFACE
#define INTERFACE INetStats

DECLARE_INTERFACE_( INetStats, IUnknown )
{
   DECLARE_UNKNOWN_PURE();

   // Bandwidth used in previous 32 frames.  All parameters are output
   // parameters in seconds or bytes/second.
   STDMETHOD_(void, BandwidthStats)(THIS_ 
                                    float *timeSpan, 
                                    ulong *sendBW, 
                                    ulong *receiveBW,
                                    const char **culprit) PURE;

   // Bytes sent and received in the previous frame.
   STDMETHOD_(void, FrameStats)(THIS_ 
                                ulong *bytesSent, 
                                ulong *bytesReceived,
                                const char **culprit) PURE;

   // max bytes sent/recv in the last 5 seconds
   STDMETHOD_(void, MaxStats)(THIS_
                              ulong *bytesSent,
                              ulong *bytesReceived) PURE;
};

#define INetStats_BandwidthStats(p,a,b,c,d)  COMCall4(p, BandwidthStats, a, b, c, d)
#define INetStats_FrameStats(p,a,b,c)        COMCall3(p, FrameStats, a, b, c)
#define INetStats_MaxStats(p,a,b)            COMCall2(p, MaxStats, a, b)

#endif // PLAYTEST

#endif // !__INETSTAT_H
