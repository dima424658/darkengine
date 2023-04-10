//    LZW.C       New improved super-duper LZW compressor & decompressor
//    This module by Greg Travis and Rex Bradford
//
//    This module implements LZW-based compression and decompression.
//    It has flexible control over the source and destination of the
//    data it uses.  Data is read from a "source" and written to a
//    "destination".    In the case of compression, the source is uncompressed
//    and the destination is compressed; for expansion the reverse is true.
//    Sources and destinations deal in byte values, even though the LZW
//    routine works in 14-bit compression codes.
//
//    Sources may be one of the following standard types:
//
//    BUFF        A memory block
//    FD              A file descriptor (fd = open()), already positioned with seek()
//    FP              A file ptr (fp = fopen()), already positioned with lseek()
//    USER        A user-supplied function
//
//    Destinations may be any of the above 4 types, plus additionally:
//
//    NULL        The bit-bucket.  Used to determine the size of destination
//                    data without putting it anywhere.
//
//    The LZW module consists of these public routines:
//
//    LzwInit() - Just sets LzwTerm() to be called on exit.
//
//    LzwTerm() - Just calls LzwFreeBuffer(), to free lzw buffer if it
//                          has been malloc'ed.
//
//    LzwSetBuffer() - Sets buffer for lzw compression & expansion routines
//                               to use.  The buffer must be at least LZW_BUFF_SIZE
//                               in size, which for 14-bit lzw is about 91K.
//
//    LzwMallocBuffer() - Allocates buffer for lzw compression & expansion
//                               routines to use.  This routine will be called auto-
//                               matically the first time LzwCompress() or LzwExpand()
//                               is used if a buffer has not been set or allocated.
//
//    LzwFreeBuffer() - Frees current buffer if allocated.
//
//    LzwCompress() - Compresses data, reading from an uncompressed source
//                          and writing compressed bytes to a destination.  Returns
//                          the size of the compressed data.  A maximum destination
//                          size may be specified, in which case a destination which
//                          is about to exceed this will be aborted, returning -1.
//
//    LzwExpand() - Expands data, reading from a compressed source and
//                          writing decompressed bytes to a destination.  Returns the
//                          size of the decompressed data.  Parameters may be used
//                          to capture a subsection of the uncompressed stream (by
//                          skipping the first n1 destination bytes and then taking
//                          the next n2).
//
//    Lzw.h supplies a large set of macros of the form:
//
//          LzwCompressSrc2Dest(...)  and  LzweExpandSrc2Dest(...)
//
//    which implement all combinations of source and destination types,
//    such as buffer->buffer, fd->buffer, buffer->null, fp->user, etc.
//
//    User types are handy when there is a need to transform the data
//    on its way to or from compression (to enhance the compression, or
//    just to massage the data into a usable form).  For example, a map
//    may want to transform elevations to delta format on the way to
//    and from compression in order to enhance the compression.
//
//    User sources supply two functions of the form:
//
//    void f_SrcCtrl(long srcLoc, LzwCtrl ctrl);
//    uchar f_SrcGet();
//
//    The control function is used to set up and tear down the Get()
//    function, which is used to supply the next byte of data.  Before
//    any compression or decompression begins, the SrcCtrl() function
//    is called with the srcLoc argument (supplied at the call to
//    LzwCompress or LzwExpand, its meaning is user-defined), and the
//    ctrl argument set to BEGIN.  After all compression and decompression
//    is done, cleanup is invoked by calling SrcCtrl() with ctrl equal
//    to END.  Between BEGIN and END, the SrcGet() function is called
//    repeatedly to get the next byte from the user input stream.
//
//    User destinations work similarly.  Again, two functions:
//
//    void f_DestCtrl(long destLoc, LzwCtrl ctrl);
//    void f_DestPut(uchar byte);
//
//    The control function is called with BEGIN and END just like the
//    source function.  The DestPut() function is called repeatedly to
//    put the next byte to the user output stream.
//
//    Note that user sources can be used for both compression (source of
//    uncompressed bytes) and expansion (source of compressed bytes).
//    Similarly, user destinations can be used for both compression
//    (destination of compressed bytes) and expansion (destination of
//    uncompressed bytes).  This is true of standard sources and
//    destinations as well, of course.

/*
* $Header: x:/prj/tech/libsrc/res/RCS/lzw.cpp 1.16 1997/01/07 11:12:52 TOML Exp $
* $log$
*/

//  ------------------------------------------------------------
//    HEADER SECTION
//  ------------------------------------------------------------

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include <lg.h>
#ifndef __LZW_H
#include <lzw.h>
#endif

#include <thrdtool.h>

// ----------------------------------------------------------
// Profiling tool
//
#ifdef TIME_LZW
#include <mprintf.h>

struct sLZWTimer
{
    sLZWTimer()
    {
        sumTime = 0;
        sumBytes = 0;
        totalTime = timeGetTime();
    }

    ~sLZWTimer()
    {
        double totalTimeInSecs = (double) (timeGetTime() - totalTime)  / 1000.0;
        double sumTimeInSecs = (double) sumTime / 1000.0;
        double sumMBytes = (double) sumBytes / (1024.0 * 1024.0);
        char buf[256];

        sprintf(buf, "Pct time in LZW was %f\n", sumTimeInSecs / totalTimeInSecs);
        mprintf(buf);

        sprintf(buf, "MB expanded per second were %f\n", sumMBytes / sumTimeInSecs);
        mprintf(buf);
    }

    void Start()
    {
        startThisTime = timeGetTime();
    }

    void Stop(DWORD bytes)
    {
        DWORD t = sumTime;
        sumTime += timeGetTime() - startThisTime;
        //Assert_(sumTime >= t);

        DWORD s = sumBytes;
        sumBytes += bytes;
        //Assert_(sumBytes >= s);
    }

    DWORD sumTime;
    DWORD sumBytes;
    DWORD totalTime;
    DWORD startThisTime;
};

sLZWTimer g_LzwExpandTimer;

#define LZWTimerStart()     g_LzwExpandTimer.Start()
#define LZWTimerStop(bytes) g_LzwExpandTimer.Stop(bytes);
#else
#define LZWTimerStart()
#define LZWTimerStop(bytes)
#endif

// ----------------------------------------------------------
// The Watcom C++ parser is verbose in warning of possible integral
// truncation, even in cases where for the given native word size
// there is no problem (i.e., assigning longs to  ints).  Here, we
// quiet the warnings, although it wouldn't be bad for someone to
// evaluate them. (toml 09-14-96)
//
#pragma warning 389 9


//  Important constants

#define MAX_VALUE ((1 << LZW_BITS) - 1) // end-of-compress-data code
#define MAX_CODE (MAX_VALUE - 2)                        // maximum real code allows
#define FLUSH_CODE (MAX_VALUE - 1)            // code to lzw string table
#define HASHING_SHIFT (LZW_BITS-8)            // # bits to shift when hashing
#define FLUSH_PAUSE 1000                                // wait on full table before flush

// we use read & write buffer when doing short-circuit file to memory (toml 01-06-97)
#define LZW_FD_TO_MEM_READ_BUFF_SIZE    (LZW_FD_READ_BUFF_SIZE + LZW_FD_WRITE_BUFF_SIZE)

// LZW thread lock

static cThreadLock g_LzwThreadLock;

//  Overall lzw buffer info

void *lzwBuffer;                                             // total buffer
bool lzwBufferMalloced;                                  // buffer malloced?

//  Global tables used for compression & expansion

tLzwCodeValue *lzwCodeValue;                                         // code value array
tLzwPrefixCode *lzwPrefixCode;                                   // prefix code array
tLzwAppendChar *lzwAppendChar;                                    // appended chars array
uchar *lzwDecodeStack;                                   // decoded string

uchar *lzwFdReadBuff;                                    // buffer for file descriptor source
uchar *lzwFdWriteBuff;                                   // buffer for file descriptor dest

//  Prototypes of internal routines

int LzwFindMatch(int hash_prefix, unsigned int hash_character);
uchar *LzwDecodeString(uchar * buffer, unsigned int code);
//  --------------------------------------------------------
//    INITIALIZATION AND TERMINATION
//  --------------------------------------------------------
//
//  LzwInit()   needs to be called once before any of the compression
//   routines are used.

void LzwInit(void)
{
    AtExit(LzwTerm);
}
//  ------------------------------------------------------------
//
// LzwTerm() needs to be called once when the lzw compression
//   routines are no longer needed.

void LzwTerm(void)
{
    LzwFreeBuffer();
}
//  ------------------------------------------------------------
//    BUFFER SETTING
//  --------------------------------------------------------
//
//  LzwSetBuffer() inits and sets buffer to use.
//
//  Returns: 0 if ok, -1 if buffer not ok

void LzwSetBufferPointers(void *buff)
{
    lzwBuffer = buff;
    lzwDecodeStack = (uchar *) lzwBuffer;
    lzwFdReadBuff = ((uchar *) lzwDecodeStack) + LZW_DECODE_STACK_SIZE;
    lzwFdWriteBuff = ((uchar *) lzwFdReadBuff) + LZW_FD_READ_BUFF_SIZE;
    lzwCodeValue = (tLzwCodeValue *) (((uchar *) lzwFdWriteBuff) + LZW_FD_WRITE_BUFF_SIZE);
    lzwPrefixCode = (tLzwPrefixCode *) (((uchar *) lzwCodeValue) + (LZW_TABLE_BUFF_SIZE * sizeof(tLzwCodeValue)));
    lzwAppendChar = ((tLzwAppendChar *) lzwPrefixCode) + (LZW_TABLE_BUFF_SIZE * sizeof(tLzwPrefixCode));
}

int LzwSetBuffer(void *buff, long buffSize)
{
    cAutoLock lock(g_LzwThreadLock);
//  Check buffer size

    if (buffSize < LZW_BUFF_SIZE)
    {
        Warning(("LzwSetBuffer: buffer too small!\n"));
        return (-1);
    }

//  De-allocate current buffer if malloced

    LzwTerm();

//  Set buffer pointers
    LzwSetBufferPointers(buff);
    lzwBufferMalloced = FALSE;

    return (0);
}
//  ------------------------------------------------------------
//
//  LzwMallocBuffer() allocates buffer with Malloc.
//
//  Returns: 0 if success, -1 if error.

int LzwMallocBuffer()
{
    cAutoLock lock(g_LzwThreadLock);
    void *buff;
    if ((lzwBuffer == NULL) || (!lzwBufferMalloced))
    {
        buff = Malloc(LZW_BUFF_SIZE);
        if (buff == NULL)
        {
            Warning(("LzwMallocBuffer: failed to allocate buffers\n"));
            return (-1);
        }
        else
        {
            LzwSetBuffer(buff, LZW_BUFF_SIZE);
            lzwBufferMalloced = TRUE;
        }
    }
    return (0);
}
//  ------------------------------------------------------------
//
//  LzwFreeBuffer() frees buffer.

void LzwFreeBuffer()
{
    cAutoLock lock(g_LzwThreadLock);
    if (lzwBufferMalloced)
    {
        Free(lzwBuffer);
        lzwBuffer = NULL;
        lzwBufferMalloced = FALSE;
    }
}
//  ------------------------------------------------------------
//    COMPRESSION
//  ------------------------------------------------------------
//
//  LzwCompress() does lzw compression.  It reads uncompressed bytes
//  from an input source and outputs compressed bytes to an output
//  destination.    It returns the number of bytes the compressed data
//  took up, or -1 if the compressed data size exceeds the allowed space.
//
//    f_ScrCtrl     = routine to call to control source data stream
//    f_SrcGet      = routine to call to get next input data byte
//    srcLoc            = source data "location", actual type undefined
//    srcSize       = size of source (input) data
//    f_DestCtrl    = routine to call to control destination data stream
//    f_DestPut     = routine to call to put next output data byte
//    destLoc       = dest data "location", actual type undefined
//    destSizeMax   = maximum allowed size of output data
//
//  Returns: actual output compressed size, or -1 if exceeded outputSizeMax
//    (in which case compression has been aborted)

//  This macro is used to accumulate output codes into a bit buffer
//  and call the destination put routine whenever more than 8 bits
//  are available.  If the output data size ever exceeds the alloted
//  size, the source and destination are shut down and -1 is returned.

typedef struct
{
    unsigned int next_code;                              // next available string code
    unsigned int character;                              // current character read from source
    unsigned int string_code;                            // current string compress code
    unsigned int index;                                  // index into string table
    long lzwInputCharCount;                              // input character count
    long lzwOutputSize;                                  // current size of output
    int lzwOutputBitCount;                               // current bit location in output
    ulong lzwOutputBitBuffer;                            // 32-bit buffer holding output bits
} LzwC;

LzwC lzwc;                                                   // current compress state

#define LzwOutputCode(code) { \
     lzwc.lzwOutputBitBuffer |= ((ulong) code) << (32-LZW_BITS-lzwc.lzwOutputBitCount); \
     lzwc.lzwOutputBitCount += LZW_BITS; \
     while (lzwc.lzwOutputBitCount >= 8) \
          { \
          (*f_DestPut)(lzwc.lzwOutputBitBuffer >> 24); \
          if (++lzwc.lzwOutputSize > destSizeMax) \
                { \
                (*f_SrcCtrl)(srcLoc, END); \
                (*f_DestCtrl)(destLoc, END); \
                return -1L; \
                } \
          lzwc.lzwOutputBitBuffer <<= 8; \
          lzwc.lzwOutputBitCount -= 8; \
          } \
}

long LzwCompress( tLzwCompressCtrlFunc f_SrcCtrl,// func to control source
                        tLzwCompressSrcGetFunc f_SrcGet,          // func to get bytes from source
                        long srcLoc,                         // source "location" (ptr, FILE *, etc.)
                        long srcSize,                        // size of source in bytes
                        void (*f_DestCtrl) (long destLoc, LzwCtrl ctrl),        // func to control dest
                        void (*f_DestPut) (uchar b),     // func to put bytes to dest
                        long destLoc,                        // dest "location" (ptr, FILE *, etc.)
                        long destSizeMax                     // max size of dest (or LZW_MAXSIZE)
                     )
{

    cAutoLock lock(g_LzwThreadLock);
//  If not already initialized, do it

    if (lzwBuffer == NULL)
    {
        if (LzwMallocBuffer() < 0)
            return (0);
    }

//  Set up for compress loop

    lzwc.next_code = 256;                                // skip over real 256 char values
    memset(lzwCodeValue, -1, sizeof(tLzwCodeValue) * LZW_TABLE_SIZE);

    lzwc.lzwOutputSize = 0;
    lzwc.lzwOutputBitCount = 0;
    lzwc.lzwOutputBitBuffer = 0;

    (*f_SrcCtrl) (srcLoc, BEGIN);
    (*f_DestCtrl) (destLoc, BEGIN);

    lzwc.string_code = (*f_SrcGet) ();
    lzwc.lzwInputCharCount = 1;

// This is the main loop where it all happens.  This loop runs until all of
// the input has been exhausted.  Note that it stops adding codes to the
// table after all of the possible codes have been defined.

    while (TRUE)
    {

//  Get next input char, if read all data then exit loop

        lzwc.character = (*f_SrcGet) ();
        if (lzwc.lzwInputCharCount++ >= srcSize)
            break;

//  See if string is in string table.   If it is, get the code value.

        lzwc.index = LzwFindMatch(lzwc.string_code, lzwc.character);
        if (lzwCodeValue[lzwc.index] != -1)
            lzwc.string_code = lzwCodeValue[lzwc.index];

//  Else if string not in string table, try to add it.

        else
        {
            if (lzwc.next_code <= MAX_CODE)
            {
                lzwCodeValue[lzwc.index] = lzwc.next_code++;
                lzwPrefixCode[lzwc.index] = lzwc.string_code;
                lzwAppendChar[lzwc.index] = lzwc.character;
                LzwOutputCode(lzwc.string_code);
                lzwc.string_code = lzwc.character;
            }

//  Else if table is full and has been for a while, flush it, and drain
//  the code value table too.

            else if (lzwc.next_code > MAX_CODE + FLUSH_PAUSE)
            {
                LzwOutputCode(lzwc.string_code);
                LzwOutputCode(FLUSH_CODE);
                memset(lzwCodeValue, -1, sizeof(tLzwCodeValue) * LZW_TABLE_SIZE);
                lzwc.string_code = lzwc.character;
                lzwc.next_code = 256;
            }

//  Else if can't add but table not full, just output the code.

            else
            {
                lzwc.next_code++;
                LzwOutputCode(lzwc.string_code);
                lzwc.string_code = lzwc.character;
            }
        }
    }

//  Done with processing loop, output current code, end-of-data code,
//  and a final 0 to flush the buffer.

    LzwOutputCode(lzwc.string_code);
    LzwOutputCode(MAX_VALUE);
    LzwOutputCode(0);

//  Shut down source and destination and return size of output

    (*f_SrcCtrl) (srcLoc, END);
    (*f_DestCtrl) (destLoc, END);

    return (lzwc.lzwOutputSize);
}
//  -----------------------------------------------------------
//    EXPANSION
//  -----------------------------------------------------------
//
//  LzwExpand() does lzw expansion.  It reads compressed bytes
//  from an input source and outputs uncompressed bytes to an output
//  destination.    It returns the number of bytes the uncompressed data
//  took up.
//
//    f_ScrCtrl     = routine to call to control source data stream
//    f_SrcGet      = routine to call to get next input data byte
//    srcLoc            = source data "location", actual type undefined
//    f_DestCtrl    = routine to call to control destination data stream
//    f_DestPut     = routine to call to put next output data byte
//    destLoc       = dest data "location", actual type undefined
//    destSkip      = # bytes of output to skip over before storing
//    destSize      = # bytes of output to store (if 0, everything)
//
//  Returns: # bytes in uncompressed output

// need to change LZW_PARTIAL_STATE_SIZE if this changes in size!
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
    uchar *last_buffer;                                  // for partial expand, covers old buffer state
    uchar *curr_buffer;                                  // for partial, the buffer we are using for globals
    void *in_state;                                      // for partial, pointer for the input installable func to use to save state
    void *out_state;                                         // for partial, pointer for the output installable func to use to save state
} LzwE;

LzwE lzwe;                                                   // current expand state

///////////////////////////////////////////////////////////////////////////////

static unsigned int LzwInputCodeGeneric(tLzwCompressSrcGetFunc f_SrcGet, LzwE * plzwe)
{
    unsigned int return_value;
    while (plzwe->lzwInputBitCount <= 24)
    {
        plzwe->lzwInputBitBuffer |= ((ulong) (*f_SrcGet) ()) << (24 - plzwe->lzwInputBitCount);
        plzwe->lzwInputBitCount += 8;
    }
    return_value = plzwe->lzwInputBitBuffer >> (32 - LZW_BITS);

    plzwe->lzwInputBitBuffer <<= LZW_BITS;
    plzwe->lzwInputBitCount -= LZW_BITS;

    return (return_value);
}

///////////////////////////////////////

static long LzwExpandGeneric(tLzwCompressCtrlFunc f_SrcCtrl,   // func to control source
                             tLzwCompressSrcGetFunc f_SrcGet,  // func to get bytes from source
                             long srcLoc,                      // source "location" (ptr, FILE *, etc.)
                             tLzwCompressCtrlFunc f_DestCtrl,  // func to control dest
                             tLzwCompressDestPutFunc f_DestPut,// func to put bytes to dest
                             long destLoc                     // dest "location" (ptr, FILE *, etc.)
                            )
{
//  Notify the control routines

    (*f_SrcCtrl) (srcLoc, BEGIN);
    (*f_DestCtrl) (destLoc, BEGIN);

//  Get first code & output it.

    lzwe.old_code = LzwInputCodeGeneric(f_SrcGet, &lzwe);
    lzwe.character = lzwe.old_code;

    if (--lzwe.destSkip < 0)
    {
        (*f_DestPut) (lzwe.old_code);
        lzwe.outputSize++;
    }

//  This is the expansion loop. It reads in codes from the source until
//  it sees the special end-of-data code.

    while ((lzwe.new_code = LzwInputCodeGeneric(f_SrcGet, &lzwe)) != MAX_VALUE)
    {

//  If flush code, flush the string table & restart from top of loop

        if (lzwe.new_code == FLUSH_CODE)
        {
            lzwe.next_code = 256;
            lzwe.old_code = LzwInputCodeGeneric(f_SrcGet, &lzwe);
            lzwe.character = lzwe.old_code;
            if (--lzwe.destSkip < 0)
            {
                if (lzwe.outputSize++ >= lzwe.destSize)
                    break;
                (*f_DestPut) (lzwe.old_code);
            }
            continue;
        }

//  Check for the special STRING+CHARACTER+STRING+CHARACTER+STRING, which
//  generates an undefined code.  Handle it by decoding the last code,
//  adding a single character to the end of the decode string.

        if (lzwe.new_code >= lzwe.next_code)
        {
            *lzwDecodeStack = lzwe.character;
            lzwe.string = LzwDecodeString(lzwDecodeStack + 1, lzwe.old_code);
        }

//  Otherwise we do a straight decode of the new code.

        else
        {
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
                (*f_DestPut) (*lzwe.string);
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
    (*f_SrcCtrl) (srcLoc, END);
    (*f_DestCtrl) (destLoc, END);

    return (lzwe.outputSize);
}

///////////////////////////////////////////////////////////////////////////////

static uchar *lzwBuffDestPtr;
static int lzwFdSrc;
static int lzwReadBuffIndex;

static unsigned int LzwInputCodeFileToMemory()
{
   unsigned int return_value;
   register ulong c;
   register ulong temp;
   while (lzwe.lzwInputBitCount <= 24)
   {
      if (lzwReadBuffIndex != LZW_FD_TO_MEM_READ_BUFF_SIZE)
      {
          c = lzwFdReadBuff[lzwReadBuffIndex];
          temp = 24 - lzwe.lzwInputBitCount;
          lzwReadBuffIndex++;
          lzwe.lzwInputBitBuffer |= (c << temp);
          lzwe.lzwInputBitCount += 8;
      }
      else
      {
         //LZWTimerStop(0);
         read(lzwFdSrc, lzwFdReadBuff, LZW_FD_TO_MEM_READ_BUFF_SIZE);
         //LZWTimerStart();
         lzwReadBuffIndex = 0;
      }
   }
   return_value = lzwe.lzwInputBitBuffer >> (32 - LZW_BITS);

   lzwe.lzwInputBitCount -= LZW_BITS;
   lzwe.lzwInputBitBuffer <<= LZW_BITS;

   return (return_value);
}

///////////////////////////////////////

static long LzwExpandFileToMemory(long srcLoc, long destLoc)
{

    lzwFdSrc = (int) srcLoc;
    lzwReadBuffIndex = LZW_FD_TO_MEM_READ_BUFF_SIZE;
    lzwBuffDestPtr = (uchar *) destLoc;

//  Get first code & output it.
    lzwe.old_code = LzwInputCodeFileToMemory();

    lzwe.character = lzwe.old_code;

    if (--lzwe.destSkip < 0)
    {
        *lzwBuffDestPtr++ = lzwe.old_code;
        lzwe.outputSize++;
    }

//  This is the expansion loop. It reads in codes from the source until
//  it sees the special end-of-data code.

    while ((lzwe.new_code = LzwInputCodeFileToMemory()) != MAX_VALUE)
    {
//  If flush code, flush the string table & restart from top of loop.
//  Also check for the special STRING+CHARACTER+STRING+CHARACTER+STRING,
//  which generates an undefined code.  Handle it by decoding the last code,
//  adding a single character to the end of the decode string.

        if (lzwe.new_code >= lzwe.next_code)
        {
            if (lzwe.new_code == FLUSH_CODE)
            {
                lzwe.next_code = 256;
                lzwe.old_code = LzwInputCodeFileToMemory();
                lzwe.character = lzwe.old_code;
                if (--lzwe.destSkip < 0)
                {
                    if (lzwe.outputSize++ >= lzwe.destSize)
                        break;
                    *lzwBuffDestPtr++ = lzwe.old_code;
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
                *lzwBuffDestPtr++ = *lzwe.string;
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

//  When break out of expansion loop, return size.

DONE_EXPAND:

    return (lzwe.outputSize);
}

///////////////////////////////////////

long LzwExpand(  tLzwCompressCtrlFunc f_SrcCtrl,   // func to control source
                 tLzwCompressSrcGetFunc f_SrcGet,  // func to get bytes from source
                 long srcLoc,                      // source "location" (ptr, FILE *, etc.)
                 tLzwCompressCtrlFunc f_DestCtrl,  // func to control dest
                 tLzwCompressDestPutFunc f_DestPut,// func to put bytes to dest
                 long destLoc,                     // dest "location" (ptr, FILE *, etc.)
                 long destSkip,                    // # dest bytes to skip over (or 0)
                 long destSize                     // # dest bytes to capture (if 0, all)
               )
{
    cAutoLock lock(g_LzwThreadLock);
    long retVal;
    LZWTimerStart();

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

    if (f_SrcCtrl == LzwFdSrcCtrl     && f_SrcGet == LzwFdSrcGet &&
        f_DestCtrl == LzwBuffDestCtrl && f_DestPut == LzwBuffDestPut)
    {
        retVal = LzwExpandFileToMemory(srcLoc, destLoc);
    }
    else
    {
        retVal = LzwExpandGeneric(f_SrcCtrl, f_SrcGet, srcLoc,
                             f_DestCtrl, f_DestPut, destLoc);
    }
    LZWTimerStop(lzwe.outputSize)

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////

// Okay, clearly there is a lot of code overlay between this and LzwExpand but for
// performance reasons in LzwExpand I didn't want to attempt to make them wholly
// integrated -- Xemu 5/1/96
#pragma off(unreferenced)
long LzwExpandPartial( tLzwCompressCtrlFunc f_SrcCtrl,  // func to control source
                              tLzwCompressSrcGetFunc f_SrcGet, // func to get bytes from source
                              long srcLoc,                   // source "location" (ptr, FILE *, etc.)
                              tLzwCompressCtrlFunc f_DestCtrl, // func to control dest
                              tLzwCompressDestPutFunc f_DestPut,         // func to put bytes to dest
                              long destLoc,              // dest "location" (ptr, FILE *, etc.)
                              long destSkip,                 // # dest bytes to skip over (or 0)
                              long destSize,                 // # dest bytes to capture (if 0, all)
                              void *state,                   // internal state structure
                              long cycles                    // how many iterations of actual processing
                            )
{
    cAutoLock lock(g_LzwThreadLock);
    LzwE *plzwe;
    int count = 0;
    plzwe = (LzwE *) state;
    LzwSetBufferPointers(plzwe->curr_buffer);

    // notify IO installables
    (*f_SrcCtrl) ((long) plzwe, RESUME);
    (*f_DestCtrl) ((long) plzwe, RESUME);

//  This is the expansion loop. It reads in codes from the source until
//  it sees the special end-of-data code.

    plzwe->new_code = LzwInputCodeGeneric(f_SrcGet, plzwe);
    while ((plzwe->new_code != MAX_VALUE) && (count < cycles))
    {

//  If flush code, flush the string table & restart from top of loop

        if (plzwe->new_code == FLUSH_CODE)
        {
            plzwe->next_code = 256;
            plzwe->old_code = LzwInputCodeGeneric(f_SrcGet, plzwe);
            plzwe->character = plzwe->old_code;
            if (--plzwe->destSkip < 0)
            {
                if (plzwe->outputSize++ >= plzwe->destSize)
                    goto DONE_EXPAND;
//                    break;
                (*f_DestPut) (plzwe->old_code);
            }
            goto LOOP_BOTTOM;
        }

//  Check for the special STRING+CHARACTER+STRING+CHARACTER+STRING, which
//  generates an undefined code.  Handle it by decoding the last code,
//  adding a single character to the end of the decode string.

        if (plzwe->new_code >= plzwe->next_code)
        {
            *lzwDecodeStack = plzwe->character;
            plzwe->string = LzwDecodeString(lzwDecodeStack + 1, plzwe->old_code);
        }

//  Otherwise we do a straight decode of the new code.

        else
        {
            plzwe->string = LzwDecodeString(lzwDecodeStack, plzwe->new_code);
        }

//  Output the decode string to the destination, in reverse order.

        plzwe->character = *plzwe->string;
        while (plzwe->string >= lzwDecodeStack)
        {
            if (--plzwe->destSkip < 0)
            {
                if (plzwe->outputSize++ >= plzwe->destSize)
                    goto DONE_EXPAND;
                (*f_DestPut) (*plzwe->string);
            }
            --plzwe->string;
        }

//  If possible, add a new code to the string table.

        if (plzwe->next_code <= MAX_CODE)
        {
            lzwPrefixCode[plzwe->next_code] = plzwe->old_code;
            lzwAppendChar[plzwe->next_code] = plzwe->character;
            plzwe->next_code++;
        }
        plzwe->old_code = plzwe->new_code;
LOOP_BOTTOM:
        count++;
        if (count < cycles)
            plzwe->new_code = LzwInputCodeGeneric(f_SrcGet, plzwe);
    }

    if (plzwe->new_code != MAX_VALUE)                // then we are still iterating, cause otherwise this is false or
        // we jumped down to DONE_EXPAND
    {
        // notify IO installables so that we can restore again later
        (*f_SrcCtrl) ((long) plzwe, SUSPEND);
        (*f_DestCtrl) ((long) plzwe, SUSPEND);
        LzwSetBufferPointers(plzwe->last_buffer);
        return (0);
    }

//  When break out of expansion loop, shut down source & dest & return size.

DONE_EXPAND:

    (*f_SrcCtrl) (srcLoc, END);
    (*f_DestCtrl) (destLoc, END);

    LzwSetBufferPointers(plzwe->last_buffer);

    return (plzwe->outputSize);
}
#pragma on(unreferenced)

void LzwExpandPartialStart(
                                     tLzwCompressCtrlFunc f_SrcCtrl,         // func to control source
                                     tLzwCompressSrcGetFunc f_SrcGet,    // func to get bytes from source
                                     long srcLoc,            // source "location" (ptr, FILE *, etc.)
                                     tLzwCompressCtrlFunc f_DestCtrl,    // func to control dest
                                     tLzwCompressDestPutFunc f_DestPut,  // func to put bytes to dest
                                     long destLoc,           // dest "location" (ptr, FILE *, etc.)
                                     long destSkip,      // # dest bytes to skip over (or 0)
                                     long destSize,      // # dest bytes to capture (if 0, all)
                                     void *state,            // internal state structure
                                     uchar * buffer      // buffer for storing "global" lzw state
                                  )
{
    cAutoLock lock(g_LzwThreadLock);
    LzwE *plzwe;
    plzwe = (LzwE *) state;

//  Check initialization

    if (buffer == NULL)
    {
        Warning(("LzwExpandPartialStart: cannot do partial expand with NULL buffer!\n"));
        return;
    }

    plzwe->last_buffer = (uchar *) lzwBuffer;
    plzwe->curr_buffer = buffer;
    LzwSetBufferPointers(buffer);

//  Set up for expansion loop

    plzwe->lzwInputBitCount = 0;
    plzwe->lzwInputBitBuffer = 0;
    plzwe->next_code = 256;                              // next available char after regular 256 chars
    plzwe->outputSize = 0;
    plzwe->destSkip = destSkip;
    plzwe->destSize = destSize ? destSize : LZW_MAXSIZE;

//  Notify the control routines

    (*f_SrcCtrl) (srcLoc, BEGIN);
    (*f_DestCtrl) (destLoc, BEGIN);

//  Get first code & output it.

    plzwe->old_code = LzwInputCodeGeneric(f_SrcGet, plzwe);
    plzwe->character = plzwe->old_code;

    if (--plzwe->destSkip < 0)
    {
        (*f_DestPut) (plzwe->old_code);
        plzwe->outputSize++;
    }

    (*f_SrcCtrl) ((long) plzwe, SUSPEND);
    (*f_DestCtrl) ((long) plzwe, SUSPEND);

    LzwSetBufferPointers(plzwe->last_buffer);
}
//  --------------------------------------------------------------
//    STANDARD INPUT SOURCES
//  --------------------------------------------------------------
//
//  LzwBuffSrcCtrl() and LzwBuffSrcGet() implement a memory buffer
//  source for lzw compression and expansion.

static uchar *lzwBuffSrcPtr;

void LzwBuffSrcCtrl(long srcLoc, LzwCtrl ctrl)
{
    LzwE *lzwep;
    if (ctrl == BEGIN)
        lzwBuffSrcPtr = (uchar *) srcLoc;
    else if (ctrl == SUSPEND)
    {
        lzwep = (LzwE *) srcLoc;
        lzwep->in_state = (void *) lzwBuffSrcPtr;
    }
    else if (ctrl == RESUME)
    {
        lzwep = (LzwE *) srcLoc;
        lzwBuffSrcPtr = (uchar *) lzwep->in_state;
    }
}

uchar LzwBuffSrcGet()
{
    return (*lzwBuffSrcPtr++);
}
//  ---------------------------------------------------------------
//
//  LzwFdSrcCtrl() and LzwFdSrcGet() implement a file-descriptor
//  source (fd = open()) for lzw compression and expansion.

void LzwFdSrcCtrl(long srcLoc, LzwCtrl ctrl)
{
    if (ctrl == BEGIN)
    {
        lzwFdSrc = (int) srcLoc;
        lzwReadBuffIndex = LZW_FD_READ_BUFF_SIZE;
    }
}

uchar LzwFdSrcGet()
{
    if (lzwReadBuffIndex == LZW_FD_READ_BUFF_SIZE)
    {
        //LZWTimerStop(0);
        read(lzwFdSrc, lzwFdReadBuff, LZW_FD_READ_BUFF_SIZE);
        //LZWTimerStart();
        lzwReadBuffIndex = 0;
    }
    return (lzwFdReadBuff[lzwReadBuffIndex++]);
}
//  ---------------------------------------------------------------
//
//  LzwFpSrcCtrl() and LzwFpSrcGet() implement a file-ptr source
//  (fp = fopen()) for lzw compression and expansion.

static FILE *lzwFpSrc;

void LzwFpSrcCtrl(long srcLoc, LzwCtrl ctrl)
{
    if (ctrl == BEGIN)
        lzwFpSrc = (FILE *) srcLoc;
}

uchar LzwFpSrcGet()
{
    return (fgetc(lzwFpSrc));
}
//  ---------------------------------------------------------------
//    STANDARD OUTPUT SOURCES
//  ---------------------------------------------------------------
//
//  LzwBuffDestCtrl() and LzwBuffDestPut() implement a memory
//  buffer destination for lzw compression and expansion.

void LzwBuffDestCtrl(long destLoc, LzwCtrl ctrl)
{
    LzwE *lzwep;
    if (ctrl == BEGIN)
        lzwBuffDestPtr = (uchar *) destLoc;
    else if (ctrl == SUSPEND)
    {
        lzwep = (LzwE *) destLoc;
        lzwep->out_state = (void *) lzwBuffDestPtr;
    }
    else if (ctrl == RESUME)
    {
        lzwep = (LzwE *) destLoc;
        lzwBuffDestPtr = (uchar *) lzwep->out_state;
    }

}

void LzwBuffDestPut(uchar b)
{
    *lzwBuffDestPtr++ = b;
}
//  ---------------------------------------------------------------
//
//  LzwFdDestCtrl() and LzwFdDestPut() implement a file-descriptor
//  destination (fd = open()) for lzw compression and expansion.

static int lzwFdDest;
static int lzwWriteBuffIndex;

void LzwFdDestCtrl(long destLoc, LzwCtrl ctrl)
{
    if (ctrl == BEGIN)
    {
        lzwFdDest = (int) destLoc;
        lzwWriteBuffIndex = 0;
    }
    else if (ctrl == END)
    {
        if (lzwWriteBuffIndex)
            write(lzwFdDest, lzwFdWriteBuff, lzwWriteBuffIndex);
    }
}

void LzwFdDestPut(uchar b)
{
    lzwFdWriteBuff[lzwWriteBuffIndex++] = b;
    if (lzwWriteBuffIndex == LZW_FD_WRITE_BUFF_SIZE)
    {
        write(lzwFdDest, lzwFdWriteBuff, LZW_FD_WRITE_BUFF_SIZE);
        lzwWriteBuffIndex = 0;
    }
}
//  ---------------------------------------------------------------
//
//  LzwFpDestCtrl() and LzwFpDestPut() implement a file-ptr destination
//  (fp = fopen()) for lzw compression and expansion.

static FILE *lzwFpDest;

void LzwFpDestCtrl(long destLoc, LzwCtrl ctrl)
{
    if (ctrl == BEGIN)
        lzwFpDest = (FILE *) destLoc;
}

void LzwFpDestPut(uchar b)
{
    fputc(b, lzwFpDest);
}
//  ---------------------------------------------------------------
//
//  LzwNullDestCtrl() and LzwNullDestPut() implement a bit-bucket
//  destination for lzw compression and expansion.  Used to size
//  results of compression or expansion.

#pragma off(unreferenced);

void LzwNullDestCtrl(long destLoc, LzwCtrl ctrl)
{
}

void LzwNullDestPut(uchar b)
{
}
#pragma on(unreferenced);

//  -----------------------------------------------------------
//    INTERNAL ROUTINES - COMPRESSION
//  -----------------------------------------------------------
//
//  LzwFindMatch() is the hashing routine.  It tries to find a match
//  for the prefix+char string in the string table.  If it finds it,
//  the index is returned.  If the string is not found, the first available
//  index in the string table is returned instead.
//
//    hash_prefix     = prefix to this code
//    hash_character = new character
//
//  Returns: string table index

int LzwFindMatch(int hash_prefix, unsigned int hash_character)
{
    int index;
    int offset;
    index = (hash_character << HASHING_SHIFT) ^ hash_prefix;
    if (index == 0)
        offset = 1;
    else
        offset = LZW_TABLE_SIZE - index;
    while (1)
    {
        if (lzwCodeValue[index] == -1)
            return (index);
        if ((lzwPrefixCode[index] == hash_prefix) &&
             (lzwAppendChar[index] == hash_character))
            return (index);
        index -= offset;
        if (index < 0)
            index += LZW_TABLE_SIZE;
    }
}
//  ------------------------------------------------------------
//    INTERNAL ROUTINES - EXPANSION
//  ------------------------------------------------------------
//
//  LzwDecodeString() decodes a string from the string table,
//  storing it in a buffer.  The buffer can then be output in
//  reverse order by the expansion program.

uchar *LzwDecodeString(uchar * buffer, unsigned int code)
{
#if defined(DBG_ON) || defined(ONEOPT)
    int i = 0;
#endif

    while (code > 255)
    {
        *buffer++ = lzwAppendChar[code];
        code = lzwPrefixCode[code];

#if defined(DBG_ON) || defined(ONEOPT)
        if (i++ >= 4094)
            CriticalMsg("LzwDecodeString: Fatal error during code expansion");
#endif
    }

    *buffer = code;
    return (buffer);
}
