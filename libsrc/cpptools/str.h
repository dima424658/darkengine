///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/str.h $
// $Author: TOML $
// $Date: 1998/05/20 17:42:49 $
// $Revision: 1.5 $
//
// (c) Copyright 1993 - 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//
// class cStr
//

#ifndef __STR_H
#define __STR_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

#include <string.h>
#include <limits.h>

#include <lg.h>

#define __CPPTOOLSAPI

#if defined (_WIN32)
    #ifndef LoadString
        #ifdef UNICODE
            #define LoadString  LoadStringW
        #else
            #define LoadString  LoadStringA
        #endif
    #endif
#else
    #define LoadString  NoLoadString
#endif

class cArgs;
class cOStore;
class cIStore;

////////////////////////////////////////////////////////////
//
// Get the length or return zero if we this is NULL
//
inline int SafeStrlen(const char *psz)
    {
    return psz ? strlen(psz) : 0;
    }

////////////////////////////////////////////////////////////
//
// cStr: Neutral string
//
class cAnsiStr;
class cUniStr;

#ifdef UNICODE
    typedef cUniStr cStr;
    #error "Need unicode version of cStr"
#else
    typedef cAnsiStr cStr;
#endif

////////////////////////////////////////////////////////////
//
// cAnsiStr: 1-byte character string
//

class __CPPTOOLSAPI cAnsiStr
    {
public:
    enum eQuoteMode
        {
        // No change
        kOff,

        // Double-up the quotes
        kDoubleQuotes,

        // Escape the quotes with a '\'
        kEscapeQuotes,

        // Quote if whitespace present
        kQuoteIfWhite,

        // Remove embedded quotes (e.g., "foo"\"bar" ==> foo\bar
        kRemoveEmbeddedQuotes,
        };

    // Constructors/Destructors
    cAnsiStr();
    cAnsiStr(const cAnsiStr &);
    cAnsiStr(char ch);
    cAnsiStr(const char *);
    cAnsiStr(const char *, int nLen);
    cAnsiStr(int nLen);
    ~cAnsiStr();

    int GetLength() const;
    BOOL IsEmpty() const;

    // Free up the data
    void Empty();

    // True after cAnsiStr() or Empty().
    BOOL IsInitialEmpty() const;

    // Index functions (zero 0)
    char GetAt(int nIndex) const;
    char &operator[](int nIndex) const;
    void SetAt(int nIndex, char ch);

    // as a C string
    operator const char *() const;

    // Assignment
    const cAnsiStr& operator=(const cAnsiStr& stringSrc);
    const cAnsiStr& operator=(char ch);
    const cAnsiStr& operator=(const char* psz);

    // String concatenation
    const cAnsiStr& operator+=(const cAnsiStr& string);
    const cAnsiStr& operator+=(char ch);
    const cAnsiStr& operator+=(const char* psz);

    friend cAnsiStr operator+(const cAnsiStr& string1, const cAnsiStr& string2);
    friend cAnsiStr operator+(const cAnsiStr& string, char ch);
    friend cAnsiStr operator+(char ch, const cAnsiStr& string);
    friend cAnsiStr operator+(const cAnsiStr& string, const char* psz);
    friend cAnsiStr operator+(const char* psz, const cAnsiStr& string);

    // Insertion and removal
    int Insert(const char *psz, int nIndex);
    int Insert(char ch, int nIndex);
    void Remove(int nStart, int nLength);
    void Assign(int nSrcLen, const char* pszSrcData);
    void Append(int nSrcLen, const char* paChars); // may contain embeded '\0's

    // String comparison
    int Compare(const char* psz) const;         // straight character
    int CompareNoCase(const char* psz) const;   // ignore case
    int Collate(const char* psz) const;         // NLS aware

    // simple sub-string extraction
    void Mid(cAnsiStr &, int nFirst, int nCount) const;

    // string scanning
    int SpanIncluding(const char* pszCharSet, int nFirst = 0) const;
    int SpanExcluding(const char* pszCharSet, int nFirst = 0) const;
    int ReverseIncluding(const char* pszCharSet, int nFirst = INT_MAX) const;
    int ReverseExcluding(const char* pszCharSet, int nFirst = INT_MAX) const;
    
    // Remove leading and trailing whitespace
    void Trim();

    // upper/lower/reverse conversion
    void MakeUpper();
    void MakeLower();
    void MakeReverse();

    // searching (return starting index, or -1 if not found)
    // look for a single character match
    int Find(char ch, int nFirst = 0) const; // like "C" strchr
    int ReverseFind(char ch) const;
    int FindOneOf(const char* pszCharSet, int nFirst = 0) const;

    // look for a specific sub-string
    int Find(const char* pszSub, int nFirst = 0) const;

    // truncate or grow the string
    void SetLength(int nNewLength);

    // Access to string implementation buffer as "C" character array
    // Callers of Get* call ReleaseBuffer()
    // if they use the cAnsiStr before it's DTOR is called.

    // grows old buffer if necessary
    char *GetBuffer(int nMinBufLength);

    // restricts length to nNewLength, with \0.
    char *GetBufferSetLength(int nNewLength);

    // called after GetBuffer: adjust internals
    void ReleaseBuffer(int nNewLength);
    void ReleaseBuffer();

    // Fetch pointer for use as an input parameter
    const char *BufIn(void) const;

    // Fetch pointer for use as an output parameter
    char **BufOut(void);

    // Fetch pointer for use as an input/output parameter
    char **BufInOut(void);

    // Must call after BufOut() and BufInOut()
    void BufDone(int nNewLength = -1,  int nAllocLength = -1);


    // Explicit storage management

    // Disown the pointer and empty this out -- caller must free
    char *Detach(void);

    // Accept ownership of specified string -- must have been alloc'd
    void Attach(char*,int nNewLength = -1, int nAllocLength = -1);

    // Formatting
    BOOL LoadString(ushort nID);
    void Format(const char *, const cArgs &);
    void FmtStr(const char *, ...);
    void FmtStr(unsigned nBufSize, const char *, ...);
    void FmtStr(ushort nID, ...);
    void FmtStr(unsigned nBufSize, ushort nID, ...);


    cAnsiStr const &Quoted(eQuoteMode); // mutates self, returns reference

    // Streaming functions
    BOOL ToStream(cOStore &) const;
    BOOL FromStream(cIStore &);

private:
    // Note: an extra character is always allocated

    char *m_pchData;            // actual string (zero terminated)
    int m_nDataLength;          // does not include terminating 0
    int m_nAllocLength;         // does not include terminating 0

    static char _ChNil;       // The Initial Nil String

    // implementation helpers
    void Init();
    void AllocCopy(cAnsiStr &dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    void AllocBuffer(int nLen);
    void ConcatCopy(int nSrc1Len, const char* pszSrc1Data, int nSrc2Len, const char *pszSrc2Data);

    char *AllocStr(int len);
    char *ReallocStr(char *, int len);
    void FreeStr(char *);
    void DoGrowBuffer(int);
    };


// Compare helpers
inline BOOL operator==(const cAnsiStr &s1, const cAnsiStr &s2)
    {
    return s1.Compare(s2) == 0;
    }

inline BOOL operator==(const cAnsiStr &s1, const char *s2)
    {
    return s1.Compare(s2) == 0;
    }

inline BOOL operator==(const char *s1, const cAnsiStr &s2)
    {
    return s2.Compare(s1) == 0;
    }

inline BOOL operator!=(const cAnsiStr& s1, const cAnsiStr& s2)
    {
    return s1.Compare(s2) != 0;
    }

inline BOOL operator!=(const cAnsiStr& s1, const char* s2)
    {
    return s1.Compare(s2) != 0;
    }

inline BOOL operator!=(const char* s1, const cAnsiStr& s2)
    {
    return s2.Compare(s1) != 0;
    }

inline BOOL operator<(const cAnsiStr& s1, const cAnsiStr& s2)
    {
    return s1.Compare(s2) < 0;
    }

inline BOOL operator<(const cAnsiStr& s1, const char* s2)
    {
    return s1.Compare(s2) < 0;
    }

inline BOOL operator<(const char* s1, const cAnsiStr& s2)
    {
    return s2.Compare(s1) > 0;
    }

inline BOOL operator>(const cAnsiStr& s1, const cAnsiStr& s2)
    {
    return s1.Compare(s2) > 0;
    }

inline BOOL operator>(const cAnsiStr& s1, const char* s2)
    {
    return s1.Compare(s2) > 0;
    }

inline BOOL operator>(const char* s1, const cAnsiStr& s2)
    {
    return s2.Compare(s1) < 0;
    }

inline BOOL operator<=(const cAnsiStr& s1, const cAnsiStr& s2)
    {
    return s1.Compare(s2) <= 0;
    }

inline BOOL operator<=(const cAnsiStr& s1, const char* s2)
    {
    return s1.Compare(s2) <= 0;
    }

inline BOOL operator<=(const char* s1, const cAnsiStr& s2)
    {
    return s2.Compare(s1) >= 0;
    }

inline BOOL operator>=(const cAnsiStr& s1, const cAnsiStr& s2)
    {
    return s1.Compare(s2) >= 0;
    }

inline BOOL operator>=(const cAnsiStr& s1, const char* s2)
    {
    return s1.Compare(s2) >= 0;
    }

inline BOOL operator>=(const char* s1, const cAnsiStr& s2)
    {
    return s2.Compare(s1) <= 0;
    }

inline void cAnsiStr::Init(void)
    {
    m_nDataLength = m_nAllocLength = 0;
    m_pchData = &_ChNil;
    }

inline cAnsiStr::cAnsiStr()
    {
    Init();
    }

//
// Destroy the string
//
inline cAnsiStr::~cAnsiStr()
    {
    if (m_pchData != &_ChNil)
        FreeStr(m_pchData);
    }

inline void cAnsiStr::Empty()
    {
    if (m_pchData != &_ChNil)
        {
        FreeStr(m_pchData);
        Init();
        }
    }

//
// Assignment operators
//
inline const cAnsiStr &cAnsiStr::operator =(char ch)
    {
    Assign(1, &ch);
    return *this;
    }

inline const cAnsiStr &cAnsiStr::operator =(const cAnsiStr& stringSrc)
    {
    Assign(stringSrc.m_nDataLength, stringSrc.m_pchData);
    return *this;
    }

inline const cAnsiStr &cAnsiStr::operator =(const char* psz)
    {
    Assign(SafeStrlen(psz), psz);
    return *this;
    }

//
// concatenation
//
inline const cAnsiStr &cAnsiStr::operator +=(const char* psz)
    {
    Append(SafeStrlen(psz), psz);
    return *this;
    }

inline const cAnsiStr &cAnsiStr::operator +=(const cAnsiStr& string)
    {
    Append(string.m_nDataLength, string.m_pchData);
    return *this;
    }

inline const cAnsiStr& cAnsiStr::operator +=(char ch)
    {
    Append(1, &ch);
    return *this;
    }

inline const char *cAnsiStr::BufIn(void) const
    {
    return m_pchData;
    }

inline char **cAnsiStr::BufOut(void)
    {
    Empty();
    return &m_pchData;
    }

// True after DetachBuffer(), cAnsiStr(), Empty().
inline BOOL cAnsiStr::IsInitialEmpty() const
    {
    return (m_pchData == &_ChNil);
    }

//
// Return a pointer to input/output buffer pointer
//
inline char **cAnsiStr::BufInOut(void)
    {
    if (IsInitialEmpty())
        *(m_pchData = AllocStr(1)) = 0;
    return &m_pchData;
    }

inline int cAnsiStr::GetLength() const
    {
    return m_nDataLength;
    }

inline BOOL cAnsiStr::IsEmpty() const
    {
    return m_nDataLength == 0;
    }

inline cAnsiStr::operator const char*() const
    {
    return (const char *)m_pchData;
    }

inline int cAnsiStr::Compare(const char* psz) const
    {
    return strcmp(m_pchData, psz);
    }

inline int cAnsiStr::CompareNoCase(const char* psz) const
    {
    return stricmp(m_pchData, psz);
    }

inline int cAnsiStr::Collate(const char* psz) const
    {
    return strcoll(m_pchData, psz);
    }

inline void cAnsiStr::MakeUpper()
    {
    strupr(m_pchData);
    }

inline void cAnsiStr::MakeLower()
    {
    strlwr(m_pchData);
    }

inline void cAnsiStr::MakeReverse()
    {
    strrev(m_pchData);
    }

inline char* cAnsiStr::GetBuffer(int nMinBufLength)
    {
    if (nMinBufLength > m_nAllocLength)
        DoGrowBuffer(nMinBufLength);
    // return a pointer to the character storage for this string
    return m_pchData;
    }

inline void cAnsiStr::SetLength(int nNewLength)
    {
    GetBuffer(nNewLength);
    m_nDataLength = nNewLength;
    m_pchData[m_nDataLength] = '\0';
    }

inline char* cAnsiStr::GetBufferSetLength(int nNewLength)
    {
    SetLength(nNewLength);
    return m_pchData;
    }

inline void cAnsiStr::ReleaseBuffer(int nNewLength)
    {
    m_nDataLength = nNewLength;
    // Caller must not call ReleaseBuffer(n) with n > requested length.
    AssertMsg(m_nDataLength <= m_nAllocLength, "Invalid length");
    // If the caller passes nNewLength > the length requested by GetBuffer()
    // this MAY fail, but is not guaranteed.  the cAnsiStr remembers the
    // GetBufferSetLength() arg, but not the GetBuffer(minlength) arg.

    //m_nAllocLength = nNewLength; // we don't count the +1 for the \0.
    m_pchData[m_nDataLength] = '\0';
    }

inline void cAnsiStr::ReleaseBuffer()
    {
    m_nDataLength = strlen(m_pchData);
    // Caller must not call ReleaseBuffer(n) with n > requested length.
    AssertMsg(m_nDataLength <= m_nAllocLength, "Invalid length");
    // If the caller passes nNewLength > the length requested by GetBuffer()
    // this MAY fail, but is not guaranteed.  the cAnsiStr remembers the
    // GetBufferSetLength() arg, but not the GetBuffer(minlength) arg.

    //m_nAllocLength = nNewLength; // we don't count the +1 for the \0.
    m_pchData[m_nDataLength] = '\0';
    }

//
// Return the character at the specified index
//
inline char cAnsiStr::GetAt(int nIndex) const
    {
    AssertMsg(nIndex >= 0 && nIndex <= m_nDataLength, "Invalid index");
    return m_pchData[nIndex];
    }


//
// Return a reference to the character at the specified index
//
inline char &cAnsiStr::operator[](int nIndex) const
    {
    AssertMsg(nIndex >= 0 && nIndex <= m_nDataLength, "Invalid index");
    return m_pchData[nIndex];
    }


//
// Set the character at the specified index
//
inline void cAnsiStr::SetAt(int nIndex, char ch)
    {
    AssertMsg(nIndex >= 0 && nIndex < m_nDataLength, "Invalid index");
    AssertMsg(ch != 0, "Invalid null character");
    m_pchData[nIndex] = ch;
    }


//
// Extract a range from the string
//
inline void cAnsiStr::Mid(cAnsiStr &s1, int nFirst, int nCount) const
    {
    AssertMsg(nFirst >= 0 && nFirst + nCount <= GetLength(), "Invalid range");
    s1.Assign(nCount, &(BufIn())[nFirst]);
    }

#endif

