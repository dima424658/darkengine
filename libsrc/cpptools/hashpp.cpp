#include <stdlib.h>
#include <hashpp.h>

////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//

int cHashHelperFunctions::hashlog2(int x)
{
   if (x < 2) return 0;
   return 1+hashlog2(x/2);
}


int cHashHelperFunctions::expmod(int b, int e, uint m)
{
   if (e == 0) return 1;
   if (e%2 == 0)
   {
      int tmp = expmod(b,e/2,m);
      return (tmp*tmp)%m;
   }
   else
   {
      int tmp = expmod(b,e-1,m);
      return (b*tmp)%m;
   }

}



bool cHashHelperFunctions::is_fermat_prime(uint n, uint numtests)
{
   int i;
   if (n < 3) return FALSE;
   for (i = 0; i < numtests; i++)
   {
      int a = rand()%(n-2) + 2;
      if (expmod(a,n,n) != a) return FALSE;
   }
   return TRUE;
}

#define NUM_FERMAT_TESTS 30

bool cHashHelperFunctions::is_prime(uint n)
{
   // if N is sufficiently large, do fermat test.
   if (n > NUM_FERMAT_TESTS*NUM_FERMAT_TESTS)
      return is_fermat_prime(n,NUM_FERMAT_TESTS);
   else // do brute-force test
   {
      int i;
      for (i = 2; i*i <= n; i++)
         if ((n/i)*i == n)
            return FALSE;
   }
   return TRUE;
}


