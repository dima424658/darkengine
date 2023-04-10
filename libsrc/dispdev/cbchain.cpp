// $Header: x:/prj/tech/libsrc/dispdev/RCS/cbchain.cpp 1.1 1997/10/21 16:15:32 KEVIN Exp $
// Handle maintenance of a callback chain, i.e., adding and removing funcs from the chain.
// Each function in the chain should call this as necessary, passing pointers
// to it's id and the next function in the chain.

// the goal is to have an arbitrary length chain without having to dynamically
// allocate storage for the elements.  Instead, each function in the chain
// keeps track of the next internally and holds it's own id. The only drawback
// is that each function can appear in the chain only once.

// Note that an app can use a system (e.g., IWinDisplayDevice) that supports 
// callback chains and won't have to link this code unless it actually 
// implements one.


#include <lgassert.h>
#include <cbchain.h>

void GenericCallbackChainHandler(int *callback_id, void (**chain)(callback_chain_info *), callback_chain_info *info)
{
   switch (info->message) {
   case kCallbackChainRemoveFunc:
      {
         int id_save = info->id;
         if ( id_save == *callback_id ) {
            info->id = *callback_id = 0;
            info->func = *chain;
            *chain = NULL;
         } else {
            AssertMsg(*chain!=NULL, "Reached end of chain trying to remove func");
            // OK, so it's not for me; need to check if it's for my chain
            (*chain)(info);
            if ( info->id == 0 ) {
               info->id = id_save;
               *chain = info->func;
            }
         }
      }
      break;
   case kCallbackChainAddFunc:
      {
         if (info->func == NULL) {
            // This is my id, I'll grab it.
            AssertMsg(*callback_id==0, "Tried to add callback to chain twice!");
            *callback_id = info->id;
            *chain = NULL;
         } else {
            if (*chain == NULL) {
               // I'll add this guy to my chain and flag him to grab his id.
               *chain  = info->func;
               info->func = NULL;
            }
            (*chain)(info);
         }
      }
      break;
   }
}
