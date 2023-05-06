/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/pickgame.cpp,v 1.8 1997/10/14 11:20:32 TOML Exp $
#include <string.h>

#include <pickgame.h>
#include <gametabl.h>

#include <dynarray.h>

#include <config.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// 
// GAME SELCTION
//
//

struct sFoundVar
{
   char name[80];

   sFoundVar(const char* s) { strcpy(name,s); };
};

typedef cDynArray<sFoundVar*> VarArray;

static const char* prefix = "";
static VarArray* itervars = NULL;

static bool iter_func(char* var)
{  
   if (strnicmp(var,prefix,strlen(prefix)) == 0)
   {
      sFoundVar* foundvar = new sFoundVar(var);
      itervars->Append(foundvar);
   }
   return FALSE;
}


// copy all vars of the form game_foo to foo

static void init_game_cfg_vars(const char* name)
{
   char prefixbuf[32];
   char valbuf[80];

   if (name == NULL) return;
   prefix = prefixbuf;
   sprintf(prefixbuf,"%s_",name);
   
   VarArray Vars;
   itervars = &Vars;
   // find all the vars
   config_get_raw_all(iter_func,valbuf,sizeof(valbuf));

   // now actually go through and set them 
   for (int i = 0; i < Vars.Size(); i++)
   {
      sFoundVar* var = Vars[i];
      char* newvar = var->name+strlen(prefix);

      config_get_raw(var->name,valbuf,sizeof(valbuf));
      config_set_string(newvar,valbuf);
      config_set_priority(newvar,PRIORITY_TRANSIENT);

      Vars[i] = NULL;
      delete var;
   }
   
   config_set_string((char*)name,"");
   config_set_priority((char*)name,PRIORITY_TRANSIENT);

}

void AppSelectGame(const char* name)
{
   const GameTableElem* game;
   init_game_cfg_vars(name);
   for (game = GameTable; game->name != NULL; game++)
   {
      if (name != NULL && strcmpi(name,game->name) == 0)
         break;
   }
   if (game->Create != NULL)
      game->Create();
}

