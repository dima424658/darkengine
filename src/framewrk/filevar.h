/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/filevar.h,v 1.4 1998/08/07 14:53:28 mahk Exp $
#pragma once  
#ifndef __FILEVAR_H
#define __FILEVAR_H

#include <tagfile.h>
#include <vernum.h>
#include <dlistsim.h>

////////////////////////////////////////////////////////////
// LIGHTWEIGHT "PERSISTENT VARIABLES" TOOL
//
// Say you have a block of global memory that you want saved and loaded with the 
// mission, gamesys, or campaign.  Just make a global of one of these classes, and 
// it will be saved and loaded automatically.
//
//

// Example: 
/*****************************************

// Here's the type of my global 
struct sFoo 
{
   int goof; 
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gTestVarDesc = 
{
   kMissionVar,         // Where do I get saved?
   "FVARTEST",          // Tag file tag
   "File Var Test",     // friendly name
   FILEVAR_TYPE(sFoo),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "dark",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sFoo,&gTestVarDesc> gFoo; 

//
// If I register an Sdesc, I'll be able to edit this using edit_file_vars
//

****************************************/

//
// Here's the list of which files we know how to save into
//

enum eWhichVarFile
{
   kMissionVar,  // I'm a "mission" variable
   kGameSysVar,  // I'm a "gamesys" variable 
   kCampaignVar, // I'm a "campaign" variable
   kNumVarFiles, 
};

//
// File Variable Descriptor
//

struct sFileVarDesc
{
   eWhichVarFile file;           // what kinds of files do I save in? 
   TagFileTag tag;               // The tag I save under
   const char* friendly_name;    // The name I use in the editor
   const char* type;             // My type (for sdesc lookup)
   TagVersion version;           // Current version
   TagVersion last_good_version; // last valid version 
   const char* game;             // what game am I valid for 
};

#define FILEVAR_TYPE(x) #x

//
// Struct containing all config & state for a persistent block 
// 

struct sFileVarData 
{
   void* mBlock; 
   int mSize; 
   const sFileVarDesc* mDesc;
};

//
// Self-threading, persistent file var class
//

class cFileVarBase;
typedef cSimpleDList<cFileVarBase*> cFileVarList; 


class cFileVarBase : public sFileVarData 
{

public: 
   cFileVarBase();  
   virtual ~cFileVarBase(); 

   virtual void Reset();  // clear to default value
   virtual void Update() {}; // we got loaded/edited 

   static cFileVarList* AllFileVars(); 

protected:
   
   BOOL SetupFile(ITagFile* file);
   void CleanupFile(ITagFile* file); 

public:
   void DatabaseMsg(ulong msg, ITagFile* file); 

}; 


//
// Templatized on structure type, type name, desc
//

template <class TYPE, const sFileVarDesc* DESC>
class cFileVar : public TYPE, public cFileVarBase
{
   TYPE& Var() { return *this; }; 
public:
   cFileVar() { mDesc = DESC; mSize = sizeof(TYPE), mBlock = &Var(); }

};


//
// Templatized on structure type, with constructor that builds desc and handles custom default value
//

template <class TYPE>
class cFileVar2 : public TYPE, public cFileVarBase, public sFileVarDesc
{
   TYPE& Var() { return *this; }; 
public:
   cFileVar2(eWhichVarFile init_file,
             const char *init_tag,
             const char* init_friendly_name,
             const char* init_type,
             uint init_version_major,
             uint init_version_minor,
             uint init_last_good_version_major,
             uint init_last_good_version_minor,
             const char* init_game,
             TYPE ** ppPublic = NULL,
             const TYPE * pDefVal = NULL)
   {
      strncpy(tag.label, init_tag, sizeof(tag.label) - 1);
      tag.label[sizeof(tag.label) - 1] = 0;
      file                    = init_file;
      friendly_name           = init_friendly_name;
      type                    = init_type;
      version.major           = init_version_major;
      version.minor           = init_version_minor;
      last_good_version.major = init_last_good_version_major;
      last_good_version.minor = init_last_good_version_minor;
      game                    = init_game;
      
      mDesc = this; 
      mSize = sizeof(TYPE), 
      mBlock = &Var(); 
      
      m_pDefVal = pDefVal;

      if (m_pDefVal)
         *((TYPE *)this) = *m_pDefVal;
      if (ppPublic)
         *ppPublic = ((TYPE *)this);
   }
   
   virtual void Reset()
   {
      if (m_pDefVal)
         *((TYPE *)this) = *m_pDefVal;
      else
         cFileVarBase::Reset();
   }

private:
   const TYPE * m_pDefVal;
   
};



#endif // __FILEVAR_H

