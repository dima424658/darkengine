/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/deepc/game/dpcgoalr.cpp,v 1.1 2000/01/26 19:06:33 porges Exp $
#include <dpcgoalr.h>
#include <drkuires.h>
#include <dpcmiss.h>

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
