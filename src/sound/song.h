/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/song.h,v 1.5 2000/01/31 10:02:38 adurant Exp $
#pragma once

//
// Song interface
//
// A Song is a fairly simple hierarchical collection of sections, samples, events,
// and gotos. These objects are basically holders for data and, in the case of songs,
// sections, and events, nodes within the hierarchy of data.  In fact, each COM
// interface has one get and one set method for storing and retrieving a structure
// which contains the interesting data for that node.  This was a design decision
// intended to avoid cluttering the interface with many get and set methods.
//
// Basically, a song serves as a binary representation of a parsed song
// file.  However, the interface presented here knows nothing about files or
// file formats: the lex/yacc parser uses this interface to build the song
// from a file.  Once parsed, a song can be queried for information by,
// for instance, a song player.
//
// ----------------------
//
// DEFINITIONS:
//
// Song: Root of the hierarchy.
// Section: A period of time when one or more samples is playing.
// Sample: .Wav data.
// Event: Currently a text string, these are the events being listened for during
//        playback of a section.
// Goto: A branch which deviates from the regular path of samples playing.  By
//       default
//
// ----------------------
//
// CONVENTIONS:
//
// Reference counting: Any time an object is added or retrieved to/from the hierarchy,
// the reference count of the added object is increased.  For instance,
// pSong->AddSection(pSection) or pSong->GetSection(n, &pSection) will increase the
// reference count of pSection.  Note: this implies that a copy of the section is not
// made in either case and the caller must be aware that it is a shared object.
//
// The info structures stored within the objects ARE copied in their get/set methods,
// as the structures are considered "primitives" and are small.
//

#ifndef SONG_H
#define SONG_H

#include <comtools.h>

F_DECLARE_INTERFACE(ISong);
F_DECLARE_INTERFACE(ISongSection);
F_DECLARE_INTERFACE(ISongSample);
F_DECLARE_INTERFACE(ISongEvent);
F_DECLARE_INTERFACE(ISongGoto);

#ifdef __cplusplus
extern "C" {
#endif

BOOL CreateSong (ISong **ppSong, IUnknown *pOuter);
BOOL CreateSongSection (ISongSection **ppSection, IUnknown *pOuter);
BOOL CreateSongSample (ISongSample **ppSample, IUnknown *pOuter);
BOOL CreateSongEvent (ISongEvent **ppEvent, IUnknown *pOuter);
BOOL CreateSongGoto (ISongGoto **ppGoto, IUnknown *pOuter);


#define kSONG_MaxStringLen 32

// Flags
#define kSONG_EventFlagImmediate  0x00000001

// Song interface data structures.
typedef struct sSongInfo
{
   char id [kSONG_MaxStringLen];
} sSongInfo;

typedef struct sSongSectionInfo
{
   char id [kSONG_MaxStringLen];
   int volume;
   int loopCount;
} sSongSectionInfo;

typedef struct sSongSampleInfo
{
   char name [kSONG_MaxStringLen];
   //int loopCount;
   //int probability;
} sSongSampleInfo;

typedef struct sSongEventInfo
{
   char eventString [kSONG_MaxStringLen];
   unsigned flags;
} sSongEventInfo;

typedef struct sSongGotoInfo
{
   int sectionIndex;
   int probability;
} sSongGotoInfo;


// ISong
#undef INTERFACE
#define INTERFACE ISong

DECLARE_INTERFACE_(ISong, IUnknown)
{
   // Comtools macro to declare IUnknown methods
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, SetSongInfo)(THIS_ sSongInfo* pSongInfo) PURE;
   STDMETHOD_(BOOL, GetSongInfo)(THIS_ sSongInfo* ppSongInfo) PURE;

   STDMETHOD_(BOOL, AddSection)(THIS_ ISongSection* pSection) PURE;
   STDMETHOD_(unsigned, CountSections) (THIS) PURE;
   STDMETHOD_(BOOL, SetSection)(THIS_ unsigned index, ISongSection* pSection) PURE;
   STDMETHOD_(BOOL, GetSection)(THIS_ unsigned index, ISongSection** ppSection) PURE;

   STDMETHOD_(BOOL, AddEvent)(THIS_ ISongEvent* pEvent) PURE;
   STDMETHOD_(unsigned, CountEvents)(THIS) PURE;
   STDMETHOD_(BOOL, GetEvent)(THIS_ unsigned index, ISongEvent** ppEvent) PURE;
};

#define ISong_SetSongInfo(p, a)                       COMCall1(p, SetSongInfo, a)
#define ISong_GetSongInfo(p, a)                       COMCall1(p, GetSongInfo, a)
#define ISong_AddSection(p, a)                        COMCall1(p, AddSection, a)
#define ISong_CountSections(p)                        COMCall0(p, CountSections)
#define ISong_SetSection(p, a, b)                     COMCall2(p, SetSection, a, b)
#define ISong_GetSection(p, a, b)                     COMCall2(p, GetSection, a, b)
#define ISong_AddEvent(p, a)                          COMCall1(p, AddEvent, a)
#define ISong_CountEvents(p)                          COMCall0(p, CountEvents)
#define ISong_GetEvent(p, a, b)                       COMCall2(p, GetEvent, a, b)
#define ISong_Release(p)                              COMCall0(p, Release)

// ISongSection
#undef INTERFACE
#define INTERFACE ISongSection

DECLARE_INTERFACE_(ISongSection, IUnknown)
{
   // Comtools macro to declare IUnknown methods
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, SetSectionInfo)(THIS_ sSongSectionInfo* pSectionInfo) PURE;
   STDMETHOD_(BOOL, GetSectionInfo)(THIS_ sSongSectionInfo* pSectionInfo) PURE;
   
   STDMETHOD_(BOOL, AddSample)(THIS_ ISongSample* pSample) PURE;
   STDMETHOD_(unsigned, CountSamples)(THIS) PURE;
   STDMETHOD_(BOOL, GetSample)(THIS_ unsigned index, ISongSample** ppSample) PURE;

   STDMETHOD_(BOOL, AddEvent)(THIS_ ISongEvent* pEvent) PURE;
   STDMETHOD_(unsigned, CountEvents)(THIS) PURE;
   STDMETHOD_(BOOL, GetEvent)(THIS_ unsigned index, ISongEvent** ppEvent) PURE;
};

#define ISongSection_SetSectionInfo(p, a)             COMCall1(p, SetSectionInfo, a)
#define ISongSection_GetSectionInfo(p, a)             COMCall1(p, GetSectionInfo, a)
#define ISongSection_AddSample(p, a)                  COMCall1(p, AddSample, a)
#define ISongSection_CountSamples(p)                  COMCall0(p, CountSamples)
#define ISongSection_GetSample(p, a, b)               COMCall2(p, GetSample, a)
#define ISongSection_AddEvent(p, a)                   COMCall1(p, AddEvent, a)
#define ISongSection_CountEvents(p)                   COMCall0(p, CountEvents)
#define ISongSection_GetEvent(p, a, b)                COMCall2(p, GetEvent, a)
#define ISongSection_Release(p)                       COMCall0(p, Release)


// ISongSample
#undef INTERFACE
#define INTERFACE ISongSample

DECLARE_INTERFACE_(ISongSample, IUnknown)
{
   // Comtools macro to declare IUnknown methods
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, SetSampleInfo)(THIS_ sSongSampleInfo* pSampleInfo) PURE;
   STDMETHOD_(BOOL, GetSampleInfo)(THIS_ sSongSampleInfo* pSampleInfo) PURE;
};

#define ISongSample_SetSampleInfo(p, a)               COMCall1(p, SetSampleInfo, a)
#define ISongSample_GetSampleInfo(p, a)               COMCall1(p, GetSampleInfo, a)
#define ISongSample_Release(p)                        COMCall0(p, Release)


// ISongEvent
#undef INTERFACE
#define INTERFACE ISongEvent

DECLARE_INTERFACE_(ISongEvent, IUnknown)
{
   // Comtools macro to declare IUnknown methods
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, SetEventInfo)(THIS_ sSongEventInfo* pEventInfo) PURE;
   STDMETHOD_(BOOL, GetEventInfo)(THIS_ sSongEventInfo* pEventInfo) PURE;

   STDMETHOD_(BOOL, AddGoto)(THIS_ ISongGoto* pGoto) PURE;
   STDMETHOD_(unsigned, CountGotos)(THIS) PURE;
   STDMETHOD_(BOOL, GetGoto)(THIS_ unsigned index, ISongGoto** ppGoto) PURE;
};

#define ISongEvent_SetEventInfo(p, a)                 COMCall1(p, SetEventInfo, a)
#define ISongEvent_GetEventInfo(p, a)                 COMCall1(p, GetEventInfo, a)
#define ISongEvent_AddGoto(p, a)                      COMCall1(p, AddGoto, a)
#define ISongEvent_CountGotos(p)                      COMCall0(p, CountGotos)
#define ISongEvent_GetGoto(p, a, b)                   COMCall2(p, GetGoto, a, b)
#define ISongEvent_Release(p)                         COMCall0(p, Release)


// ISongGoto
#undef INTERFACE
#define INTERFACE ISongGoto

DECLARE_INTERFACE_(ISongGoto, IUnknown)
{
   // Comtools macro to declare IUnknown methods
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, SetGotoInfo)(THIS_ sSongGotoInfo* pGotoInfo) PURE;
   STDMETHOD_(BOOL, GetGotoInfo)(THIS_ sSongGotoInfo* pGotoInfo) PURE;
};

#define ISongGoto_SetGotoInfo(p, a)                   COMCall1(p, SetGotoInfo, a)
#define ISongGoto_GetGotoInfo(p, a)                   COMCall1(p, GetGotoInfo, a)
#define ISongGoto_Release(p)                          COMCall0(p, Release)


#ifdef __cplusplus
}
#endif

#endif
