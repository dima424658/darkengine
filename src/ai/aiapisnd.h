/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapisnd.h,v 1.5 1998/10/21 22:20:38 TOML Exp $
//
// @Note (toml 05-19-98): this all has to be reexamined right after eee
//
//


#ifndef __AIAPISND_H
#define __AIAPISND_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <aiapicmp.h>

#pragma once
#pragma pack(4)

typedef unsigned eAISoundConcept;
class cTagSet;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAISoundEnactor
//

DECLARE_INTERFACE_(IAISoundEnactor, IAIComponent)
{

   STDMETHOD_(void, HaltCurrent)() PURE;

   //
   // Attempt to play a sound concept
   //
   STDMETHOD (RequestConcept)(eAISoundConcept, const cTagSet * bonusTags = NULL) PURE;
   STDMETHOD (RequestConcept)(const char * pszConcept, const cTagSet * bonusTags = NULL) PURE;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()
#endif /* !__AIAPISND_H */
