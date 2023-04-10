/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/family_.h,v 1.2 1998/07/12 14:13:10 JUSTIN Exp $
#pragma once  
#ifndef __FAMILY__H
#define __FAMILY__H

#include <resapi.h>

//////////////////
// types and such for the family controls
#define MAX_FAMILIES 16
#define FAM_NAME_LEN 24

typedef struct {
   char name[FAM_NAME_LEN];
   int load_id;
} fam_record;


EXTERN char       _fam_water_name[FAM_NAME_LEN];
EXTERN char       _fam_sky_name[FAM_NAME_LEN];

EXTERN fam_record cur_fams[MAX_FAMILIES];
EXTERN int        num_fams;

EXTERN IRes     * _fam_pal_hnd;

EXTERN fam_record *_fam_lookup_record(char *str);
EXTERN  void _fam_clear_records(void);
EXTERN void _fam_remove_all(void);
EXTERN void _fam_synch(BOOL remap); 


#endif // __FAMILY__H
