/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/sndscrpt.h,v 1.17 1999/03/30 21:26:40 Justin Exp $
//
// sndscrpt.h

#pragma once

#ifndef __SNDSCRPT_H
#define __SNDSCRPT_H

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>


///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

enum eSoundSpecial
{
   kSoundNormal,
   kSoundLoop,
   kSoundSpecialBig = 0xffffffff
};

enum eEnvSoundLoc
{
   kEnvSoundOnObj,
   kEnvSoundAtObjLoc,
   kEnvSoundAmbient, 
}; 

enum eSoundNetwork
{
   kSoundNetDefault,       // default: network spatials, but not ambients
   kSoundNetworkAmbient,   // ambient, but network it anyway
   kSoundNoNetworkSpatial, // spatial, but don't network it
};


///////////////////////////////////////
//
// Sound service
//
DECLARE_SCRIPT_SERVICE(Sound, 0xf1)
{
   // play a raw sound by name
   STDMETHOD_(boolean, Play)(object CallbackObject, 
                             const string ref SoundName,
                             eSoundSpecial Special = kSoundNormal,
                             eSoundNetwork Network = kSoundNetDefault) PURE;
   STDMETHOD_(boolean, Play)(object CallbackObject, 
                             const string ref SoundName, 
                             object TargetObject,
                             eSoundSpecial Special = kSoundNormal,
                             eSoundNetwork Network = kSoundNetDefault) PURE;
   STDMETHOD_(boolean, Play)(object CallbackObject, 
                             const string ref SoundName, 
                             vector ref Vector,
                             eSoundSpecial Special = kSoundNormal,
                             eSoundNetwork Network = kSoundNetDefault) PURE;
   STDMETHOD_(boolean, PlayAmbient)(object CallbackObject,
                                    const string ref SoundName,
                                    eSoundSpecial Special = kSoundNormal,
                                    eSoundNetwork Network = kSoundNetDefault)
      PURE;
                                    

   // play a schema by name
   STDMETHOD_(boolean, PlaySchema)(object CallbackObject, 
                                   object Schema,
                                   eSoundNetwork Network = kSoundNetDefault)
      PURE;
   STDMETHOD_(boolean, PlaySchema)(object CallbackObject, 
                                   object Schema, 
                                   object SourceObject,
                                   eSoundNetwork Network = kSoundNetDefault)
      PURE;
   STDMETHOD_(boolean, PlaySchema)(object CallbackObject, 
                                   object Schema, 
                                   vector ref Vector,
                                   eSoundNetwork Network = kSoundNetDefault)
      PURE;
   STDMETHOD_(boolean, PlaySchemaAmbient)(object CallbackObject,
                                          object Schema,
                                          eSoundNetwork Network = kSoundNetDefault)
      PURE;

   // dispatch an environemntal sound event
   // SourceObject defaults to CallbackObject
   // For callbacks, this will be treated like any other schema.
   STDMETHOD_(boolean, PlayEnvSchema)(object CallbackObject,
                                      const string ref Tags,
                                      object SourceObject = OBJ_NULL, 
                                      object AgentObject = OBJ_NULL, 
                                      eEnvSoundLoc loc = kEnvSoundOnObj,
                                      eSoundNetwork Network = kSoundNetDefault)
      PURE;



   // Play a "voice over," or possibly not if it's inappropriate 
   STDMETHOD_(boolean,PlayVoiceOver)(object cb_obj, 
                                    object Schema) PURE; 


   // We can halt all sounds on a given object, or get more specific,
   // requiring a given sound or schema name and even a callback
   // object.  The return values here are the number of sounds or
   // schemas which were halted.
   STDMETHOD_(integer, Halt)(object TargetObject,
                             const string ref SoundName = "",
                             object CallbackObject
                              = OBJ_NULL) PURE;

   STDMETHOD_(boolean, HaltSchema)(object TargetObject,
                                   const string ref SoundName = "",
                                   object CallbackObject
                                    = OBJ_NULL) PURE;

   STDMETHOD(HaltSpeech)(object speakerObj) PURE;

   STDMETHOD_(boolean,PreLoad)(const string ref SpeechName) PURE;
};


///////////////////////////////////////////////////////////////////////////////
//
// MESSAGES
//


///////////////////////////////////////
//
// MESSAGE: "SoundDone"
//

// Message parameter structure
#define kSoundDoneMsgVer 1

struct sSoundDoneMsg : public sScrMsg
{
   vector coordinates;
   ObjID targetObject;
   string name;

   sSoundDoneMsg()
   {
   }

   sSoundDoneMsg(ObjID to, ObjID PlayedOn, vector Coordinates, 
                 const char *Name)
    : sScrMsg(to, "SoundDone"),
      coordinates(Coordinates),
      targetObject(PlayedOn),
      name(Name)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};


///////////////////////////////////////
//
// MESSAGE: "SchemaDone"
//

// Message parameter structure
#define kSchemaDoneMsgVer 1


struct sSchemaDoneMsg : public sScrMsg
{
   vector coordinates;
   ObjID targetObject;
   string name;

   sSchemaDoneMsg()
   {
   }

   sSchemaDoneMsg(ObjID to, ObjID PlayedOn, vector Coordinates, 
                  const char *Name)
    : sScrMsg(to, "SchemaDone"),
      coordinates(Coordinates),
      targetObject(PlayedOn),
      name(Name)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};

#ifdef SCRIPT

#define OnSoundDone() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(SoundDone, \
                                                        sSoundDoneMsg)
#define DefaultOnSoundDone() SCRIPT_CALL_BASE(SoundDone)

#define OnSchemaDone() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(SchemaDone, \
                                                         sSchemaDoneMsg)
#define DefaultOnSchemaDone() SCRIPT_CALL_BASE(SchemaDone)

#endif

///////////////////////////////////////////////////////////////////////////////
//
// Base scripts
//

///////////////////////////////////////////////////////////////////////////////

#endif // __SNDSCRPT_H
