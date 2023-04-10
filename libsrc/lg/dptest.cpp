//////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/dptest.cpp $
// $Author: JUSTIN $
// $Date: 1998/05/01 14:34:34 $
// $Revision: 1.1 $
//
// This is a very simple, standalone regression test for Datapath. It was
// written during the addition of Context Paths to Datapath, to make sure
// nothing got horribly broken. It's not a fully proper regression test,
// since it doesn't produce a neat yes/no answer -- it's mainly intended
// to make it easy to examine the behaviour of Datapath.
//
// It looks for three files, named "test1.txt", "test2.txt", and "test3.txt".
// It seeks them in the relative directories "test" and "test/test_c/test_cc".
// It will also look on a path specified by the environment variable
// "DPTESTS". Fiddle with all of these values, and make sure that you get
// the results you expect.
//

#include <windows.h>
#include <comtools.h>
#include <lg.h>
#include <appagg.h>
#include <appapi.h>
#include <mprintf.h>

#include <datapath.h>

tResult LGAPI AppCreateObjects(int argc, const char *argv[])
{
   GenericApplicationCreate(argc, argv, "DatapathTest", NULL);
   // USE_EXTENDED_START_UP();

   return NOERROR;
}

void TryFiles (Datapath * dp) {
   // Try finding a couple of files
   // First, exercise DatapathOpen...
   FILE * file1 = DatapathOpen (dp, "test1.txt", "r");
   if (file1) {
      printf ("Opened test1.txt in %s (fully %s)\n", 
              DatapathLastPath (dp),
              DatapathLastContextPath (dp));
      fclose (file1);
   } else {
      printf ("Couldn't open test1.txt!\n");
   }

   // ... then, try out DatapathFDOpen...
   int fd2 = DatapathFDOpen (dp, "test2.txt", 0);
   if (fd2 >= 0) {
      printf ("Opened test2.txt in %s (fully %s)\n", 
              DatapathLastPath (dp),
              DatapathLastContextPath (dp));
      close (fd2);
   } else {
      printf ("Couldn't open test2.txt!\n");
   }

   // ... finally, try out DatapathFind.
   char buff[256];
   int ret = DatapathFind (dp, "test3.txt", buff, 256);
   if (ret) {
      printf ("Opened %s\n", buff);
   } else {
      printf ("Couldn't open test3.txt!\n");
   }
}

int LGAPI AppMain(int /* argc*/ , const char * /* argv */ [])
{
   Datapath dp;
   Datapath context;

   DatapathClear (&dp);

   // First, try just the current directory:
   DatapathTryCurrent (&dp);
   printf ("Looking in just the current directory.\n");
   TryFiles (&dp);

   // Now, only look on the path:
   DatapathNoCurrent (&dp);
   printf ("Looking on just the path with context.\n");
   // First, put in a few hardcoded directories:
   DatapathAdd (&dp, "test;test\\test_c\\test_cc;test\\test_c");
   // Now set up a context from the environment:
   DatapathClear(&context);
   DatapathAddEnv (&context, "DPTESTS");
   DatapathSetContext (&dp, &context);
   TryFiles (&dp);

   // Now, try using the current directory:
   DatapathTryCurrent (&dp);
   printf ("Looking in current directory and path with context.\n");
   TryFiles (&dp);

   // Try copying the path, and using the copy:
   Datapath dp2;
   DatapathClear(&dp2);
   DatapathCopy (&dp2, &dp);
   printf ("Looking in current directory and copied path with context.\n");
   TryFiles (&dp2);

   // Try clearing the context in the copy:
   DatapathSetContext (&dp2, NULL);
   printf ("Looking in current directory and copied path without context.\n");
   TryFiles (&dp2);

   // Get the full list of files in the path directories:
   printf("\nGetting contents of the directories without context or dots:\n");
   DatapathDir * pdpd = DatapathOpenDir (&dp2, "*.*", DP_SCREEN_DOT);
   char * curfile;
   char buf[256];
   do {
      curfile = DatapathReadDir (pdpd);
      if (curfile) {
         DatapathDirGetPath (pdpd, buf);
         printf ("Got %s as %s\n", curfile, buf);
      }
   } while (curfile);
   DatapathCloseDir (pdpd);

   // Get the full list of files in the path directories:
   printf("\nGetting full contents of the directories with context:\n");
   pdpd = DatapathOpenDir (&dp, "*.*", 0);
   do {
      curfile = DatapathReadDir (pdpd);
      if (curfile) {
         DatapathDirGetPath (pdpd, buf);
         printf ("Got %s as %s\n", curfile, buf);
      }
   } while (curfile);
   DatapathCloseDir (pdpd);

   // Get the full list of files in the path directories:
   printf("\nGetting full contents of the null Datapath:\n");
   DatapathFree(&dp);
   pdpd = DatapathOpenDir (&dp, "*.*", 0);
   do {
      curfile = DatapathReadDir (pdpd);
      if (curfile) {
         DatapathDirGetPath (pdpd, buf);
         printf ("Got %s as %s\n", curfile, buf);
      }
   } while (curfile);
   DatapathCloseDir (pdpd);

   return 0;
}

