/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motdesc.cpp,v 1.26 1998/04/20 15:52:33 kate Exp $

#include <wtypes.h>

#include <motdesc.h>
#include <string.h>
#include <motmngr.h>

#include <dbmem.h> // must be last included header

int MotDescNumMotions()
{
   if(g_pMotionSet)
   {
      return g_pMotionSet->NumMotions();
   } else
   {
      Warning(("Motion Database Not Loaded!\n"));
      return 0;
   }
}


BOOL MotDescNeckFixed(int mot_num)
{
   if(g_pMotionSet)
   {
      return g_pMotionSet->NeckIsFixed(mot_num);
   } else
   {
      Warning(("Motion Database Not Loaded!\n"));
      return FALSE;
   }
}

char *MotDescGetName(int mot_num)
{
   if(g_pMotionSet)
   {
      return (char *)g_pMotionSet->GetName(mot_num);
   } else
   {
      Warning(("Motion Database Not Loaded!\n"));
      return NULL;
   }
}

#define kMotDefaultBlendLength 500 // in msec

int MotDescBlendLength(int mot_num)
{
   if(g_pMotionSet)
   {
      return g_pMotionSet->BlendLength(mot_num);
   } else
   {
      Warning(("Motion Database Not Loaded!\n"));
      return 0;
   }
}

int MotDescNameGetNum(char *name)
{
   if(!name) // don't bother looking
      return -1;
   if(g_pMotionSet)
   {
      // make sure this cast is safe
      Assert_(strlen(name)<sizeof(Label));
      return g_pMotionSet->GetMotHandleFromName((Label *)name);
   } else
   {
      Warning(("Motion Database Not Loaded!\n"));
      return -1;
   }
}

