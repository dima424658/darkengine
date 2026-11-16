#include "../../libsrc/lgd3d/geometry_ranges.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
static void check(bool pass){if(!pass){std::fputs("geometry range regression failed\n",stderr);std::abort();}}
int main() {
   dark::GeometryRanges ranges;ranges.free.push_back({0,4096});
   struct Live {uint64_t offset,size;};std::vector<Live> live;std::vector<bool> used(4096,false);
   uint32_t random=0x39de91u;auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
   for(unsigned step=0;step<20000;++step) {
      if(!live.empty() && next()%3==0) {
         const auto index=next()%live.size();const auto block=live[index];
         ranges.release(block.offset,block.size);
         for(uint64_t i=block.offset;i<block.offset+block.size;++i){check(used[i]);used[i]=false;}
         live.erase(live.begin()+index);
      }else {
         const uint64_t size=1+next()%128,alignment=next()%2?4:32;
         bool possible=false;
         for(uint64_t start=0;start+size<=used.size();start+=alignment) {
            bool empty=true;for(uint64_t i=start;i<start+size;++i)empty=empty && !used[i];
            if(empty){possible=true;break;}
         }
         uint64_t offset=0;const bool taken=ranges.take(size,alignment,offset);check(taken==possible);
         if(taken) {
            check(offset%alignment==0 && offset+size<=used.size());
            for(uint64_t i=offset;i<offset+size;++i){check(!used[i]);used[i]=true;}
            live.push_back({offset,size});
         }
      }
      for(size_t i=1;i<ranges.free.size();++i)check(ranges.free[i-1].offset+ranges.free[i-1].size<ranges.free[i].offset);
   }
   for(const auto block:live)ranges.release(block.offset,block.size);
   check(ranges.free.size()==1 && ranges.free[0].offset==0 && ranges.free[0].size==4096);
   uint64_t offset=99;check(ranges.take(4096,32,offset) && offset==0 && ranges.free.empty());
   check(!ranges.take(1,32,offset));ranges.release(0,4096);
   check(!ranges.take(1,0,offset) && !ranges.take(0,4,offset) && !ranges.take(1,3,offset));
   ranges.free={{std::numeric_limits<uint64_t>::max()-15,15}};
   check(!ranges.take(1,32,offset)); // Alignment rounding must not wrap to zero.
   std::puts("PASS fragmented mixed-alignment allocations, independent occupancy oracle, coalescing and overflow");
}
