/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __NET_H
#define __NET_H

#include <comtools.h>
#include <netguid.h>

#ifndef SHIP
#define NET_ALLOW_SIMULATION
#define NET_ALLOW_TIME_SYNCH
#endif

typedef uint32 tNetTransportPlayerID;

#define NET_PLAYER_ALL       ((tNetTransportPlayerID)0)
#define NET_PLAYER_SYSTEM    ((tNetTransportPlayerID)0)
#define NET_PLAYER_UNKNOWN   ((tNetTransportPlayerID)0xffffffff)

#define NET_SEND_GUARANTEED  0x00000001
#define NET_SEND_ASYNC       0x00000200
#define NET_DATA_LOCAL       0x00000001
#define NET_ENUM_REMOTE      0x00000010
#define NET_PLAYER_TYPE      0x00000001

#define NET_OK               ((HRESULT)0)
#define NET_NO_MESSAGES      ((HRESULT)0x887700be)
#define NET_PENDING          ((HRESULT)0x8000000a)
#define NET_TIMEOUT          ((HRESULT)0x887700f0)

enum eNetTransportSystemMessage
{
   kNetSystemCreatePlayer = 0x0003,
   kNetSystemDestroyPlayer = 0x0005,
   kNetSystemSessionLost = 0x0031,
   kNetSystemHostChanged = 0x0101,
   kNetSystemSendComplete = 0x010d
};

typedef struct sNetTransportName
{
   uint32 size;
   uint32 flags;
   char *short_name;
   char *long_name;
} sNetTransportName;

typedef struct sNetTransportCaps
{
   uint32 size;
   uint32 flags;
   uint32 max_buffer_size;
   uint32 max_queue_size;
   uint32 max_players;
   uint32 hundred_baud;
   uint32 latency;
   uint32 max_local_players;
   uint32 header_length;
   uint32 timeout;
} sNetTransportCaps;

typedef struct sNetTransportSystemHeader
{
   uint32 type;
} sNetTransportSystemHeader;

typedef struct sNetTransportCreatePlayer
{
   uint32 type;
   uint32 player_type;
   tNetTransportPlayerID player;
   uint32 current_players;
   void *data;
   uint32 data_size;
   sNetTransportName name;
   tNetTransportPlayerID parent;
   uint32 flags;
} sNetTransportCreatePlayer;

typedef struct sNetTransportDestroyPlayer
{
   uint32 type;
   uint32 player_type;
   tNetTransportPlayerID player;
   void *local_data;
   uint32 local_data_size;
   void *remote_data;
   uint32 remote_data_size;
   sNetTransportName name;
   tNetTransportPlayerID parent;
   uint32 flags;
} sNetTransportDestroyPlayer;

typedef struct sNetTransportSendComplete
{
   uint32 type;
   tNetTransportPlayerID from;
   tNetTransportPlayerID to;
   uint32 flags;
   uint32 priority;
   uint32 timeout;
   void *context;
   uint32 message_id;
   HRESULT result;
   uint32 send_time;
} sNetTransportSendComplete;

typedef BOOL (*tNetTransportPlayerCallback)(
   tNetTransportPlayerID player, uint32 type,
   const sNetTransportName *name, uint32 flags, void *context);

#undef INTERFACE
#define INTERFACE INet

DECLARE_INTERFACE_(INet, IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   STDMETHOD(Close)(THIS) PURE;
   STDMETHOD(DestroyPlayer)(THIS_ tNetTransportPlayerID player) PURE;
   STDMETHOD(EnumPlayers)(THIS_ const GUID *instance,
                          tNetTransportPlayerCallback callback,
                          void *context, uint32 flags) PURE;
   STDMETHOD(GetCaps)(THIS_ sNetTransportCaps *caps, uint32 flags) PURE;
   STDMETHOD(GetPlayerData)(THIS_ tNetTransportPlayerID player, void *data,
                            uint32 *size, uint32 flags) PURE;
   STDMETHOD(Receive)(THIS_ tNetTransportPlayerID *from,
                      tNetTransportPlayerID *to, uint32 flags,
                      void *data, uint32 *size) PURE;
   STDMETHOD(SendEx)(THIS_ tNetTransportPlayerID from,
                     tNetTransportPlayerID to, uint32 flags,
                     const void *data, uint32 size, uint32 priority,
                     uint32 timeout, void *context,
                     uint32 *message_id) PURE;
   STDMETHOD(SetPlayerData)(THIS_ tNetTransportPlayerID player,
                            const void *data, uint32 size,
                            uint32 flags) PURE;

   STDMETHOD_(BOOL, Host)(THIS_ const char *media,
                          const char *session_name) PURE;
   STDMETHOD_(BOOL, Join)(THIS_ const char *media,
                          const char *session_name,
                          const char *address) PURE;
   STDMETHOD_(tNetTransportPlayerID, SimpleCreatePlayer)(THIS_ const char *name) PURE;
   STDMETHOD(GetPlayerAddress)(THIS_ tNetTransportPlayerID player,
                               char *buffer, int buffer_size) PURE;
   STDMETHOD_(void, ResetPlayerData)(THIS_ tNetTransportPlayerID player,
                                     void *user_data) PURE;
#ifdef NET_ALLOW_SIMULATION
   STDMETHOD_(void, UseInternetSimulation)(THIS_ BOOL value) PURE;
   STDMETHOD_(void, SetInternetParameters)(THIS_ double min_latency,
                                           double average_latency,
                                           double max_latency,
                                           ulong loss_percent) PURE;
#endif
   STDMETHOD_(const char *, ErrorString)(THIS_ HRESULT result) PURE;
};

EXTERN void NetCreate(void);

#endif
