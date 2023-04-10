/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/textarch.h,v 1.5 1998/10/31 07:17:26 mahk Exp $
#pragma once  
#ifndef __TEXTARCH_H
#define __TEXTARCH_H
#include <objtype.h>
#include <comtools.h>

F_DECLARE_INTERFACE(IRes); 

////////////////////////////////////////////////////////////
// Texture archetype system
//

//
// Clear the mapping from textures to archetypes

EXTERN void ClearTextureArchetypeMapping(void);

//
// builds and maps texture archetypes for all textures.
//
EXTERN void BuildTextureArchetypes(void); 
// Finds non-existent texture archetypes and rebuilds those
EXTERN void RebuildTextureArchetypes(void); 
// Build one terrain texture archetype
EXTERN ObjID BuildOneTextureArchetype(int idx); 
// Forcibly rebuild one texture archetype
EXTERN ObjID ForciblyRebuildOneTextureArchetype(int idx); 

//
// Given a texture index, find its representative
//
EXTERN ObjID GetTextureObj(int idx); 

//
// Given a texture name, find its representative
// This is the only way to find an object texture rep
//
EXTERN ObjID GetTextureObjNamed(const char* respath, const char* resname);

//
// Get a texture obj by IRes 
//
EXTERN ObjID GetResTextureObj(IRes* res); 

//
// Given an ObjID, return its texture name
//
EXTERN const char* GetTextureObjName(ObjID obj); 

//
// Test to see if an object is a texture representative.
//
EXTERN BOOL IsTextureObj(ObjID obj); 


//
// Given an representative, get it's texture index
// < 0 for none
// 

EXTERN int GetObjTextureIdx(ObjID obj);

//
// Init/Term
//
EXTERN void TextureArchetypesInit(void); 
EXTERN void TextureArchetypesTerm(void); 

EXTERN void TextureArchetypesPostLoad(void); 




#endif // __TEXTARCH_H


