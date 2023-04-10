/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkgoalr.cpp,v 1.3 2000/02/19 12:27:15 toml Exp $
#include <drkgoalr.h>
#include <drkuires.h>
#include <drkmiss.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

cStr GoalStringTable()
{
   const sMissionData* data = GetMissionData();    
   cStr table = data->path; 
   if (table[0] != '\0')
      table+="/"; 
   table += "goals" ;
   return table; 
}

cStr GoalDescription(int i)
{
   char buf[64]; 
   sprintf(buf,"TEXT_%d",i); 
   return FetchUIString(GoalStringTable(),buf); 
}

cStr GoalFiction(int i)
{
   char buf[64]; 
   sprintf(buf,"FICTION_%d",i); 
   return FetchUIString(GoalStringTable(),buf); 
}

IDataSource* GoalStatus(int state)
{
   char buf[64]; 
   sprintf(buf,"goali%03d",state); 
   return FetchUIImage(buf); 
}
