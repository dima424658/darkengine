///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/filespec.h $
// $Author: TOML $
// $Date: 1996/10/21 14:14:58 $
// $Revision: 1.3 $
//
// (c) Copyright 1993 - 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.

#ifndef _H_FILESPEC
#define _H_FILESPEC

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

#include <filekind.h>

#ifndef __cplusplus
// @TBD (toml 03-22-96): Need C interface to this functionality
#error "No C version of filespec"
#endif

// @TBD (toml 03-22-96): Make export/importable
#define __CPPTOOLSAPI

//
// Forward declarations
//
class cFilePath;
class cIStore;
class cOStore;
class cAnsiStr;
class cUniStr;

#ifdef UNICODE
    typedef cUniStr cStr;
#else
    typedef cAnsiStr cStr;
#endif


//
// Accessory types
//
enum eFileSpecNameStringStyle
    {
    kFullPathNameStyle,
    kNameOnlyNameStyle,
    kAnchorRelativeNameStyle,
    kCurDirRelativeNameStyle
    };

typedef int tUntitledNumber;


//
// cFileSpec
//
// This class *must not* add any non-static data members to sFileDesc, and *must not* have any virtual functions
//
class __CPPTOOLSAPI cFileSpec : public sFileDesc
    {
friend class cFilePath;

public:
    // Constructors/destructors
    cFileSpec();
    cFileSpec(const cFileSpec &);
    cFileSpec(const sFileDesc &);

    // Create (relative) specification of a file given its string
    cFileSpec(const char *);

    // Create a specification of a file given a path and a string
    cFileSpec(const cFilePath &, const char *);

    // Create a specification of a file from portable pieces
    cFileSpec(eFileSpecKind, const char *);

    ~cFileSpec();

    cFileSpec &operator=(const cFileSpec &);
    cFileSpec &operator=(const sFileDesc &);

    // Comparisons
    BOOL operator==(const sFileDesc &) const;
    BOOL operator!=(const sFileDesc &) const;
	int Compare(const sFileDesc &) const;

    // Set kind to unknown (reinitialize)
    void Empty();

    // Get a user readable name string for the file specification
    void GetNameString(cStr &, eFileSpecNameStringStyle = kFullPathNameStyle) const;
    void GetNameString(cStr &, int iDesiredLength) const;
    void GetNameString(cStr &, const cFilePath &) const;

    // Set/Get anchor used by GetNameString() and Anchoring functions
    static const cFilePath & GetAnchorPath();
    static void SetAnchorPath(const cFilePath &);

    // Return a pointer to the file root and extension
    const char *GetFileName() const;

    // Get a string with the file root and extension
    void GetFileName(cStr &fileName) const;

    // Set the root and extension
    void SetFileName(const char *fileName);

    // Get just the file root
    void GetFileRoot(cStr &root) const;

    // Set the file root
    void SetFileRoot(const char* root);

    // Get the extension of this
    void GetFileExtension(cStr& extension) const;
    const char * GetFileExtension() const;

    // Change the extension of this
    void SetFileExtension(const char* extension);

    // Return the file path of this
    void GetFilePath(cFilePath &) const;

    // Replace the file path of this, leaving the file name intact
    void SetFilePath(const cFilePath &);

    // Make this a full path relative to the CWD
    BOOL MakeFullPath();

    // Make this a full path relative to the specified path
    BOOL MakeFullPath(const cFilePath &);

    // Make this relative to the default anchor path (obsolete)
    BOOL MakeAnchoredPath();

    // Make this a full path relative to the default anchor path (obsolete)
    BOOL MakeUnanchoredPath();

    // Make this relative to the specified anchor path
    BOOL MakeAnchoredPath(const cFilePath &);

    // Return a string with this relative to the CWD
    BOOL GetFullPath(cStr &) const;

    // Return a string with this relative to the default anchor path (obsolete)
    BOOL GetAnchoredPath(cStr &) const;

    // Return a string with this relative to the specified anchor path
    BOOL GetAnchoredPath(cStr &, const cFilePath &) const;

    // Return a full path string with this relative to the specified path
    BOOL GetFullPath(cStr &, const cFilePath &) const;

    // Return TRUE if this is a valid file specification
    BOOL IsValidFileSpec() const;

    // Return TRUE if file specified by this exists
    BOOL FileExists() const;

    // Delete the file specified by this
    BOOL UnlinkFile() const;

    BOOL IsReadOnly() const;
    BOOL IsWriteOnly() const;
    BOOL IsModified() const;
    BOOL IsHidden() const;
    BOOL IsSystem() const;

    ulong GetModificationTime() const;

    void Set(const cFilePath &, const char *);

    void GetRelativePath(cStr &) const;
    void SetRelativePath(const char *);
    BOOL IsRelativePath() const;

    // Untitled access functions
    void SetUntitledNumber(tUntitledNumber);

    // Special name access functions
    void SetSpecialName(const char *);

    // Streaming functions
    BOOL ToStream(cOStore &) const;
    BOOL FromStream(cIStore &);

    // Wildcarding
    BOOL NameMatches(const char * pszWildcard);

    // Volume information
    BOOL GetRootDir(cStr &) const;
    BOOL GetVolumeName(cStr &) const;
    BOOL GetFileSystemName(cStr &) const;
    ulong GetVolumeSerialNumber() const;
    int GetCaseConventions() const;
    ulong GetMaxLegalComponentLen() const;
    BOOL IsValidChar(char c) const;
    BOOL IsValidComponentName(const char * psz) const;

    static BOOL IsValidFileSpec(const char *);

private:
    void ReplaceParts(const char *pDrive = 0, const char *pDir = 0,
            const char *pRoot = 0, const char *pExt = 0);

    BOOL IsAttributeSet(unsigned) const;
    static cFilePath AnchorPath;
    };


//
// Construct an empty file specification
//
inline cFileSpec::cFileSpec()
    {
    }


//
// Return TRUE if two file specifications are not logically equal
//
inline BOOL cFileSpec::operator!=(const sFileDesc &FileSpec) const
    {
    return !operator==(FileSpec);
    }


//
// Assignment operator
//
inline cFileSpec &cFileSpec::operator=(const cFileSpec &FileSpec)
    {
    return *this = (sFileDesc &)FileSpec;
    }

inline const cFilePath & cFileSpec::GetAnchorPath()
    {
    return AnchorPath;
    }

#endif

