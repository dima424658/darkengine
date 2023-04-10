///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/str.cpp $
// $Author: TOML $
// $Date: 1997/10/15 12:10:22 $
// $Revision: 1.8 $
//
// (c) Copyright 1993 - 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//
// This file originated as string.cpp, stringex.cpp, and winstr.cpp from
// the MFC 1.0 library. It was detached, modified, and renamed to provided
// a framework-independent string class.
//

#ifdef _WIN32
#include <windows.h>
#endif
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <lg.h>
#include <str.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////
#if defined (__WATCOMC__)
    #define snprintf    _bprintf
    #define vsnprintf   _vbprintf
#elif defined (__SC__) || defined (_MSC_VER)
    #define snprintf    _snprintf
    #define vsnprintf   _vsnprintf
#else
#error ("Need size-limited printf functions");
#endif

///////////////////////////////////////////////////////////////////////////////

// For an empty string, m_pchData will point here
char cAnsiStr::_ChNil = '\0';


//
// Return the next allocation increment
//
inline int NextAllocLen(int nCurLen, int nReqLen)
    {
    int nMinLen = (nCurLen < INT_MAX / 2) ? 2 * (uint) nCurLen : INT_MAX - 1;
// If we are allocating too little...
    if (nReqLen < nMinLen)
        // ...then lets allocate more
        return nMinLen + 1;

    return nReqLen + 1;
    }


//
// Construction/Destruction
//

//
// For the copy constructor, we clone the original
//
cAnsiStr::cAnsiStr(const cAnsiStr & Str)
    {
    Init();

    if (Str.GetLength())
        {
        AllocBuffer(Str.GetLength());
        memcpy(m_pchData, Str.operator const char *(), Str.GetLength());
        }
    }


cAnsiStr::cAnsiStr(const char *pStr)
    {
    Init();

    int nLen;
    if ((nLen = SafeStrlen(pStr)) != 0)
        {
        AllocBuffer(nLen);
        memcpy(m_pchData, pStr, nLen);
        }
    }


cAnsiStr::cAnsiStr(char ch)
    {
    Init();
    AllocBuffer(1);
    *m_pchData = ch;
    }


cAnsiStr::cAnsiStr(const char *pStr, int nLen)
    {
    Init();
    if (nLen != 0)
        {
        AllocBuffer(nLen);
        memcpy(m_pchData, pStr, nLen);
        }
    }


cAnsiStr::cAnsiStr(int nLen)
    {
    Init();
    if (nLen >= 1)
        {
        AllocBuffer(nLen);
        *m_pchData = 0;
        m_nDataLength = 0;
        }
    }


//
// Allocate a string buffer, allowing one extra character for '\0' termination
//
void cAnsiStr::AllocBuffer(int nReqLen)
    {
    if (!m_nAllocLength)
        {
        Init();
        if (nReqLen == 0)
            return;
        }

    // max size (enough room for 1 extra)
    AssertMsg(nReqLen >= 0 && nReqLen < INT_MAX - 1, "Invalid string allocation size");

    if (m_nAllocLength < nReqLen + 1)
        {
        const int nAllocLength = NextAllocLen(m_nAllocLength, nReqLen);
        if (m_nAllocLength)
            m_pchData = ReallocStr(m_pchData, nAllocLength);
        else
            m_pchData = AllocStr(nAllocLength);
        m_nAllocLength = nAllocLength - 1;
        }

    m_pchData[nReqLen] = '\0';
    m_nDataLength = nReqLen;
    }


//
// Clone the data attached to this string, allocating 'nExtraLen' characters.
// Place the result in uninitialized string 'dest'.  Copy the part or all of
// original data to start of new string
//
void cAnsiStr::AllocCopy(cAnsiStr & dest, int nCopyLen, int nCopyIndex,
                          int nExtraLen) const
    {
    int nNewLen = nCopyLen + nExtraLen;
    if (nNewLen == 0)
        dest.Init();
    else
        {
        dest.AllocBuffer(nNewLen);
        memcpy(dest.m_pchData, &m_pchData[nCopyIndex], nCopyLen);
        }
    }


//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const cAnsiStr&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//
void cAnsiStr::Assign(int nSrcLen, const char *pszSrcData)
    {
    if (nSrcLen < 0)
        {
        CriticalMsg("Invalid length");
        return;
        }

    // Test for assignment to self
    if (nSrcLen && pszSrcData == m_pchData)
        return;

    // check if it will fit
    if (nSrcLen > m_nAllocLength)
        {
        // it won't fit, allocate another one
        AllocBuffer(nSrcLen);
        }

    if ((m_pchData && pszSrcData) || nSrcLen == 0)
        {
        memcpy(m_pchData, pszSrcData, nSrcLen);
        m_nDataLength = nSrcLen;
        m_pchData[nSrcLen] = '\0';
        }
    else
        {
        CriticalMsg("Null string assignment");
        m_nDataLength = 0;
        if (m_pchData)
            m_pchData[0] = '\0';
        }
    }


//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator +" is done as friend functions for simplicity
//      There are three variants:
//          cAnsiStr + cAnsiStr
// and for ? = char, const char*
//          cAnsiStr + ?
//          ? + cAnsiStr

void cAnsiStr::ConcatCopy(int nSrc1Len, const char *pszSrc1Data,
                           int nSrc2Len, const char *pszSrc2Data)
    {
    // -- master concatenation routine
    // Concatenate two sources

    int nNewLen = nSrc1Len + nSrc2Len;
    AllocBuffer(nNewLen);
    memcpy(m_pchData, pszSrc1Data, nSrc1Len);
    memcpy(&m_pchData[nSrc1Len], pszSrc2Data, nSrc2Len);
    }


cAnsiStr operator + (const cAnsiStr & string1, const cAnsiStr & string2)
    {
    cAnsiStr s;
    s.ConcatCopy(string1.m_nDataLength, string1.m_pchData,
                 string2.m_nDataLength, string2.m_pchData);
    return s;
    }


cAnsiStr operator + (const cAnsiStr & string, const char *psz)
    {
    cAnsiStr s;
    s.ConcatCopy(string.m_nDataLength, string.m_pchData, SafeStrlen(psz), psz);
    return s;
    }


cAnsiStr operator + (const char *psz, const cAnsiStr & string)
    {
    cAnsiStr s;
    s.ConcatCopy(SafeStrlen(psz), psz, string.m_nDataLength, string.m_pchData);
    return s;
    }


//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void cAnsiStr::Append(int nSrcLen, const char *pszSrcData)
    {
    if (nSrcLen < 0)
        {
        CriticalMsg("Invalid length");
        return;
        }

    if (m_pchData == &_ChNil)
        {
        Assign(nSrcLen, pszSrcData);
        return;
        }

    int nReqLen = m_nDataLength + nSrcLen;
// If the buffer is too small...
    if (nReqLen > m_nAllocLength)
        {
        // ...then enlarge it
        int nAllocLength = NextAllocLen(m_nAllocLength, nReqLen);
        m_pchData = ReallocStr(m_pchData, nAllocLength);
        m_nAllocLength = nAllocLength - 1;
        }

    memcpy(&m_pchData[m_nDataLength], pszSrcData, nSrcLen);
    m_nDataLength = nReqLen;

    AssertMsg(m_nDataLength <= m_nAllocLength, "Concatination result too long");
    m_pchData[m_nDataLength] = '\0';
    }
//
// concatenation
//

cAnsiStr
operator + (const cAnsiStr & string1, char ch)
    {
    cAnsiStr s;
    s.ConcatCopy(string1.m_nDataLength, string1.m_pchData, 1, &ch);
    return s;
    }


cAnsiStr
operator + (char ch, const cAnsiStr & string)
    {
    cAnsiStr s;
    s.ConcatCopy(1, &ch, string.m_nDataLength, string.m_pchData);
    return s;
    }


// Substring thingies

int cAnsiStr::SpanIncluding(const char *pszCharSet, int nFirst) const
    {
    AssertMsg(nFirst >= 0, "Invalid range");

    if (nFirst >= m_nDataLength)
        return 0;

    return strspn(m_pchData + nFirst, pszCharSet);
    }


int cAnsiStr::SpanExcluding(const char *pszCharSet, int nFirst) const
    {
    AssertMsg(nFirst >= 0, "Invalid range");

    if (nFirst >= m_nDataLength)
        return 0;

    return strcspn(m_pchData + nFirst, pszCharSet);
    }


//////////////////////////////////////////////////////////////////////////////
// Finding

int cAnsiStr::Find(char ch, int nFirst) const
    {
    // find a single character (strchr)

    AssertMsg(nFirst >= 0, "Invalid range");

    if (nFirst > m_nDataLength)
        return -1;

    register char *pchData = m_pchData + nFirst;
    register char *psz = (char *) strchr(pchData, ch);
    return psz ? psz - m_pchData : -1;
    }


int cAnsiStr::ReverseFind(char ch) const
    {
    // find a single character (start backwards, strrchr)

    register char *psz;
    psz = (char *) strrchr(m_pchData, ch);
    return (psz == NULL) ? -1 : psz - m_pchData;
    }


int cAnsiStr::FindOneOf(const char *pszCharSet, int nFirst) const
    {
    // like single character find, but look for any of the characters
    // in the string "pszCharSet", like strpbrk
    AssertMsg(nFirst >= 0, "Invalid range");

    if (nFirst > m_nDataLength)
        return -1;

    register char *pchData = m_pchData + nFirst;
    register char *psz = (char *) strpbrk(pchData, pszCharSet);
    return psz ? psz - pchData : -1;
    }


int cAnsiStr::Find(const char *pszSub, int nFirst) const
    {
    // find a sub-string (like strstr)
    AssertMsg(nFirst >= 0, "Invalid range");

    if (nFirst > m_nDataLength)
        return -1;

    register char *pchData = m_pchData + nFirst;
    register char *psz = (char *) strstr(pchData, pszSub);
    return psz ? psz - m_pchData : -1;
    }


///////////////////////////////////////////////////////////////////////////////
// Advanced access

void cAnsiStr::DoGrowBuffer(int nMinBufLength)   // must be growing!
    {
    LAssertMsg(nMinBufLength > m_nAllocLength, "Must be growing!");
// we have to grow the buffer, AllocBuffer will realloc
    const int nOldLen = m_nDataLength;           // AllocBuffer will tromp m_nDataLength

    AllocBuffer(nMinBufLength);

    m_nDataLength = nOldLen;
    m_pchData[m_nDataLength] = '\0';
    }
// Attach Buffer will free the previous buffer (if not the same)
// It will set both lengths, with either provided or computed values
//
char *cAnsiStr::Detach(void)
    {
    char *pvOldBuff = m_pchData;
    if (pvOldBuff == &_ChNil)
        {                                        // Copy the one true NIL, DON'T return it.
        pvOldBuff = AllocStr(1);
        pvOldBuff[0] = '\0';
        }
    Init();
    return pvOldBuff;
    }


void cAnsiStr::Attach(char *pNewBuff, int nNewLength, int nNewAlloc)
    {

    if (!pNewBuff)
        {
        AssertMsg(nNewLength == 0 || nNewLength == -1, "Attach Null w/ Length");
        AssertMsg(nNewAlloc == 0 || nNewAlloc == -1, "Attach Null w/ AllocStr");
        Empty();
        return;
        }                                        // The Nil buffer

    AssertMsg(pNewBuff != m_pchData || m_pchData == &_ChNil, "Can't attach held pointer!");

    Empty();                                     // or more efficiently just if...free...
    m_pchData = pNewBuff;
    BufDone(nNewLength, nNewAlloc);
    }


void cAnsiStr::BufDone(int nNewLength, int nNewAlloc)
    {

    if (nNewLength == -1)
        nNewLength = strlen(m_pchData);          // zero terminated

    if (nNewAlloc == -1)
        nNewAlloc = (nNewLength + 1);            // we don't count the +1 for the '\0' which strlen doesn't count

    AssertMsg(nNewLength <= nNewAlloc, "Invalid length");


    m_nDataLength = nNewLength;
    m_nAllocLength = nNewAlloc - 1;
    m_pchData[m_nDataLength] = '\0';
    }
///////////////////////////////////////////////////////////////////////////////
// Insertion & Removal

int cAnsiStr::Insert(const char *psz, int nIndex)
    {
    int nSrcLen = SafeStrlen(psz);
    if (nSrcLen == 0)
        return 0;

    int nNewLen = m_nDataLength + nSrcLen;
    AssertMsg(nIndex <= m_nDataLength, "Invalid range");

    // If the buffer is too small...
    if (nNewLen > m_nAllocLength)
        {
        int nOldLen = m_nDataLength;
        char *pszOldData = Detach();
        // ...then allocate a new buffer
        AllocBuffer(nNewLen);

        // Copy in the front of the old string
        memcpy(m_pchData, pszOldData, nIndex);

        // Copy in the new string
        memcpy(&m_pchData[nIndex], psz, nSrcLen);

        // Copy in the tail of the old string (and the null)
        memcpy(&m_pchData[nIndex + nSrcLen], &pszOldData[nIndex],
               nOldLen - nIndex + 1);

        AssertMsg(pszOldData, "Insert failed");

        if (pszOldData != &_ChNil)
            FreeStr(pszOldData);
        }
    else
        {
        // Shift the tail of the string (and the null) to the right
        memmove(&m_pchData[nIndex + nSrcLen], &m_pchData[nIndex],
                m_nDataLength - nIndex + 1);

        // Copy in the new data
        memcpy(&m_pchData[nIndex], psz, nSrcLen);

        m_nDataLength = nNewLen;
        }

    AssertMsg(m_nDataLength <= m_nAllocLength, "Inserted result to big");

    return nSrcLen;
    }


//
// Insert a string into this string
//
int cAnsiStr::Insert(char ch, int nIndex)
    {
    char buf[2];
    buf[0] = ch;
    buf[1] = 0;
    return Insert(buf, nIndex);
    }


//
// Delete part of this string
//
void cAnsiStr::Remove(int nStart, int nLength)
    {
    AssertMsg(nStart >= 0 && nLength >= 0, "Invalid range");

    if (nStart > m_nDataLength)
        nStart = m_nDataLength;

    if (nStart > m_nDataLength - nLength)
        nLength = m_nDataLength - nStart;

    if (nLength)
        // Shift the end of the string (and the null) to the left
        memmove(&m_pchData[nStart], &m_pchData[nStart + nLength],
                m_nDataLength - nStart - nLength + 1);

    m_nDataLength -= nLength;
    }


//
// Search backward for a character not in the specified set
//
int cAnsiStr::ReverseExcluding(const char *pszCharSet, int nFirst) const
    {
    AssertMsg(nFirst >= 0, "Invalid range");

    if (nFirst > m_nDataLength)
        nFirst = m_nDataLength;

    char *p1 = m_pchData + nFirst;
    while (p1 >= m_pchData)
        {
        const char *p2 = pszCharSet;
        while (*p2)
            {
            if (*p2 == *p1)
                goto FOUND;
            p2++;
            }
        break;                                   /* it's not in p2        */
FOUND:
        p1--;
        }

    return m_pchData + nFirst - p1;
    }

//
// Search backward for a character in the specified set
//
int cAnsiStr::ReverseIncluding(const char *pszCharSet, int nFirst) const
    {
    AssertMsg(nFirst >= 0, "Invalid range");

    if (nFirst > m_nDataLength)
        nFirst = m_nDataLength;

    char *p1 = m_pchData + nFirst;
    while (p1 > m_pchData)
        {
        const char *p2 = pszCharSet;
        while (*p2)
            {
            if (*p2 == *p1)
                goto FOUND;
            p2++;
            }
        p1--;
        }

FOUND:
    return m_pchData + nFirst - p1;
    }

//
// Remove leading and trailing whitespace
//
void cAnsiStr::Trim()
    {
    if (m_nDataLength != 0)
        {
        char * pStart = m_pchData;
        char * pEnd   = m_pchData + m_nDataLength;

        // Skip leading whitespace
        while (*pStart && isspace(*pStart))
            pStart++;

        // If we hit something, scan from the end
        if (*pStart)
            {
            while (isspace(*(pEnd - 1)))
                {
                pEnd--;
                }
            *pEnd = 0;

            if (pStart != m_pchData || pEnd != m_pchData + m_nDataLength)
                {
                m_nDataLength = strlen(pStart);
                memmove(m_pchData, pStart, m_nDataLength + 1);
                }
            }
        else
            Empty();

        }
    }



//
// Default allocation routine
//
char *cAnsiStr::AllocStr(int len)
    {
    return (char *) malloc(len);
    }


//
// Default reallocation routine
//
char *cAnsiStr::ReallocStr(char *p, int len)
    {
    return (char *) realloc((void *) p, len);
    }


//
// Default free routine
//
void cAnsiStr::FreeStr(char *p)
    {
    if (p)
        free((void *) p);
    }


//
// Quote this string
//
const cAnsiStr & cAnsiStr::Quoted(eQuoteMode mode)
    {
    if (mode == kOff)
        return *this;

    switch (mode)
        {
        case kDoubleQuotes:
                {
                for (int i = Find('"', 0); i != -1; i = Find('"', i))
                    {
                    Insert('"', i);
                    i += 2;
                    }
                // Do outside quotes last
                Insert('"', 0);
                *this += '"';
                break;
                }

        case kEscapeQuotes:
                {
                for (int i = Find('"', 0); i != -1; i = Find('"', i))
                    {
                    Insert('\\', i);
                    i += 2;
                    }

                // Do outside quotes last
                Insert('"', 0);
                *this += '"';
                break;
                }

            // Unquote a string (including embedded quotes)
        case kRemoveEmbeddedQuotes:
                {
                for (int i = Find('"', 0); i != -1; i = Find('"', i))
                    {
                    Remove(i, 1);
                    }
                break;
                }

            // Add quotes if string has whitespace
        case kQuoteIfWhite:
                {
                if (FindOneOf(" \t") != -1)
                    {
                    Insert('"', 0);
                    *this += '"';
                    }
                break;
                }

            }

    return *this;
    }


//
// Do sprintf formatting
//

void cAnsiStr::FmtStr(unsigned nBufSize, const char *format,...)
    {
    va_list arg_ptr;
    va_start(arg_ptr, format);
    char *buffer = GetBufferSetLength(nBufSize);
    int n = vsnprintf(buffer, nBufSize - 1, format, arg_ptr);
    ReleaseBuffer(n);
    va_end(arg_ptr);
    }


void cAnsiStr::FmtStr(const char *format,...)
    {
    const kFmtBufSize = 256;

    va_list arg_ptr;
    va_start(arg_ptr, format);
    char *buffer = GetBufferSetLength(kFmtBufSize);
    int n = vsnprintf(buffer, kFmtBufSize - 1, format, arg_ptr);
    ReleaseBuffer(n);
    va_end(arg_ptr);
    }


void cAnsiStr::FmtStr(unsigned nBufSize, ushort nID,...)
    {
    cAnsiStr Str;
    if (Str.LoadString(nID))
        {
        const char *format = Str;
        va_list arg_ptr;
        va_start(arg_ptr, format);
        char *buffer = GetBufferSetLength(nBufSize);
        int n = vsnprintf(buffer, nBufSize - 1, format, arg_ptr);
        ReleaseBuffer(n);
        va_end(arg_ptr);
        }
    }


void cAnsiStr::FmtStr(ushort nID,...)
    {
    cAnsiStr Str;
    if (Str.LoadString(nID))
        {
        const char *format = Str;
        const kFmtBufSize = 256;

        va_list arg_ptr;
        va_start(arg_ptr, format);
        char *buffer = GetBufferSetLength(kFmtBufSize);
        int n = vsnprintf(buffer, kFmtBufSize - 1, format, arg_ptr);
        ReleaseBuffer(n);
        va_end(arg_ptr);
        }
    }


BOOL cAnsiStr::LoadString(ushort nID)
    {
#if 0
    AssertMsg(nID != 0, "0 is an illegal string ID");

// NOTE: resource strings limited to 255 characters
    char szBuffer[256];

    int nLen;
#if 0
// LoadString without annoying warning from the Debug kernel if the
// segment containing the string is not present
    if (::FindResource(AfxGetResourceHandle(),
                       MAKEINTRESOURCE((nID >> 4) + 1), RT_STRING) == NULL)
        nLen = 0;
    else
#endif
#define AfxGetResourceHandle() 0
#pragma message ("NEED EQUIV OF AfxGetResourceHandle()")
        nLen = ::LoadString(AfxGetResourceHandle(), nID, szBuffer, 255);

    Assign(nLen, szBuffer);
    return nLen != 0;
#else
    return FALSE;
#endif
    }

BOOL cAnsiStr::ToStream(cOStore &) const
    {
    return FALSE;
    }

BOOL cAnsiStr::FromStream(cIStore &)
    {
    return FALSE;
    }
