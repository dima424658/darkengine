#include <datapath.h>
#include <gfile.h>
#include <string.h>

main()
{
   Datapath dp;
   GfileInfo gfi;

   DatapathClear(&dp);
   DatapathTryCurrent(&dp);

   GfileRead(&gfi, "test.pcx", &dp);
   GfileWrite(&gfi, "out.pcx", &dp);
   printf ("done now.\n");
}
