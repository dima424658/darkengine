/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


#include <propface.h>
#include <propman.h>
#include <propbase.h>
#include <drkfixpr.h>

#include <dbmem.h> //must be last header

static IBoolProperty* gFixtureProp = NULL;

static sPropertyDesc fixture_prop_desc =
{
   "Fixture",
   0, // flags
   NULL, 0, 0, // constraints, version
   { "Object System", "Fixture" },
};

void init_fixture_prop()
{
   gFixtureProp = CreateBoolProperty(&fixture_prop_desc,kPropertyImplSparseHash);
}

void term_fixture_prop()
{
   SafeRelease(gFixtureProp);
}

BOOL is_fixture(ObjID obj)
{
  BOOL fixturetest = FALSE;

  if (!gFixtureProp)
    return FALSE;

  if (gFixtureProp->Get(obj,&fixturetest))
    return fixturetest;
  else
    return FALSE;
  
}
