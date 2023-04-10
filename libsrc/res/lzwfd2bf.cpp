//        LZWFD2BF.C        Hard-coded routine to expand fd to buff
//        This module by Greg Travis and Rex Bradford
//

#if 0

/*
* $Header: x:/prj/tech/libsrc/res/RCS/lzwfd2bf.cpp 1.4 1997/01/06 11:42:37 TOML Exp $
* $Log: lzwfd2bf.cpp $
 * Revision 1.4  1997/01/06  11:42:37  TOML
 * phase 1 optimization
 * 
 * Revision 1.3  1996/09/14  16:10:30   TOML
 * Prepared for revision
 *
 * Revision 1.2  1994/09/21  09:37:25   rex
 * Put #ifdef around everything
 *
 * Revision 1.1  1994/09/21  09:34:14   rex
 * Initial revision
 *
*/

//  ------------------------------------------------------------
//        HEADER SECTION
//  ------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include <lg.h>
#ifndef __LZW_H
#include <lzw.h>
#endif

#ifdef OPTIMIZED_LZW_EXPAND_FD2BUFF

//  Important constants

#define MAX_VALUE ((1 << LZW_BITS) - 1) // end-of-compress-data code
#define MAX_CODE (MAX_VALUE - 2)                        // maximum real code allows
#define FLUSH_CODE (MAX_VALUE - 1)            // code to lzw string table

//  Overall lzw buffer info

extern void *lzwBuffer;                                  // total buffer
extern bool lzwBufferMalloced;                       // buffer malloced?

//  Global tables used for compression & expansion

extern short *lzwCodeValue;                          // code value array
extern ushort *lzwPrefixCode;                            // prefix code array
extern uchar *lzwAppendChar;                             // appended chars array
extern uchar *lzwDecodeStack;                            // decoded string

extern uchar *lzwFdReadBuff;                             // buffer for file descriptor source

//  -----------------------------------------------------------
//        EXPANSION -> FD TO BUFF
//  -----------------------------------------------------------
//
//  INPUT MACROS

static int lzwFdSrc;
static int lzwReadBuffIndex;

typedef struct
{
    int lzwInputBitCount;
    ulong lzwInputBitBuffer;
    unsigned int next_code;                              // next available string code
    unsigned int new_code;                               // next code from source
    unsigned int old_code;                               // last code gotten from source
    unsigned int character;                              // current char for string stack
    uchar *string;                                           // used to output string in reverse order
    long outputSize;                                         // size of uncompressed data
    long destSkip;                                           // # bytes to skip over
    long destSize;                                           // destination size
} LzwE;

extern LzwE lzwe;                                            // current expand state
#define LzwFdSrcBegin(fd) { \
     lzwFdSrc = fd;     \
     lzwReadBuffIndex = LZW_FD_READ_BUFF_SIZE;  \
}

static unsigned int LzwInputCode()
{
    unsigned int return_value;
    uchar c;
    while (lzwe.lzwInputBitCount <= 24)
    {
        if (lzwReadBuffIndex == LZW_FD_READ_BUFF_SIZE)
        {
            read(lzwFdSrc, lzwFdReadBuff, LZW_FD_READ_BUFF_SIZE);
            lzwReadBuffIndex = 0;
        }
        c = lzwFdReadBuff[lzwReadBuffIndex++];
        lzwe.lzwInputBitBuffer |= ((ulong) c) << (24 - lzwe.lzwInputBitCount);
        lzwe.lzwInputBitCount += 8;
    }
    return_value = lzwe.lzwInputBitBuffer >> (32 - LZW_BITS);

    lzwe.lzwInputBitBuffer <<= LZW_BITS;
    lzwe.lzwInputBitCount -= LZW_BITS;

    return (return_value);
}
//  -----------------------------------------------------------
//
//  OUTPUT MACROS

static uchar *lzwBuffDestPtr;
#define LzwBuffDestBegin(destLoc) (lzwBuffDestPtr = (uchar *)(destLoc))
#define LzwBuffDestPut(byte) (*lzwBuffDestPtr++ = (byte))

//  ------------------------------------------------------------
//
//  LzwDecodeString() decodes a string from the string table,
//  storing it in a buffer.  The buffer can then be output in
//  reverse order by the expansion program.

static uchar *LzwDecodeString(uchar * buffer, unsigned int code)
{
#ifdef DBG_ON
    int i = 0;
#endif

    while (code > 255)
    {
        *buffer++ = lzwAppendChar[code];
        code = lzwPrefixCode[code];

#ifdef DBG_ON
        if (i++ >= 4094)
            Warning(("LzwDecodeString: Fatal error during code expansion\n"));
#endif
    }

    *buffer = code;
    return (buffer);
}
//  -----------------------------------------------------------
//
//  THE ACTUAL EXPANSION ROUTINE
//
//  LzwExpandFd2Buff() is a variant of LzwExpand(), hard-coded to
//  go from file descriptor to buffer.
//
//        fdSrc       = file descriptor of source
//        pdest       = ptr to dest buffer
//        destSkip    = # bytes of (uncompressed) data to skip
//        destSize    = # bytes of (uncompressed) data to store (if 0, everything)
//
//  Returns: # bytes in uncompressed output

long LzwExpandFd2Buff(int fdSrc, uchar * pdest, long destSkip, long destSize)
{
//  If not already initialized, do it

    if (lzwBuffer == NULL)
    {
        if (LzwMallocBuffer() < 0)
            return (0);
    }

//  Set up for expansion loop

    lzwe.lzwInputBitCount = 0;
    lzwe.lzwInputBitBuffer = 0;
    lzwe.next_code = 256;                                // next available char after regular 256 chars
    lzwe.outputSize = 0;
    lzwe.destSkip = destSkip;
    lzwe.destSize = destSize ? destSize : LZW_MAXSIZE;

//  Notify the control routines

    LzwFdSrcBegin(fdSrc);
    LzwBuffDestBegin(pdest);

//  Get first code & output it.

    lzwe.old_code = LzwInputCode();
    lzwe.character = lzwe.old_code;

    if (--lzwe.destSkip < 0)
    {
        LzwBuffDestPut(lzwe.old_code);
        lzwe.outputSize++;
    }

//  This is the expansion loop. It reads in codes from the source until
//  it sees the special end-of-data code.

    while ((lzwe.new_code = LzwInputCode()) != MAX_VALUE)
    {
//  If flush code, flush the string table & restart from top of loop.
//  Also heck for the special STRING+CHARACTER+STRING+CHARACTER+STRING,
//  which generates an undefined code.  Handle it by decoding the last code,
//  adding a single character to the end of the decode string.

        if (lzwe.new_code >= lzwe.next_code)
        {
            if (lzwe.new_code == FLUSH_CODE)
            {
                lzwe.next_code = 256;
                lzwe.old_code = LzwInputCode();
                lzwe.character = lzwe.old_code;
                if (--lzwe.destSkip < 0)
                {
                    if (lzwe.outputSize++ >= lzwe.destSize)
                        break;
                    LzwBuffDestPut(lzwe.old_code);
                }
                continue;
            }
            else
            {
                *lzwDecodeStack = lzwe.character;
                lzwe.string = LzwDecodeString(lzwDecodeStack + 1, lzwe.old_code);
            }
        }

//  Otherwise we do a straight decode of the new code.

        else
        {
            if (lzwe.new_code < 256)
            {
                lzwe.string = lzwDecodeStack;
                *lzwe.string = lzwe.new_code;
            }
            else
                lzwe.string = LzwDecodeString(lzwDecodeStack, lzwe.new_code);
        }

//  Output the decode string to the destination, in reverse order.

        lzwe.character = *lzwe.string;
        while (lzwe.string >= lzwDecodeStack)
        {
            if (--lzwe.destSkip < 0)
            {
                if (lzwe.outputSize++ >= lzwe.destSize)
                    goto DONE_EXPAND;
                LzwBuffDestPut(*lzwe.string);
            }
            --lzwe.string;
        }

//  If possible, add a new code to the string table.

        if (lzwe.next_code <= MAX_CODE)
        {
            lzwPrefixCode[lzwe.next_code] = lzwe.old_code;
            lzwAppendChar[lzwe.next_code] = lzwe.character;
            lzwe.next_code++;
        }
        lzwe.old_code = lzwe.new_code;
    }

//  When break out of expansion loop, shut down source & dest & return size.

DONE_EXPAND:

    return (lzwe.outputSize);
}
#endif
#endif
