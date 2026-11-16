// Compile with the engine's sdesc.cpp flags plus section garbage collection.
#include "../../libsrc/sdesc/sdesc.cpp"
#include <cstdlib>
#include <cstring>
int main() {
   char red[]="red",blue[]="blue";char *names[]={red,blue};
   sFieldDesc field{};field.size=sizeof(uint32_t);field.datasize=2;field.data=names;
   uint32_t bits=0;
   char one[]="red";parse_bits(&field,&bits,one);if(bits!=1)std::abort();
   char two[]="red,blue";parse_bits(&field,&bits,two);if(bits!=3)std::abort();
   char spaced[]="  blue, red";parse_bits(&field,&bits,spaced);if(bits!=3)std::abort();
   if(std::strcmp(one,"red") || std::strcmp(two,"red,blue") || std::strcmp(spaced,"  blue, red"))std::abort();
   std::puts("PASS named bit parsing preserves input and stops at the final terminator");
}
