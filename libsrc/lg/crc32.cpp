///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/crc32.cpp $
// $Author: TOML $
// $Date: 1997/01/29 17:52:47 $
// $Revision: 1.1 $
//

#include <crc32.h>

static ulong g_Crc32Table[256];

//
// Build auxiliary table for parallel byte-at-a-time CRC-32.
//
#define CRC32_POLY 0x04c11db7     // AUTODIN II, Ethernet, & FDDI

static void Crc32Init()
{
    int i, j;
    ulong c;

    for (i = 0; i < 256; ++i) {
        for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
        g_Crc32Table[i] = c;
    }
}

// Initialized first time "crc32()" is called. If you prefer, you can
// statically initialize it at compile time. [Another exercise.]
//

ulong Crc32(uchar *buf, int len)
{
    uchar *p;
    ulong  crc;

    if (!g_Crc32Table[1])                        // if not already done    
        Crc32Init();                             // build table 
    crc = 0xffffffff;                            // preload shift register, per CRC-32 spec 
    for (p = buf; len > 0; ++p, --len)
        crc = (crc << 8) ^ g_Crc32Table[(crc >> 24) ^ *p];
    return ~crc;                                 // transmit complement, per CRC-32 spec 
}
