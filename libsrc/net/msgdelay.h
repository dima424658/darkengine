/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/net/RCS/msgdelay.h 1.2 1999/10/21 15:32:36 MAT Exp $
// declares cMessageDelayQueue

#include <lg.h>
#include <net.h>

#ifdef NET_ALLOW_SIMULATION
class cMessageDelayQueue
{
   // Used for simulating internet latency and lossiness.  Messages are queued, given
   // a random delay that obeys given parameters, and then made available for dequeing
   // after that delay.  Loss of messages is also similuated.

public:
   void Init();
   // Must be called before any other function.

   void SetInternetParameters(ulong lossPercent,ulong minLatency,ulong aveLatency,ulong maxLatency);
   void SetFullInternetParameters(ulong lossPercent,ulong minLatency,ulong aveLatency,ulong maxLatency,ulong spikePercent,ulong spikeLatency);
   // Set various parameters for simulating Internet conditions.  The latency
   // parameters are in millisecs, lossPercent is the likelihood of loosing a
   // message, although "guaranteed" messages will just be delayed.

   void GetInternetParameters(ulong *lossPercent,ulong *minLatency,ulong *aveLatency,ulong *maxLatency);
   void GetFullInternetParameters(ulong *lossPercent,ulong *minLatency,ulong *aveLatency,ulong *maxLatency,ulong *spikePercent,ulong *spikeLatency);
   // get the current parameters.

   void InternetParameterDialog();
   // Query the user for the desired internet parameters (uses "netstats.dll")

   void DelayMessage(ulong from,ulong to,ulong flags,void *data,ulong size);
   // Enqueue this message to be sent after a random delay (or loss) based on the
   // internet simulation parameters.
   
   void NextReadyMessage(ulong *from,ulong *to,ulong *flags,void **data,ulong *size);
   // Return the next message that has been delayed long enough and is now ready to
   // be sent.  If none are ready, *data will be null (all others will be unchanged).
};
#endif
