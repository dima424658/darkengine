/*
 * $Source: x:/prj/tech/libsrc/md/RCS/mdd.h $
 * $Revision: 1.3 $
 * $Author: TOML $
 * $Date: 1996/10/21 09:35:56 $
 *
 * Debugging output routines for the Model library
 * These routines are restricted to a separate file,
 * debug.c, so as to allow it to all disappear in final
 * shipping versions of stuff.  These are in a separate
 * h file to cut down on recompiles.
 */


#ifndef __MDD_H
#define __MDD_H

#include <mds.h>

// String names of those enums in mds.h
extern char * mdd_sub_names[3];
extern char * mdd_mat_names[2];
extern char * mdd_pgon_names[4];

// These routines all use printf to
// do an ascii dump of this stuff

// dump a model header
void md_prn_header(mds_model *h);

// dump a subobj
void md_prn_subobj(mds_subobj *s);

// dump a material
void md_prn_mat(mds_mat *m);

// dump a light
void md_prn_light(mds_light *l);

// dump a polygon
// returns a pointer to the next pgon
mds_pgon *md_prn_pgon(mds_pgon *p);

// dump a sphere
void md_prn_sphere_info(mds_sphere *s);

// dump a node, returns pointer to the
// next node.
uchar *md_prn_node(uchar *n);

// The big one, dumps a whole model
void md_prn_model(mds_model *m);

#endif // __MDD_H
