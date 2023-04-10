/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/quesedit.cpp,v 1.2 2000/02/19 13:11:19 toml Exp $

#include <appagg.h>
#include <questapi.h>
#include <str.h>
#include <mprintf.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <command.h>
#include <simpwrap.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>

#include <questapi.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define IS_DELIM(x) ((x) == ',')

void QuestSet(char* pArg, eQuestDataType type)
{
   char *pName;
   int value = 0;
   char* pDelim;
   char save;
   
   while (*pArg && isspace(*pArg)) 
      pArg++;
   
   // get first delimeter
   pDelim = pArg; 
   while (*pDelim && !IS_DELIM(*pDelim))
      pDelim++;
   if (*pDelim)
   {
      save = *pDelim;
      *pDelim = '\0';
      pName = pArg;
      pArg = pDelim+1;
      if (*pArg)
         value = atoi(pArg);
      AutoAppIPtr(QuestData);
      pQuestData->Create(pName, value, type);
      *pDelim = save;
   }
   else
      mprintf("Usage: quest_set <name>, <value>\n");
}

void QuestSetCampCmd(char *pArg)
{
   QuestSet(pArg, kQuestDataCampaign);
}

void QuestSetMisCmd(char *pArg)
{
   QuestSet(pArg, kQuestDataMission);
}

void QuestGetCmd(char* pArg)
{
   while (*pArg && isspace(*pArg)) 
      pArg++;

   AutoAppIPtr(QuestData);
   int val = pQuestData->Get(pArg);
   mprintf("%s = %d\n", pArg, val);
}

void QuestDeleteCmd(char* pArg)
{
   while (*pArg && isspace(*pArg)) 
      pArg++;

   AutoAppIPtr(QuestData);
   pQuestData->Delete(pArg);
}

void QuestSpewListenCmd(int objID)
{
#ifdef DEBUG
   AutoAppIPtr(QuestData);
   pQuestData->ObjSpewListen(objID);
   mprintf("\n");
#endif
}

void QuestSubscribeCmd(char* pArg)
{
   char *pName;
   char* pDelim;
   char save;
   ObjID objID;
   
   while (*pArg && isspace(*pArg)) 
      pArg++;
   
   // get first delimeter
   pDelim = pArg; 
   while (*pDelim && !IS_DELIM(*pDelim))
      pDelim++;
   if (*pDelim)
   {
      save = *pDelim;
      *pDelim = '\0';
      objID = atoi(pArg);
      *pDelim = save;
      pName = pDelim+1;
      AutoAppIPtr(QuestData);
      pQuestData->SubscribeMsg(objID, pName);
   }
}

void QuestUnsubscribeCmd(char* pArg)
{
   char *pName;
   char* pDelim;
   char save;
   ObjID objID;
   
   while (*pArg && isspace(*pArg)) 
      pArg++;
   
   // get first delimeter
   pDelim = pArg; 
   while (*pDelim && !IS_DELIM(*pDelim))
      pDelim++;
   if (*pDelim)
   {
      save = *pDelim;
      *pDelim = '\0';
      objID = atoi(pArg);
      *pDelim = save;
      pName = pDelim+1;
      AutoAppIPtr(QuestData);
      pQuestData->UnsubscribeMsg(objID, pName);
   }
}

struct sQueryTron 
{
   char name[32];
   int value;
}; 

static sFieldDesc query_tron_fields[] = 
{
   { "Name", kFieldTypeString, FieldLocation(sQueryTron,name), },
   { "Value", kFieldTypeInt, FieldLocation(sQueryTron,value), },
};

static sStructDesc query_tron_desc = StructDescBuild(sQueryTron,kStructFlagNone,query_tron_fields); 

void QuestEdit(eQuestDataType type)
{
   AutoAppIPtr(QuestData);
   char **ppszMenu = (char **)Malloc(1);
   int iNumMenuSlots = 0;
   IQuestDataIter *pIter;

   for (pIter = pQuestData->Iter(type); !pIter->Done(); pIter->Next())
   {
      ppszMenu = (char **) Realloc(ppszMenu, (iNumMenuSlots+1)*sizeof(char*));
      ppszMenu[iNumMenuSlots] = (char*) Malloc(sizeof(char)*(strlen(pIter->Name())+12));
      sprintf(ppszMenu[iNumMenuSlots], "%s %10d", pIter->Name(), pIter->Value());
      ++iNumMenuSlots;
   }
   pIter->Release();
   if (iNumMenuSlots>0)
   {
      int iChoice = PickFromStringList("Quest Data", (const char* const*)ppszMenu, iNumMenuSlots);
      if (iChoice>=0)
      {
         char *pChar = ppszMenu[iChoice];
         while (*pChar != ' ')
            ++pChar;
         *pChar = '\0';
         int value = atoi(pChar+1);

         // Set up initial values
         sQueryTron tron;
         strncpy(tron.name, ppszMenu[iChoice], 31);
         tron.value = value;
         sStructEditorDesc editdesc = 
         {  
            "Quest Data", 
         };

         IStructEditor* ed = CreateStructEditor(&editdesc,&query_tron_desc,&tron);

         if (ed->Go(kStructEdModal))
            pQuestData->Create(tron.name, tron.value, type);
      }
   }
   for (int i = 0; i < iNumMenuSlots; ++i)
      Free(ppszMenu[i]);
   Free(ppszMenu);
}

void QuestEditCampCmd(void)
{
   QuestEdit(kQuestDataCampaign);
}

void QuestEditMisCmd(void)
{
   QuestEdit(kQuestDataMission);
}

static Command g_QuestCommands[] = 
{
   {"quest_create", FUNC_STRING, QuestSetCampCmd, "quest_set <name>,<value>", HK_EDITOR},
   {"quest_create_mis", FUNC_STRING, QuestSetMisCmd, "quest_set_mis <name>,<value>", HK_EDITOR},
   {"quest_get", FUNC_STRING, QuestGetCmd, "quest_get <name>", HK_EDITOR},
   {"quest_delete", FUNC_STRING, QuestDeleteCmd, "quest_delete <name>", HK_EDITOR},
   {"quest_spew_sub", FUNC_INT, QuestSpewListenCmd, "quest_spew_sub <objID>", HK_EDITOR},
   {"quest_sub", FUNC_STRING, QuestSubscribeCmd, "quest_sub <objID>,<name>", HK_EDITOR},
   {"quest_unsub", FUNC_STRING, QuestUnsubscribeCmd, "quest_unsub <objID>,<name>", HK_EDITOR},
   {"quest_edit", FUNC_VOID, QuestEditCampCmd, "quest_edit", HK_EDITOR},
   {"quest_edit_mis", FUNC_VOID, QuestEditMisCmd, "quest_edit_mis", HK_EDITOR},
};

void QuestEditInit(void)
{ 
   COMMANDS(g_QuestCommands, HK_ALL);
}
