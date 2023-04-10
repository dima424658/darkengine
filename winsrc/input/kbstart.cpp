/*
 * $Source: x:/prj/tech/winsrc/input/RCS/kbstart.cpp $
 * $Revision: 1.11 $
 * $Author: JAEMZ $
 * $Date: 1998/01/06 18:32:46 $
 *
 * Keyboard system startup routine.
 *
 * This file is part of the input library.
 */


#include <kb.h>

#include <defehand.h>
#include <appagg.h>
#include <inpcompo.h>

EXTERN bool kb_inited;

bool kb_inited=0;

int kb_startup(void *init_buf)
{
   if (kb_inited)
      return 0;
   kb_inited=FALSE;

   GetInputComponents();
   ConnectInput();

   return 0;
}
