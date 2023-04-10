/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/oracle.c,v 1.1 1996/10/21 21:43:00 buzzard Exp $

// Oracle: can answer any question in O(1) time.
//   It does this by cacheing the result from a previous frame

#include <lg.h>
#include <string.h>

int oracle_flags;
int oracle_save;
int oracle_last_memorized;

uchar *current_memorize;
uchar *last_memorize;
int current_playback;
int current_record, oracle_record_size;

// INVARIANT:   oracle_save != 0 iff current_memorize != 0

void OracleFinishQueues(void)
{
   if (current_memorize) {
      if (last_memorize)
         Free(last_memorize);
      last_memorize = current_memorize;
      oracle_last_memorized = oracle_save;

      current_memorize = 0;
      oracle_save = 0;
   }
}

void OracleStartFrame(int answer_flags, int memorize_flags)
{
   OracleFinishQueues();

   if (answer_flags == oracle_last_memorized) {
      oracle_flags = answer_flags;
      current_playback = 0;
   } else
      oracle_flags = 0;

   if (memorize_flags) {
      oracle_save = memorize_flags;
      oracle_record_size = 1024;
      current_memorize = Malloc(oracle_record_size);
      current_record = 0;
   }
}

// make the oracle try to answer exactly this set of flags
void OracleSmartStart(int answer_flags)
{
   if (answer_flags == oracle_last_memorized)
      OracleStartFrame(answer_flags, 0);
   else if (answer_flags == oracle_save)
      OracleStartFrame(answer_flags, 0);
   else
      OracleStartFrame(0, answer_flags);
}

bool OracleStore(void *data, int datasize)
{
   while (current_record + datasize >= oracle_record_size) {
      oracle_record_size *= 2;
      current_memorize = Realloc(current_memorize, oracle_record_size);
   }

   memcpy(current_memorize + current_record, data, datasize);
   current_record += datasize;
   return TRUE;
}

bool OracleFetch(int requeue, void *data, int datasize)
{
   memcpy(data, last_memorize + current_playback, datasize);
   current_playback += datasize;
   if (requeue)
      OracleStore(data, datasize);
   return TRUE;
}

bool OracleFetchBool(int requeue)
{
   bool result;
   OracleFetch(requeue, &result, sizeof(result));
   return result;
}

bool OracleStoreBool(bool result)
{
   OracleStore(&result, sizeof(result));
   return result;
}

