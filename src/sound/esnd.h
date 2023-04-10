/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/esnd.h,v 1.5 2000/01/31 10:03:01 adurant Exp $
#pragma once

/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   environmental sound--C++ functions

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

#ifndef _ESND_H_
#define _ESND_H_

#include <matrixs.h>

#include <objtype.h>
#include <osystype.h>
#include <tagdbt.h>
#include <schbase.h>
#include <esndprop.h>
#include <ctagset.h>


extern void ESndNotifyTextParseEnd();

// This one plays a schema ambiently, as for a voiceover.
extern int ESndPlay(cTagSet *pTagSet, ObjID Obj1, ObjID Obj2, 
                    sSchemaCallParams *pParams = NULL, 
                    void *pData = NULL);

extern int ESndPlayObj(cTagSet *pTagSet, ObjID Obj1, ObjID Obj2, 
                       sSchemaCallParams *pParams = NULL, 
                       void *pData = NULL);

extern int ESndPlayLoc(cTagSet *pTagSet, ObjID Obj1, ObjID Obj2, 
                       const mxs_vector *pLoc, sSchemaCallParams *pParams = NULL, 
                       void *pData = NULL);


enum eESndTagSets
{
   kObj1ClassTags,
   kObj1MaterialTags,
   kObj2ClassTags,
   kObj2MaterialTags,
   kRoomTags,
   kAppTags, 
   kNumESndTagSets,
};

typedef struct sESndEvent 
{
   ObjID obj1;
   ObjID obj2;
   cTagSet* tagsets[kNumESndTagSets];
   sSchemaCallParams *params; 
} sESndEvent; 


typedef void (*tESndGameCallback)(sESndEvent* ev); 

tESndGameCallback ESndSetGameCallback(tESndGameCallback pfFunc);

#endif // ~_ESND_H_
