/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objtool.cpp,v 1.4 1999/05/23 01:33:00 mahk Exp $
#include <iobjsys.h>
#include <string.h>
#include <ctype.h>
#include <comtools.h>
#include <appagg.h>
#include <traitman.h>
#include <label.h>
#include <stdlib.h>
#include <linktype.h>
#include <objedit.h>

// Must be last header 
#include <dbmem.h>


// 
// "OBJECT TOOLS"
// Refugees from objedit that were needed for stand-alone game. 
//

#define UNKNOWN_NAME "UNKNOWN"


static const char* indef_article(const char* word)
{
   static char vowels[] = "AEIOU";

   if (strchr(vowels,toupper(word[0])) != NULL)
      return "An";
   else
      return "A";
}

const const char*  ObjEditName(ObjID obj)
{
   static char namebuf[1024];

   if (obj == OBJ_NULL)
      return "None (0)";

   AutoAppIPtr_(ObjectSystem,objsys); 

   const char* name = UNKNOWN_NAME; 
   if (objsys->Exists(obj))
      name = objsys->GetName(obj);

   if (name == NULL)
   {
      AutoAppIPtr_(TraitManager,traitman); 

      ObjID arch = traitman->GetArchetype(obj);
      name = objsys->GetName(arch);

      if (name != NULL)
         sprintf(namebuf,"%s %s",indef_article(name),name);
      else
         strcpy(namebuf,UNKNOWN_NAME);

   }
   else
      strcpy(namebuf,name);

   // append the ObjID
   sprintf(namebuf+strlen(namebuf)," (%d)",obj);

   return namebuf;
}

////////////////////////////////////////

static char wildcard[] = "*";

ObjID EditGetObjNamed(const char* name)
{
   // try integer id
   ObjID obj = atoi(name); 
   if (obj != OBJ_NULL) return obj;

   // try object name
   AutoAppIPtr_(ObjectSystem,ObjSys);
   obj = ObjSys->GetObjectNamed(name);
   if (obj != OBJ_NULL) return obj;

   // try wildcard
   if (strncmp(name,wildcard,strlen(wildcard)) == 0)
      return LINKOBJ_WILDCARD;

   // look for (id)
   const char* r = strchr(name,'(');
   if (r != NULL) 
   {
      r++;
      obj = atoi(r);
      if (ObjSys->Exists(obj))
         return obj;
   }

   // give up
   return OBJ_NULL;
}
