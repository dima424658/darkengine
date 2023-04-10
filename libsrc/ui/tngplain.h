// $Header: x:/prj/tech/libsrc/ui/RCS/tngplain.h 1.3 1998/06/18 13:30:49 JAEMZ Exp $

#ifndef __TNGPLAIN_H
#define __TNGPLAIN_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <tng.h>

// Typedefs
typedef struct {
   TNG *tng_data;
   Point size;
} TNG_plain;

// Prototypes

// Initializes the TNG 
EXTERN errtype tng_plain_init(void *ui_data, TNG *ptng, Point size);

// Deallocate all memory used by the TNG 
EXTERN errtype tng_plain_destroy(TNG *ptng);

// Fill in ppt with the size of the TNG 
EXTERN errtype tng_plain_size(TNG *ptng, Point *ppt);

// Returns the current "value" of the TNG
EXTERN int tng_plain_getvalue(TNG *ptng);

// Macros
#define TNG_PL(ptng) ((TNG_plain *)(ptng->type_data))

#endif // __TNGPLAIN_H

