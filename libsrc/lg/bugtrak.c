#define __BUGTRAK_SRC

#include "bugtrak.h"
#include "lgsprntf.h"
#include "dbg.h"
#ifdef __WATCOMC__
#include "env.h"
#else
#include <stdlib.h>
#include <sys\stat.h>
#endif
#include "time.h"
#include "array.h"
#include <fcntl.h>
#include <io.h>
#include <string.h>

errtype bugtrak_report_registration(int id);
errtype bugtrak_begin_session(char *bt_session_file);
errtype bugtrak_end_session();

BugFunc bug_report_cb;

// Initializes the bugtrak system, sets up the session file, checks all the environment variables, etc.
errtype bugtrak_init(int bt_max_systems, char *fname, BugFunc bug_report_callback)
{
   int num = 0, numlen = 1;
   int fp, i;
   bool freeit = FALSE;
   char *subname;

   bug_report_cb = bug_report_callback;

   bt_max_sys = bt_max_systems;
   array_init(&bt_short_table, sizeof(BugFunc), bt_max_systems);
   array_init(&bt_long_table, sizeof(BugFunc), bt_max_systems);
   bt_index_table = (int *)Malloc(bt_max_sys * sizeof(int));
   bt_num_reg = 0;

   if (fname == NULL)
   {
      fname = (char *)Malloc(12 * sizeof(char));
      freeit = TRUE;
      strcpy(fname, "btses000.txt");
   }

   // Zip around until we find an open filename to use...
   subname = strrchr(fname,'0');
//   mprintf("fname = %s  subname = %s\n",fname,subname);
   while ((strlen(subname) != strlen(fname)) && (subname[0] == subname[-1]))
   {
      subname--;
      numlen++;
   }
//   mprintf ("After while: fname = %s subname = %s numlen = %d\n",fname,subname,numlen);
	while ( (fp = open(fname,O_BINARY|O_RDONLY)) != -1) {
		/* Check next slot */
      close(fp);                       /* good idea to, like, close the opened file */
 		++num;
      for (i=0; i<numlen; i++)
         subname[numlen - (i+1)] = '0' + ((num>>(3*i))&7);
//         fname[numbase-numlen] = '0'+((num>>(3*i))&7);
//  		fname[7] = '0'+(num&7);
//  		fname[6] = '0'+((num>>3)&7);
//  		fname[5] = '0'+((num>>6)&7);
  	}
//   mprintf("After loop: fname = %s subname = %s\n---\n",fname,subname);
   bugtrak_begin_session(fname);
   if (freeit)
      Free(fname);
   return(OK);
}

// Registers a function with the bugtrak system.  short_proc or long_proc will get called when the bugtrak needs
// to generate system information.  id will get set to the bugtracking id used to just call that one system's
// bugtrak functions.
errtype bugtrak_register(BugFunc short_proc, BugFunc long_proc, int *id)
{
   int index;
   array_newelem(&bt_short_table, &index);
   ((BugFunc *)(bt_short_table.vec))[index] = short_proc;
   array_newelem(&bt_long_table, &index);
   ((BugFunc *)(bt_long_table.vec))[index] = long_proc;
   bt_index_table[bt_num_reg] = index;
   *id = bt_num_reg;
   bt_num_reg++;

   return(bugtrak_report_registration(index));
}

// Generates a report for that bugtracking system identified by id, of length determined by length (long or short)
// by calling the registered function for that system.  Note that most of the work needs to actually be done by 
// the functions that the bugtracking library calls.
errtype bugtrak_generate(int id, int length)
{
   errtype retval = OK;
   if (length == LONG_FUNC)
      retval = ((BugFunc *)(bt_long_table.vec))[id]();
   if ((length == SHORT_FUNC) && (retval == OK))
      retval = ((BugFunc *)(bt_short_table.vec))[id]();
   return(retval);
}

// Like bugtrak_generate, only gets called for all registered systems.
errtype bugtrak_generate_all(int length)
{
   int i;
   errtype retval = OK;
   for (i=0; (i<bt_num_reg) && (retval == OK); i++)
   {
      retval = bugtrak_generate(bt_index_table[i], length);
   }
   return(retval);
}

// Shuts down the bugtrak system, deallocates memory, and indicates the end of the session in the
// session file.
errtype bugtrak_shutdown(int status_code)
{
   bugtrak_end_session(status_code);
   Free(bt_index_table);
   array_destroy(&bt_long_table);
   array_destroy(&bt_short_table);
   return(OK);
}

// Lets the user generate a bug report from inside the program.  Details to be provided later, but it will probably
// pop up a window with some fields for the user to fill out then will write that to a special file and then
// call all the registered systems.
errtype bugtrak_file_report(void)
{
   if (bug_report_cb != NULL)
      bug_report_cb();
   return(OK);
}

errtype bugtrak_report_registration(int id)
{
   char oput[50];

   lg_sprintf(oput, "REGISTERING FUNCTIONS TO ID %d\n",id);
   write(bt_session_handle, oput, strlen(oput) * sizeof(char));
   return(OK);
}

errtype bugtrak_begin_session(char *bt_session_file)
{
   char oput[50];
   time_t t;

   if ((bt_session_handle=open(bt_session_file,O_CREAT|O_WRONLY|O_TRUNC, S_IWRITE))==-1) {
      Warning(("ARGH!!  OPEN FAILURE!!! %s %d\n",bt_session_file,bt_session_handle));
      return(ERR_NODEV);
   }
   lg_sprintf(oput, "NAME: %s\n", getenv("USER"));
   write(bt_session_handle, oput, strlen(oput) * sizeof(char));
   time(&t);
   lg_sprintf(oput, "STARTING SESSION AT: %s\n", ctime(&t));
   write(bt_session_handle, oput, strlen(oput) * sizeof(char));
   return(OK);
}

errtype bugtrak_end_session(int status_code)
{
   char oput[50];
   time_t t;

   time(&t);
   lg_sprintf(oput, "ENDING SESSION AT: %s\n", ctime(&t));
   write(bt_session_handle, oput, strlen(oput) * sizeof(char));
   lg_sprintf(oput, "STATUS = %d\n", status_code);
   write(bt_session_handle, oput, strlen(oput) * sizeof(char));
   return(OK);
}
