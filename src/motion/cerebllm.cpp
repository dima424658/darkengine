/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/cerebllm.cpp,v 1.5 2000/02/19 12:29:51 toml Exp $
//
// XXX This is kind of inelegant since whenever you want new kinds of things
// to be able to have cerebellums, they need to be explicitly added here, instead
// of setting a property or something at the place where they get defined, 
// but really I think this is sufficient.
//                                                          KJ 12/97

#include <cbllmapi.h>
#include <puppet.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

ICerebellum *CbllmGetFromObj(ObjID obj)
{
   ICerebellum *pCbllm;

   // next check if automaton
   if(NULL!=(pCbllm=PuppetGetCerebellum(obj)))
      return pCbllm;

   // next check if player

   return NULL;
}
