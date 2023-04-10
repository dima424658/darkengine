/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/family.h,v 1.16 2000/01/29 13:38:46 adurant Exp $
// hack for family loading/palette switch until real dataflow
#pragma once

#ifndef __FAMILY_H
#define __FAMILY_H

///////////////////////////
// exposed calls to rest of world

EXTERN BOOL family_add(char *str);
EXTERN BOOL family_remove(char *str, BOOL synch);
EXTERN void family_clear_all(void);

///////////////////////////
// special water stuff
//

// Add water textures
EXTERN BOOL family_add_water(const char* prefix); 

// Find the handle ids of a given water texture set, adding them if necessary
EXTERN BOOL family_find_water(const char* prefix, int* handles, int nhandles); 

// Reload water based on render type
EXTERN void family_reload_water(void); 

// Is this a water texture?
EXTERN BOOL family_texture_is_water(int idx); 

// these control the family name blocks for disk read/write
// NOTE: block_build MALLOC's the void * it returns, you must free it
EXTERN void *family_name_block_build(int *cnt, int *size_per);
EXTERN BOOL family_name_block_parse(int cnt, int size_per, void *mem);

// startup/shutdown, init also sets up commands
EXTERN void family_init(void);
EXTERN void family_term(void);

///////////////////////////
// diskmemory swap block control
// integrated block load with the lresdisk block swizzle stuff
EXTERN BOOL familyDiskTexBlockLoad(void *mem);

// integrated block load with the lresdisk block swizzle stuff
// NOTE: this MALLOC's the void *, you MUST FREE IT
EXTERN void *familyDiskTexBlockBuild(void);

/////////////////
// compatibility for old bl file stuff!!
// returns the name for the current family
EXTERN char *family_which(void);
EXTERN void  family_load(char *family_name);
EXTERN char *sky_which(void);

/////////////////
// support for external modification
EXTERN BOOL family_free_single(int handle);

#endif  // __FAMILY_H



