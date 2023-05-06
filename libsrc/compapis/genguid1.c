///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/genguid1.c $
// $Author: TOML $
// $Date: 1997/02/24 23:47:58 $
// $Revision: 1.19 $
//
// GUIDs as declared in the header are simply extern declarations.
// This file actually defines the instances of the GUIDs
//
// define guids for these components.
//

#include <lg.h>
#include <comtools.h>
#ifdef _WIN32
#include <initguid.h>
#else
#include <nt\initguid.h>
#endif

#ifndef _WIN32
#define DEFINE_OLEGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

EXTERN const GUID __cdecl GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,       0x00000001L, 0, 0);
DEFINE_OLEGUID(IID_IMalloc,             0x00000002L, 0, 0);
DEFINE_OLEGUID(IID_IMarshal,            0x00000003L, 0, 0);

/* RPC related interfaces */
DEFINE_OLEGUID(IID_IRpcChannel,         0x00000004L, 0, 0);
DEFINE_OLEGUID(IID_IRpcStub,            0x00000005L, 0, 0);
DEFINE_OLEGUID(IID_IStubManager,        0x00000006L, 0, 0);
DEFINE_OLEGUID(IID_IRpcProxy,           0x00000007L, 0, 0);
DEFINE_OLEGUID(IID_IProxyManager,       0x00000008L, 0, 0);
DEFINE_OLEGUID(IID_IPSFactory,          0x00000009L, 0, 0);

/* storage related interfaces */
DEFINE_OLEGUID(IID_ILockBytes,          0x0000000aL, 0, 0);
DEFINE_OLEGUID(IID_IStorage,            0x0000000bL, 0, 0);
DEFINE_OLEGUID(IID_IStream,             0x0000000cL, 0, 0);
DEFINE_OLEGUID(IID_IEnumSTATSTG,        0x0000000dL, 0, 0);

/* moniker related interfaces */
DEFINE_OLEGUID(IID_IBindCtx,            0x0000000eL, 0, 0);
DEFINE_OLEGUID(IID_IMoniker,            0x0000000fL, 0, 0);
DEFINE_OLEGUID(IID_IRunningObjectTable, 0x00000010L, 0, 0);
DEFINE_OLEGUID(IID_IInternalMoniker,    0x00000011L, 0, 0);

/* storage related interfaces */
DEFINE_OLEGUID(IID_IRootStorage,        0x00000012L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved1,        0x00000013L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved2,        0x00000014L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved3,        0x00000015L, 0, 0);

/* concurrency releated interfaces */
DEFINE_OLEGUID(IID_IMessageFilter,      0x00000016L, 0, 0);

/* CLSID of standard marshaler */
DEFINE_OLEGUID(CLSID_StdMarshal,        0x00000017L, 0, 0);

/* interface on server for getting info for std marshaler */
DEFINE_OLEGUID(IID_IStdMarshalInfo,     0x00000018L, 0, 0);

/* interface to inform object of number of external connections */
DEFINE_OLEGUID(IID_IExternalConnection, 0x00000019L, 0, 0);

/* NOTE: LSB 0x1a through 0xff are reserved for future use */
#endif

// comtools:
#include <objcguid.h>

// compapis:
#include <appguid.h>
#include <dispguid.h>
#include <indvguid.h>
#include <gshlguid.h>
#include <wappguid.h>
#include <loopguid.h>
#include <moviguid.h>
#include <recguid.h>
#include <arqguid.h>
#include <thrmguid.h>
#include <cachguid.h>

// winshell:
#include <wnshguid.h>
#include <wdspguid.h>

// sdesc:
#include <sdesguid.h>

// namedres:
#include <resguid.h>

// dark:
#include <scriptguid.h>

// actmovie:
#include <actguid.h>

// net:
#include <netguid.h>

// input:
#include <inpguid.h>