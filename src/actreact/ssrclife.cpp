/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/ssrclife.cpp,v 1.5 1998/10/05 17:28:14 mahk Exp $
#include <ssrclife.h>
#include <sdesbase.h>
#include <iobjsys.h>
#include <stimbase.h>


// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
// PERIODIC LIFE CYCLE IMPL
//

//------------------------------------------------------------
// STRUCT DESC 
//

static const char* periodic_flags[] = 
{
  "No Max Firings" ,
  "Destroy Object on Completion"
}; 

#define NUM_PERIODIC_FLAGS (sizeof(periodic_flags)/sizeof(periodic_flags[0]))


static sFieldDesc periodic_fields[] = 
{
   { "Flags", kFieldTypeBits, FieldLocation(sPeriodicLifeCycle,flags), kFieldFlagNone, 0, NUM_PERIODIC_FLAGS, NUM_PERIODIC_FLAGS, periodic_flags }, 
   { "Period", kFieldTypeInt, FieldLocation(sPeriodicLifeCycle,period), kFieldFlagUnsigned },
   { "Max Firings", kFieldTypeInt, FieldLocation(sPeriodicLifeCycle,max_firings) }, 
   { "Intensity Slope", kFieldTypeFloat, FieldLocation(sPeriodicLifeCycle,slope) }, 
}; 

static sStructDesc periodic_sdesc = StructDescBuild(sPeriodicLifeCycle,kStructFlagNone,periodic_fields); 

const sStructDesc* sPeriodicLifeCycle::gpDesc = &periodic_sdesc; 

//------------------------------------------------------------
// Firing Iterator
//

struct sPeriodicLifeCycleFire
{
   // our life cycle 
   sPeriodicLifeCycle& life; 

   typedef tStimTimeStamp tTime; 

   // config variables
   tTime start; 
   tTime last;
   tTime now; 
   tStimLevel level; 

   // iteration variables
   tTime t; 
   int i; 

   int to_fire; 

   sPeriodicLifeCycleFire(sPeriodicLifeCycle* plife, BOOL initial, tStimLevel lv, tTime s, tTime l, tTime n)
      : start(s),last(l),now(n), i (0),life(*plife),level(lv)
   {

      // handle zero period case somewhat gracefully...
      if (life.period == 0)
      {
         Warning(("Stim source has zero-length period\n")); 
         life.period++; 
      }

      // convert period to freq so we don't have to divide twice
      // @NOTE: possible precision issue.
      float freq = 1.0/life.period; 

      if (last < start) last = start; 
      if (now < start) 
      {
         to_fire = 0; 
         t = start; 
         return; 
      }

      // how many times we've already fired.  
      int fired = (last - start) * freq; 
      to_fire = (now - start) * freq; 
      
      // special case: send an event at the first firing
      if (!initial)
         fired++; 
      to_fire ++; 

      if (!(life.flags & sPeriodicLifeCycle::kNoMaxFirings))
      {
         if (fired >= life.max_firings)
            to_fire = 0; // we're expired 
         else if (to_fire >= life.max_firings)
            to_fire = life.max_firings; 
      }

      // set up iteration 
      t = start + life.period * fired; 
      i = fired; 
      level += i * life.slope; 

   }

   BOOL Next(tStimLevel* plev, tTime* pt, ulong* flags)
   {
      *plev = level; 
      *pt = t; 

      if (i >= to_fire)
         return FALSE; 

      t += life.period; 
      level += life.slope; 
      i++;

      *flags = 0; 
      
      ulong destroy_mask = sPeriodicLifeCycle::kNoMaxFirings|sPeriodicLifeCycle::kDestroy; 

      // destroy set but not no max firings
      if ((life.flags & destroy_mask) == sPeriodicLifeCycle::kDestroy 
          && i == life.max_firings)
         *flags |= kStimDestroySrcObj; 

      return TRUE; 
   }
};

sPeriodicLifeCycle::sFire* sPeriodicLifeCycle::BeginFiring(BOOL initial, 
                                                           tStimLevel level, 
                                                           tStimTimeStamp start, 
                                                           tStimTimeStamp last,
                                                           tStimTimeStamp now)
{
   return new sFire(this,initial,level,start,last,now); 
}

BOOL sPeriodicLifeCycle::FireNext(sFire* fire, tStimLevel* nextlevel, tStimTimeStamp* nexttime, ulong* flags)
{
   Assert_(fire); 
   return fire->Next(nextlevel,nexttime,flags); 
}

BOOL sPeriodicLifeCycle::EndFiring(sFire* fire)
{
   BOOL retval = (flags & kNoMaxFirings) || fire->i < max_firings; 
   delete fire; 
   return retval; 
}

