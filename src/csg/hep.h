/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef HEP_H
#define HEP_H

typedef struct {
   void *next;
   char *block;
   char *last;
   int left, size;
   int alloc, total;
} Hep;

extern void MoreHep(Hep *h);
extern void InitHep(Hep *h);
extern void MakeHep(Hep *h, int size);
extern void ResetHep(Hep *h);
extern void *HepAlloc(Hep *h);
extern void HepFree(Hep *h, void *p);
extern int HepSize(Hep *h);

#endif
