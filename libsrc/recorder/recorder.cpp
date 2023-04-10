///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/recorder/RCS/recorder.cpp $
// $Author: JAEMZ $
// $Date: 1997/09/28 20:27:21 $
// $Revision: 1.20 $
//

#include <comtools.h>
#include <lg.h>
#include <appapi.h>
#include <appagg.h>
#include <aggmemb.h>

#include <recorder.h>
#include <recprmpt.h>

#include <filepath.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <sys\types.h>
#include <time.h>    // time, localtime
#include <string.h>  // strncat
#include <stdlib.h>  // getenv
#include <str.h>

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
extern "C"
{
__declspec(dllimport) DWORD __stdcall GetCurrentThreadId(void);
};
#define AssertCorrectThread() AssertMsg(GetCurrentThreadId() == m_ThreadRecordingId, "Cannot record actions in multiple threads.");
#else
#define AssertCorrectThread()
#endif

const unsigned kRecVersion = 3;

///////////////////////////////////////////////////////////////////////////////

tResult LGAPI
_RecorderCreate(REFIID, IRecorder ** /* ppRecorder */ ,
                IUnknown * pOuterUnknown, eRecMode mode,
                const char * pszRecFilepath, const char * pszRecFileName,
                enum eRecSaveOption fSaveOption)
{
    if (new cRecorder(pOuterUnknown, mode, pszRecFilepath, pszRecFileName, fSaveOption) != 0)
        return S_OK;
    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cRecorder
//
// Implementation of generic record/playback system
//

///////////////////////////////////////
//
// Pre-fab COM implementations
//
IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cRecorder);

///////////////////////////////////////

cRecorder::cRecorder(IUnknown * pOuterUnknown, eRecMode mode,
                     const char * pszRecFilepath, const char *pszRecFileName,
                     eRecSaveOption fSaveOption)
  : m_mode(mode),
    m_RecFile(-1),
    m_fSaveOption(fSaveOption),
    m_fFirstAccess(TRUE)
#ifdef _WIN32
    , m_ThreadRecordingId(GetCurrentThreadId())
#endif
{
    // Add internal components to outer aggregate...
    INIT_AGGREGATION_1( pOuterUnknown,
                        IID_IRecorder, this,
                        (kPriorityLibrary - 1),
                        NULL );

    // Set the file name...
    if (pszRecFileName && *pszRecFileName)
        m_RecFileSpec.SetRelativePath(pszRecFileName);
    else
    {
        // Build an unique string based upon the user's login name, current
        // date and time.
        char            szGeneratedFileName[13];
        const char *    pszUserName     = getenv("USER");
        time_t          lTime           = time(NULL);
        tm *            pTm             = localtime(&lTime);

        if (pszUserName)
        {
            strncpy(szGeneratedFileName, pszUserName, 3);
            szGeneratedFileName[3] = 0;
        }
        else
            strcpy(szGeneratedFileName, "REC");

        itoa((pTm->tm_yday << 11) | (pTm->tm_min + (pTm->tm_hour * 60)), szGeneratedFileName + 3, 16);
        strcat(szGeneratedFileName, ".rec");
        m_RecFileSpec.SetRelativePath(szGeneratedFileName);
    }

    // Complete the file path...
    if (pszRecFilepath && *pszRecFilepath)
    {
        m_RecFileSpec.SetFilePath(pszRecFilepath);
    }

    m_RecFileSpec.MakeFullPath();

    // Open the recording file...
    if (m_RecFileSpec.FileExists())
    {
        if (m_mode == kRecRecord)
            m_RecFileSpec.UnlinkFile();
    }
    else
    {
        if (m_mode == kRecPlayback)
            m_mode = kRecInactive;
    }

    if (m_mode == kRecInactive)
        return;

    int access = _O_BINARY;

    if (m_mode == kRecPlayback)
        access |= _O_RDONLY;
    else
        access |= _O_RDWR | _O_CREAT | _O_TRUNC;

    m_RecFile = open(m_RecFileSpec.GetName(), access, S_IREAD | S_IWRITE);

    if (m_RecFile == -1)
    {
        CriticalMsg1("Failed to open recording file %s", m_RecFileSpec.GetName());
        m_mode = kRecInactive;
        return;
    }

    m_pszLastStreamItem = "(no previous)";
    m_sizeLastItem = 0;

}

///////////////////////////////////////

cRecorder::~cRecorder()
{
    // Delete the hash table entries

    // Close the recording file
    if (m_RecFile != -1)
        close(m_RecFile);

    switch (m_fSaveOption)
    {
        case kRecSavePrompted:
            if (RecPromptYesNo("Save recording file?"))
                break;
            // else fall through

        case kRecDiscardIfNoFailure:
            m_RecFileSpec.UnlinkFile();
    }
}

///////////////////////////////////////

const uint8 kRecTypeDesc     = 1;
const uint8 kRecTypedData    = 2;
const uint8 kRecRawData      = 3;

#pragma pack(1)
struct sRecBlockHeader
{
    tRecBlockType blockType;

    union
    {
        // Type descriptor
        uint8 typeSize;

        // Typed data
        uint8 typeID;

        // Raw data
        uint8 rawSize;
    };
};
#pragma pack()

///////////////////////////////////////

inline BOOL cRecorder::WriteBlock(tRecBlockType type, uint8 dataInfo, const void * pData, size_t dataSize)
{
    write(m_RecFile, &type, sizeof(type));
    write(m_RecFile, &dataInfo, sizeof(dataInfo));
    return write(m_RecFile, pData, dataSize) == dataSize;
}

///////////////////////////////////////

inline BOOL cRecorder::ReadBlockHeader(sRecBlockHeader * pHeader)
{
    return read(m_RecFile, pHeader, sizeof(sRecBlockHeader)) == sizeof(sRecBlockHeader);
}

///////////////////////////////////////

inline BOOL cRecorder::ReadBlockData(void * pReadTo, size_t size)
{
    return read(m_RecFile, pReadTo, size) == size;
}

///////////////////////////////////////
//
// Query the present recorder mode
//

STDMETHODIMP_(eRecMode) cRecorder::GetMode()
{
    return m_mode;
}

///////////////////////////////////////

STDMETHODIMP cRecorder::StreamAddOrExtract(void * pData, size_t sizeData , const char * pszTypeTag)
{
    // We call directly to cRecorder here to save the cost of the virtual function, remove
    // if these are overridden at a later date (toml 09-30-96)
    switch (m_mode)
    {
        case kRecRecord:
        {
            return cRecorder::AddToStream(pData, sizeData, pszTypeTag);
        }
        case kRecPlayback:
        {
            return cRecorder::ExtractFromStream(pData, sizeData, pszTypeTag);
        }
    }
    return S_OK;
}

///////////////////////////////////////
//
// Add arbitrary data to recording stream
//

STDMETHODIMP cRecorder::AddToStream(void *pData, size_t sizeData, const char *pszTypeTag)
{
    AssertCorrectThread();
    AssertMsg(m_mode != kRecPlayback, "Can't AddToStream() when playing back");

    // If this is the first use...
    if (m_fFirstAccess)
    {
        // ... write the recording header
        AutoAppIPtr(Application);
        const char pcszRecHeader[] = "---\r\nProject: %s\r\nUser: %s\r\nDate/Time: %s\r\nLGT Recording System Version %d\r\n---\r\n\x1a";

        const char * pszProject;
        if ((void *)pApplication && pApplication->GetCaption())
            pszProject = pApplication->GetCaption();
        else
            pszProject = "Unknown";
        const char *    pszUser     = (getenv("USER")) ? getenv("USER") : "Unknown";
        time_t          lTime       = time(NULL);
        tm *            pTm         = localtime(&lTime);

        cStr recHeaderStr;
        recHeaderStr.FmtStr(pcszRecHeader, pszProject, pszUser, asctime(pTm), kRecVersion);
        write(m_RecFile, recHeaderStr.operator const char *(), recHeaderStr.GetLength() + 1);
        write(m_RecFile, &kRecVersion, sizeof(kRecVersion));
        m_fFirstAccess = FALSE;
    }

    // If the stream addition is typed...
    if (pszTypeTag)
    {
        // Determine if this is a new type name, if not, add it
        sRecDataType *pDataType = m_RecDataTypeTable.Search(pszTypeTag);

        if (!pDataType)
        {
            AssertMsg1(strlen(pszTypeTag) <= kMaxTypeNameLen, "Type name \"%s\" too long for recorder", pszTypeTag);
            AssertMsg1(sizeData <= kRecMaxDataSize, "Type \"%s\" too big for recorder -- record it in smaller pieces", pszTypeTag);
            AssertMsg1(m_RecDataTypeTable.GetCount() < kMaxTypes, "Type \"%s\" exceeds total maximum of recordable types", pszTypeTag);

            pDataType = NewRecDataType();

            pDataType->typeSize = (uint8)sizeData;
            strncpy(pDataType->typeName, pszTypeTag, kMaxTypeNameLen);
            pDataType->typeName[kMaxTypeNameLen] = '\0';

            // Add the new type to the hash table.
            m_RecDataTypeTable.Insert(pDataType);

            // Write out the new type block
            if (!WriteBlock(kRecTypeDesc, pDataType->typeSize, pDataType->typeName, kMaxTypeNameLen + 1))
            {
                CriticalMsg("Error writing to recording");
                m_mode = kRecInactive;
                return E_FAIL;
            }
        }

        // Verify type size now is same as previous add
        if (pDataType->typeSize != sizeData)
        {
            CriticalMsg("Invalid size for recording data");
            m_mode = kRecInactive;
            return E_FAIL;
        }

        // Write the data, typed
        if (!WriteBlock(kRecTypedData, pDataType->typeID, pData, sizeData))
        {
            CriticalMsg("Error writing to recording");
            m_mode = kRecInactive;
            return E_FAIL;
        }
    }
    else
    {
        // Write the data, raw
        if (!WriteBlock(kRecRawData, (uint8)sizeData, pData, sizeData))
        {
            CriticalMsg("Error writing to recording");
            m_mode = kRecInactive;
            return E_FAIL;
        }
    }

    return S_OK;
}

///////////////////////////////////////
//
// Extract arbitrary data from recording stream
//

STDMETHODIMP cRecorder::ExtractFromStream(void *pData, size_t sizeData, const char *pszTypeTag)
{
    AssertCorrectThread();
    AssertMsg(m_mode != kRecRecord, "Can't ExtractFromStream() when recording");

    if (m_fFirstAccess)
    {
        char c;
        int version;

        // Skip header text
        do
        {
            read(m_RecFile, &c, 1);
        } while (c);

        read(m_RecFile, &version, sizeof(version));
        if (version != kRecVersion)
        {
            CriticalMsg2("Cannot use recording -- incompatible recorder version\nRecording is from version %d, current is %d", version, kRecVersion);
            m_mode = kRecInactive;
            return S_FALSE;
        }
        m_fFirstAccess = FALSE;
    }

    ///////////////////////////////////

    sRecBlockHeader header;

    enum eRecReadState
    {
        kReading,
        kDoneSuccess,
        kDoneInconsistentType,
        kDoneExpectedTag,
        kDoneBadRead,
    };

    const char * ppszReadErrors[] =
    {
        "Still reading",
        "No error",
        "Inconsistent type",
        "Expected typed data",
        "Bad read",
    };

    const char * pszThisStreamItem = "<unknown>";
    size_t       sizeThisItem = 0;

    eRecReadState   readState       = kReading;
    BOOL            fEndOfRecording = TRUE;

    while (readState == kReading && ReadBlockHeader(&header))
    {
        fEndOfRecording = FALSE;
        switch (header.blockType)
        {
            case kRecTypeDesc:
            {
                if (pszTypeTag)
                {
                    sRecDataType * pNewDataType = NewRecDataType();
                    pNewDataType->typeSize = header.typeSize;
                    ReadBlockData(pNewDataType->typeName, kMaxTypeNameLen + 1);

                    m_RecDataTypeTable.Insert(pNewDataType);

                    pszThisStreamItem = "new data type";
                    sizeThisItem = header.typeSize;

                    if (strcmp(pNewDataType->typeName, pszTypeTag) != 0 || pNewDataType->typeSize != sizeData) // New type should match client value
                        readState = kDoneInconsistentType;

                    // else state continues to be kReading
                }
                else
                    readState = kDoneExpectedTag;
                break;
            }

            case kRecTypedData:
            {
                if (pszTypeTag)
                {
                    sRecDataType * pDataType = m_RecDataTypeTable.Search(pszTypeTag);

                    pszThisStreamItem = m_RecDataTypesArray[header.typeID].typeName;
                    sizeThisItem = m_RecDataTypesArray[header.typeID].typeSize;

                    if (pDataType && pDataType->typeID == header.typeID && sizeThisItem == sizeData)
                    {
                        if (ReadBlockData(pData, sizeData))
                            readState = kDoneSuccess;
                        else
                            readState = kDoneBadRead;
                    }
                    else
                        readState = kDoneInconsistentType;
                }
                else
                    readState = kDoneExpectedTag;
                break;
            }

            case kRecRawData:
            {
                pszThisStreamItem = "raw data";
                sizeThisItem = header.rawSize;

                if (header.rawSize == sizeData)
                {
                    if (ReadBlockData(pData, sizeData))
                        readState = kDoneSuccess;
                    else
                        readState = kDoneBadRead;
                }
                else
                    readState = kDoneInconsistentType;
                break;
            }

            default:
                CriticalMsg("Error in playback stream: unknown recording file block type");
                readState = kDoneBadRead;
        }
    }

    ///////////////////////////////////

    if (readState != kDoneSuccess)
    {
        AssertMsg7(fEndOfRecording, "Playback error \"%s\".\n"
                                    "Expected %s sized %u,\n"
                                    "Got %s sized %u,\n"
                                    "Last was %s sized %u.\n"
                                    "Recording is inconsistent and/or invalid.",
                                    ppszReadErrors[readState],
                                    (pszTypeTag) ? pszTypeTag : "raw data",
                                    sizeData,
                                    pszThisStreamItem,
                                    sizeThisItem,
                                    m_pszLastStreamItem,
                                    m_sizeLastItem);
        m_mode = kRecInactive;
        return (fEndOfRecording) ? S_FALSE : E_FAIL;
    }

    m_pszLastStreamItem = pszThisStreamItem;
    m_sizeLastItem      = sizeThisItem;

    return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cRecorder::Pause()
{
    int recorderPauseMarker = 0;
    
    switch (m_mode)
    {
        case kRecRecord:
        {
            m_mode = kRecPausedRecord;
            break;
        }
        case kRecPlayback:
        {
            m_mode = kRecPausedPlayback;
            break;
        }
    }
    StreamAddOrExtract(&recorderPauseMarker, sizeof(recorderPauseMarker), "Record pause marker");
    return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cRecorder::Resume()
{
    int recorderResumeMarker = 0;
    
    switch (m_mode)
    {
        case kRecPausedRecord:
        {
            m_mode = kRecRecord;
            break;
        }
        case kRecPausedPlayback:
        {
            m_mode = kRecPlayback;
            break;
        }
        default:
            CriticalMsg("Called recorder resume when not paused");
    }
    StreamAddOrExtract(&recorderResumeMarker, sizeof(recorderResumeMarker), "Record resume marker");
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

tHashSetKey cRecDataTypeTable::GetKey(tHashSetNode node) const
{
    return (tHashSetKey) (((sRecDataType *) (node))->typeName);
}

///////////////////////////////////////////////////////////////////////////////
