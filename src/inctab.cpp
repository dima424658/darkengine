/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/inctab.cpp,v 1.2 1997/10/14 11:20:18 TOML Exp $

#include <lg.h>
#include <hashpp.h>
#include <hshpptem.h>

#include <inctab.h>

// Must be last header
#include <dbmem.h>

typedef cHashTable<const char *, int, cHashFunctions> IntTab;
typedef cHashTable<const char *, float, cHashFunctions> FloatTab;

static IntTab *int_tab = NULL;
static FloatTab *float_tab = NULL;

void IncTabsInit()
{
   if (int_tab == NULL)
      int_tab = new IntTab;
   if (float_tab == NULL)
      float_tab = new FloatTab;
}

void IncTabsShutdown()
{
   if (int_tab != NULL)
   {
      delete int_tab;
      int_tab = NULL;
   }
   if (float_tab != NULL)
   {
      delete float_tab;
      float_tab = NULL;
   }
}

void IncTabAddInt(char *define, int val)
{
   int_tab->Set((const char*)define, val);
}

void IncTabAddFloat(char *define, float val)
{
   float_tab->Set((const char*)define, val);
}

BOOL IncTabLookupInt(char *define, int *val)
{
   return int_tab->Lookup((const char*)define, val);
}

BOOL IncTabLookupFloat(char *define, float *val)
{
   return float_tab->Lookup((const char*)define, val);
}

int IncTabLookup(char *define, void *val)
{
   if (IncTabLookupInt(define, (int*)val))
      return INC_TAB_INT;
   if (IncTabLookupFloat(define, (float*)val))
      return INC_TAB_FLOAT;
   return FALSE;
}


