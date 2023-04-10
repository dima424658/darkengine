/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcsound.h>

#include <string.h>

#include <lg.h>

#include <str.h>

#include <ctag.h>
#include <ctagset.h>

#include <esnd.h>

#include <dbmem.h>

////////////////////////////////////////////////////////////
// ENVIRONMENTAL SOUND CALLBACK 
//

static cTag* find_tag_with_key(cTagSet* set,const cStr& key)
{
   if (set == NULL)
      return NULL; 

   for (int i = 0; i < set->Size(); i++)
   {
      const cTag* tag = set->GetEntry(i); 
      if (stricmp(tag->GetType(),key) == 0)
         return (cTag*)tag; 
   }
   return NULL; 
}

// finds the tags for the key and sets the second one alphabetically to 
// "key2" 
static void set_second_tag(cTagSet* s1, cTagSet* s2, const cStr& key)
{
   cTag* t1 = find_tag_with_key(s1,key); 
   cTag* t2 = find_tag_with_key(s2,key); 
   if (t1 && t2)
   {
      cStr key2 = key;
      key2 += "2"; 
      if (strcmp(t1->GetEnumValue(),t2->GetEnumValue()) < 0)
         t2->Set(key2, t2->GetEnumValue());
      else
         t1->Set(key2, t1->GetEnumValue());
   }
}

static void env_sound_CB(sESndEvent* ev)
{
   set_second_tag(ev->tagsets[kObj1MaterialTags],ev->tagsets[kObj2MaterialTags],"Material"); 
   set_second_tag(ev->tagsets[kObj1ClassTags],ev->tagsets[kObj2ClassTags],"WeaponType"); 
}

//////////////////////
// INIT/TERM OF THE DEEP COVER SOUND STUFF

void DPCSoundInit()
{
   ESndSetGameCallback(env_sound_CB);
}

void DPCSoundTerm()
{
   ESndSetGameCallback(NULL);    
}
