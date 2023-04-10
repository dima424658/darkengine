/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipsdscr.h,v 1.5 1998/09/10 12:14:17 TOML Exp $
//
//
//

#ifndef __AIPSDSCR_H
#define __AIPSDSCR_H

#pragma once
#pragma pack(4)

struct sAIPsdScrAct;

///////////////////////////////////////
//
// ENUM: eAIPsdScrAct -- stock acts for use in simple "pseudo-scripts"
//

enum eAIPsdScrAct
{                                // Args
   kAIPS_Nothing,
   kAIPS_Script,                 // Message text
   kAIPS_Play,                   // sound concept, sound tags, motion tags
   kAIPS_Alert,                  // Level
   kAIPS_BecomeHostile,
   kAIPS_EnableInvestigate,
   kAIPS_Goto,                   // Object name
   kAIPS_Frob,                   // Object name
   kAIPS_Wait,                   // time, motion tags, motion delay period
   kAIPS_Mprint,
   kAIPS_MetaProperty,           // Add/remove, metaproperty, object (optional)
   kAIPS_AddLink,                // Flavor, to object, from object (optional)
   kAIPS_RemoveLink,             // Flavor, to object, from object (optional)
   kAIPS_Face,                   // Angle, object
   kAIPS_Signal,                 // Signal, object list, max radius
   kAIPS_DestScript,             // Message text

   kAIPS_Num,
   
   kAIPS_TypeMax = 0xffffffff                    // force to 32-bits
};

///////////////////

EXTERN const char * g_ppszAIPsdScrActions[kAIPS_Num];

///////////////////////////////////////
//
// STRUCT: sAIPsdScrAct
//

#define kAIPSArgLen 64
#define kAIPSArgs   4

struct sAIPsdScrAct
{
   eAIPsdScrAct type;
   char         args[kAIPSArgs][kAIPSArgLen];
   
   const char * GetArg(unsigned n) const
   {
      if (args[n][0])  
         return args[n];
      return NULL;
   }
};

///////////////////////////////////////
//
// Macro for SDescs. Note, number of shown presently limited to 3
//

#define AIPSDSCR_ARG_FIELD(tagstr, structname, field) \
   { tagstr,          kFieldTypeEnum,   FieldLocation(structname, field.type), kFieldFlagNone,     0, FieldNames(g_ppszAIPsdScrActions) }, \
   { "   Argument 1", kFieldTypeString, FieldLocation(structname, field.args[0])                                                    }, \
   { "   Argument 2", kFieldTypeString, FieldLocation(structname, field.args[1])                                                    }, \
   { "   Argument 3", kFieldTypeString, FieldLocation(structname, field.args[2])                                                    }

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPSDSCR_H */
