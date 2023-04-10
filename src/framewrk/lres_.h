/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lres_.h,v 1.2 2000/01/29 13:21:17 adurant Exp $
// internal header for lres internal code files
#pragma once

#ifndef __LRES__H
#define __LRES__H

#include <lresname.h>
#include <lrestok.h>
#include <lress.h>

// actual thing which does the single load, looping through all wheres
// mostly modularized out for use by lresdisk and other stuff
rn_hnd _ResNameLoadSingle(char *name, char *tokens, char *base_p, int types, RN_ScanCallback cback);

// takes a rn_name with tokenized path, generates data for scan single
// for now, this basically detokenizes, then RawLoad retokenizes
// clearly stupid, but it works
// the problem is base_p is needed for the diskpathMunge, so it has to be built
rn_hnd _ResNameLoadRnName(rn_name *rndat, int types, RN_ScanCallback cback);

#endif  //  __LRES__H
