#ifndef DARK_GEOMETRY_RANGES_HPP
#define DARK_GEOMETRY_RANGES_HPP
#include <algorithm>
#include <cstdint>
#include <vector>
namespace dark {
// Sorted, disjoint free ranges. Callers release only live allocations from this
// arena. Allocation splits preserve ordering; release joins only its neighbors.
struct GeometryRanges {
   struct Range {uint64_t offset,size;};
   std::vector<Range> free;
   bool take(uint64_t bytes,uint64_t alignment,uint64_t &offset) {
      if(!bytes || !alignment || (alignment&(alignment-1)))return false;
      for(size_t i=0;i<free.size();++i) {
         const auto range=free[i];const auto aligned=(range.offset+alignment-1)&~(alignment-1);
         if(aligned<range.offset || aligned-range.offset>range.size || bytes>range.size-(aligned-range.offset))continue;
         const auto prefix=aligned-range.offset,suffix=range.size-prefix-bytes;
         offset=aligned;
         if(prefix && suffix) {
            free[i]={range.offset,prefix};free.insert(free.begin()+i+1,{aligned+bytes,suffix});
         }else if(prefix)free[i]={range.offset,prefix};
         else if(suffix)free[i]={aligned+bytes,suffix};
         else free.erase(free.begin()+i);
         return true;
      }
      return false;
   }
   void release(uint64_t offset,uint64_t bytes) {
      if(!bytes)return;
      auto next=std::lower_bound(free.begin(),free.end(),offset,[](const Range &range,uint64_t value){return range.offset<value;});
      if(next!=free.begin()) {
         auto previous=next-1;
         if(previous->offset+previous->size==offset) {
            previous->size+=bytes;
            if(next!=free.end() && previous->offset+previous->size==next->offset) {
               previous->size+=next->size;free.erase(next);
            }
            return;
         }
      }
      if(next!=free.end() && offset+bytes==next->offset) {
         next->offset=offset;next->size+=bytes;return;
      }
      free.insert(next,{offset,bytes});
   }
};
}
#endif
