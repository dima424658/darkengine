/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprops.h,v 1.14 1999/03/29 08:41:47 JON Exp $
//
// Core AI properties, typically things shared between the AI manager and cAI
//
// @TBD (toml 04-21-98): this is going to turn into a dependency liability. May
// want to handle differently?
//
// AI property categories are:
//    "AI" - properties that either describe the gobal AI categorization/
//           calibration, and properties (often read only) that show the AI
//           state
//    "AI Attributes" - basically "stats"
//    "AI Ability Settings" - Properties for tweaking or showing state for
//                            individual AI abilities
//
// AI properties are:
//
// Category    Name        Global pointer       Purpose
// --------    ----        --------------       -------------------------------
// @TBD (toml 04-29-98): fill this out, generate doc


#ifndef __AIPROPS_H
#define __AIPROPS_H

#include <property.h>
#include <propface.h>
#include <aitype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// AI PROPERTY CATEGORIES
//

#define AI_CORE_CAT     "AI: AI Core"
#define AI_STATE_CAT    "AI: State"
#define AI_ATTRIB_CAT   "AI: Attributes"
#define AI_ABILITY_CAT  "AI: Ability Settings"
#define AI_UTILITY_CAT  "AI: Utility"
#define AI_RESPONSE_CAT "AI: Responses"
#define AI_CONVERSE_CAT "AI: Conversations"
#define AI_DEBUG_CAT    "AI: Debug"
#define AI_RANGED_CAT   "AI: Ranged Combat"

///////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION: AIGetProperty
//
// Template function gets the value for an object, or the default value
//

template <class IPROP, class PROP_TYPE>
inline
const PROP_TYPE AIGetProperty(IPROP * pProp, ObjID obj, PROP_TYPE defVal)
{
   PROP_TYPE result = defVal;
   pProp->Get(obj, &result);
   return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// Handy way to specifify nozeroing without having a seperate class for 
// every property 
//

#ifdef __DATAOPS__H

template <class TYPE> 
class cNoZeroDataOps : public cClassDataOps<TYPE>
{
public:
   cNoZeroDataOps()
    : cClassDataOps<TYPE>(cClassDataOps<TYPE>::kNoFlags)
   {
   }
}; 

#else

class cNoZeroDataOps;

#endif

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPROPS_H */
