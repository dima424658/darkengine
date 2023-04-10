/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

#define MAX_LENGTH 40
#define ALL NULL

typedef int GenericID;

typedef struct lst_node{
   
   GenericID entry;
   char entry_text[MAX_LENGTH]; //needs to be provided/supported by the app
   struct lst_node *next;      //but for now just prints the ID number

} lst_node;

/*
typedef struct _search_params {
  lst_node *left_phylum;
  lst_node *left_object;
  lst_node *link_flavor;
  lst_node *right_phylum;
  lst_node *right_object;
} search_params;
 */

lst_node *InsertMenuEntry(GenericID new_entry,lst_node *start,int *total_entries);

lst_node *BuildMenuList(int which_menu);

DrawElement *BuildDrawElements(lst_node *start,int *num, int which_menu);
   //Takes a linked list of lst_nodes and builds up the drawelems for the menu

void UpdateSearchButtons(LGadMenu *current_menu);

bool ObjMenuCallback(int which_sel,LGadMenu *vm);

bool PhylumMenuCallback(int which_sel,LGadMenu *vm);

bool LinkMenuCallback(int which_sel,LGadMenu *vm);

bool ObjectButtonCallback(ushort action, void *data, LGadBox *vb);

bool CenterButtonCallback(ushort action, void *data, LGadBox *vb);

void CreateSearchButtons(LGadRoot *root, Rect *bounds);
