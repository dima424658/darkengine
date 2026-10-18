// $Header: x:/prj/tech/libsrc/dispdev/RCS/cbchain.h 1.1 1997/10/21 16:15:44 KEVIN Exp $
// Generic callback chain structures and handler prototype

#ifndef __CBCHAIN_H
#define __CBCHAIN_H

typedef struct callback_chain_info callback_chain_info;

struct callback_chain_info {
   int message;
   int id;
   void (*func)(callback_chain_info *);
};

typedef void (callback_chain_func)(callback_chain_info *);

enum eCallbackChainMessage {
   kCallbackChainAddFunc,
   kCallbackChainRemoveFunc,
};

EXTERN void GenericCallbackChainHandler(int *, callback_chain_func **, callback_chain_info *);

#endif
