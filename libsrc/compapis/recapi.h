///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/recapi.h $
// $Author: TOML $
// $Date: 1997/06/18 04:42:36 $
// $Revision: 1.19 $
//
// The interface to the state recording system.  This is used to record all non
// deterministic elements of a given program run for later reproduction of
// exceptions and abnormalities.
//

#ifndef __RECAPI_H
#define __RECAPI_H

#include <comtools.h>
#include <recguid.h>

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IRecorder);

///////////////////////////////////////////////////////////////////////////////
//
// Recorder API
//

#define kRecMaxDataSize     255

enum eRecMode
{
    kRecRecord,
    kRecPlayback,
    kRecInactive,
    kRecPausedRecord,
    kRecPausedPlayback
};

typedef enum eRecMode eRecMode;

enum eRecSaveOption
{
    kRecSaveUnconditional,
    kRecSavePrompted,
    kRecDiscardIfNoFailure
};

//
// Create a recorder and add it to the global app-object
//
#define RecorderCreate(fMode, pszRecFileName) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _RecorderCreate(IID_TO_REFIID(IID_IRecorder), NULL, pAppUnknown, fMode, NULL, pszRecFileName, kRecSaveUnconditional); \
    COMRelease(pAppUnknown); \
}

#define RecorderCreate2(fMode, pszRecFilePath, fSaveOption) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _RecorderCreate(IID_TO_REFIID(IID_IRecorder), NULL, pAppUnknown, fMode, pszRecFilePath, NULL, fSaveOption); \
    COMRelease(pAppUnknown); \
}


//
// Creates a recorder, aggregating it with specfied pOuter,
// use IAggregate protocol if ppRecorder is NULL, else self-init
//
EXTERN tResult LGAPI
_RecorderCreate(REFIID, IRecorder ** ppRecorder, IUnknown * pOuter,
                eRecMode fMode,
                const char * pszRecFilepath, const char * pszRecFileName,
                enum eRecSaveOption fSaveOption);


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IRecorder
//
#undef INTERFACE
#define INTERFACE IRecorder

DECLARE_INTERFACE_(IRecorder, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Query the present recorder mode
    //
    STDMETHOD_(eRecMode, GetMode)(THIS) PURE;

    //
    // Combined Add/Extract depending on mode, can be no-op if inactive or recording ended
    //
    STDMETHOD (StreamAddOrExtract)(THIS_ void * pData, size_t sizeData,
                                   const char * pszTypeTag) PURE;

    //
    // Add/extract arbitrary data to/from recording stream
    //
    STDMETHOD (AddToStream)(THIS_ void * pData, size_t sizeData,
                                  const char * pszTypeTag) PURE;

    STDMETHOD (ExtractFromStream)(THIS_ void * pData, size_t sizeData,
                                  const char * pszTypeTag) PURE;

    //
    // Pause/resume recording
    //
    STDMETHOD (Pause)(THIS) PURE;
    STDMETHOD (Resume)(THIS) PURE;
};

// Recorder Macros
#define IRecorder_GetMode(p)                    COMCall0(p, GetMode)
#define IRecorder_AddToStream(p, a, b, c)       COMCall3(p, AddToStream, a, b, c)
#define IRecorder_ExtractFromStream(p, a, b, c) COMCall3(p, ExtractFromStream, a, b, c)

//
// RecStreamAddOrExtract() provides a lightweight interface to the recording
// stream, useful for subsystems with limited recording needs
//

#define RecStreamAddOrExtract(pRecorder, pData, sizeData, pszTypeTag)          \
                                                                               \
    do                                                                         \
    {                                                                          \
        if (pRecorder)                                                         \
        {                                                                      \
            COMCall3((pRecorder),                                              \
                      StreamAddOrExtract,                                      \
                      (pData), (sizeData), pszTypeTag);                        \
        }                                                                      \
    } while (0)


///////////////////////////////////////////////////////////////////////////////

#endif /* !__RECAPI_H */
