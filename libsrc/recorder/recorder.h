///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/recorder/RCS/recorder.h $
// $Author: TOML $
// $Date: 1997/08/12 12:18:29 $
// $Revision: 1.13 $
//

#ifndef __RECORDER_H
#define __RECORDER_H

#include <aggmemb.h>
#include <comtools.h>
#include <recapi.h>
#include <filespec.h>
#include <dynarray.h>
#include <hashset.h>

#pragma pack(4)

//////////////////////////////////////////////////////////////////////////////
//
// CLASS: cRecDataTypeTable
//
// Implementation of a string hash table to store types in the output file format.
// (based upon the templates in hashset.h)
//

const int kMaxTypeNameLen = 31;
const int kMaxTypes = 256;

struct sRecDataType
{
    char typeName[kMaxTypeNameLen + 1];     // The name of the string to hash
    uint8 typeID;                            // The ID to read/write from/to the rec file.
    uint8 typeSize;                          // The data size.
};

typedef cDynArray<sRecDataType> cRecDataTypesArray;

class cRecDataTypeTable : public cStrHashSet < sRecDataType *>
{
 public:
    cRecDataTypeTable()
    {
    };

 private:
    virtual tHashSetKey GetKey(tHashSetNode) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cRecorder
//
// Implementation of generic record/playback system
//

typedef uint8 tRecBlockType;
struct sRecBlockHeader;

class cRecorder : public IRecorder
{
 public:
    cRecorder(IUnknown * pOuterUnknown, eRecMode mode,
              const char * pszRecFilepath, const char *pszRecFileName,
              eRecSaveOption fSaveOption);
    virtual ~cRecorder();

 private:
    //
    // IUnknown methods
    //
    DECLARE_SIMPLE_AGGREGATION(cRecorder);

    //
    // Query the present recorder mode
    //
    STDMETHOD_(eRecMode, GetMode)();

    //
    // Combined Add/Extract depending on mode
    //
    STDMETHOD (StreamAddOrExtract)(void * pData, size_t sizeData,
                                   const char * pszTypeTag);

    //
    // Add/extract arbitrary data to/from recording stream
    //
    STDMETHOD(AddToStream) (void *pData, size_t sizeData,
                            const char *pszTypeTag);
    STDMETHOD(ExtractFromStream) (void *pData, size_t sizeData,
                                  const char *pszTypeTag);

    STDMETHOD (Pause)();
    STDMETHOD (Resume)();

    ///////////////////////////////////

    BOOL WriteBlock(tRecBlockType type, uint8 dataInfo, const void * pData, size_t dataSize);
    BOOL ReadBlockHeader(sRecBlockHeader * pHeader);
    BOOL ReadBlockData(void * pReadTo, size_t size);

    ///////////////////////////////////

    sRecDataType * NewRecDataType()
    {
        // @TBD (toml 06-18-97): replace this dynarray: its the wrong thing for this case
        AssertMsg(m_RecDataTypeTable.GetCount() < kMaxTypes, "Too many types in recording!");

        sRecDataType * pDataType = &(m_RecDataTypesArray[m_RecDataTypeTable.GetCount()]);
        pDataType->typeID = (uint8) m_RecDataTypeTable.GetCount();
        pDataType->typeSize = 0;
        pDataType->typeName[0] = 0;
        return pDataType;
    }

    ///////////////////////////////////
    //
    // Instance Data
    //

    eRecMode            m_mode;                   // Current recorder mode
    cFileSpec           m_RecFileSpec;

    cRecDataTypeTable   m_RecDataTypeTable;       // Hash table for data types
    sRecDataType        m_RecDataTypesArray[kMaxTypes];

    int                 m_RecFile;                // RecFile descriptor
    eRecSaveOption      m_fSaveOption;

    BOOL                m_fFirstAccess;           // @Note (toml 09-30-96): This variable may want to change to an enumerated "state"
#ifdef _WIN32
    ulong               m_ThreadRecordingId;
#endif
    const char *        m_pszLastStreamItem;
    size_t              m_sizeLastItem;
};

#pragma pack()

///////////////////////////////////////////////////////////////////////////////

#endif /* !__RECORDER_H */
