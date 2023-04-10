/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editlink.cpp,v 1.14 1998/02/04 13:53:26 dc Exp $
// Object link editor

#include <stdlib.h>

#include <editlink.h>
#include <edlinkst.h>

#include <sdesc.h>
#include <isdesced.h>
#include <isdescst.h>

#include <link.h>
#include <linkbase.h>
#include <linkman.h>
#include <lnkquery.h>
#include <linkid.h>

#include <relation.h>

#include <objsys.h>
#include <objedit.h>

#include <mprintf.h>

// for the windows controls
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>

#include <comtools.h>
#include <wappapi.h>
#include <appagg.h>

#include <dialogs.h>

// must be last header
#include <dbmem.h>

//
// Link Editor
//

struct LinkEditor
{
   LinkEditorDesc       editdesc;
   HWND                 hWnd;
   HWND                 hMainWnd;
   HINSTANCE            hInst;
   HWND                 hWndList;
   ObjID                objid_src;
   ObjID                objid_dest;
   RelationID           relid;
   HFONT                hWndFont;
};

// property IDs
#define ID_LINKED "LinkEditor: ed"

// control IDs
#define ID_ADDBUTTON 0
#define ID_DELETEBUTTON 1
#define ID_DONEBUTTON 2
#define ID_LISTVIEW 5

// END for the windows controls

#define MAX_STRLEN 32

// add a single line (LinkID, source objid, dest objid, flavor) to the list view

static LinkID get_item_linkid(LinkEditor* ed, int idx)
{
      LV_ITEM item;  
      item.iItem = idx;
      item.iSubItem = 0;
      item.mask = LVIF_PARAM;
      if (ListView_GetItem(ed->hWndList,&item))
         return (LinkID)item.lParam;
      else 
         return LINKID_NULL;
}

extern BOOL show_internal_links = FALSE;

BOOL LinkEditorShowAllLinks(BOOL newval)
{
   BOOL old = show_internal_links;
   show_internal_links = newval;
   return old;
}

BOOL linkedit_fill_line(HWND hWnd, int index, LinkID id, sLinkAndData* link)
{
   char buf[MAX_STRLEN];
   AutoAppIPtr_(LinkManager,LinkMan);
   IRelation* rel = LinkMan->GetRelation(link->flavor);
   const char* flavor = rel->Describe()->name;

   ListView_SetItemText(hWnd, index, 0, (char*)flavor);
   sprintf(buf, "%08X", id);
   ListView_SetItemText(hWnd, index, 1, buf);

   ListView_SetItemText(hWnd, index, 2, (char*)ObjEditName(link->source));
   ListView_SetItemText(hWnd, index, 3, (char*)ObjEditName(link->dest));
   if (link->data != NULL)
   {
      AutoAppIPtr_(StructDescTools,pTools); 
      const sRelationDataDesc* ddesc = rel->DescribeData();
      const sStructDesc* sdesc = pTools->Lookup(ddesc->type);
      if (!sdesc)
         sprintf(buf,"%08X",link->data);
      else
      {
         pTools->UnparseSimple(sdesc,link->data,buf,sizeof(buf));
      }
      ListView_SetItemText(hWnd, index, 4, buf);      
   }

   SafeRelease(rel);
   return TRUE;
}




void LGAPI edit_cb(sStructEditEvent* , StructEditCBData data);

static void linkedit_edit_data(LinkEditor* ed, int idx)
{
   LinkID id = get_item_linkid(ed,idx);
   if (id == LINKID_NULL) return;

   AutoAppIPtr_(LinkManager,LinkMan);
   void* data = LinkMan->GetData(id);



   IRelation* Rel = LinkMan->GetRelation(LINKID_RELATION(id));
   const sRelationDataDesc* ddesc = Rel->DescribeData();

   if (ddesc->size <= 0)
      return;

   if (data == NULL)
   {
      char* blank = new char[ddesc->size]; 
      memset(blank,0,ddesc->size);
      Rel->SetData(id,blank);
      data = Rel->GetData(id); 
      delete blank; 
   }

   AutoAppIPtr_(StructDescTools,pTools); 

   const sStructDesc* sdesc = pTools->Lookup(ddesc->type); 
   if (sdesc)
   {
      sStructEditorDesc editdesc = { "", kStructEditAllButtons};
      strcpy(editdesc.title,ddesc->type);
      IStructEditor* sed = CreateStructEditor(&editdesc,(sStructDesc*)sdesc,data);
      sed->SetCallback(edit_cb,(StructEditCBData)ed);
      sed->Go(kStructEdModeless);
      SafeRelease(sed);
   }

   SafeRelease(Rel);
}

int linkedit_add_line(HWND hWnd, int index, LinkID id, sLinkAndData* link)
{
   LV_ITEM item;


   item.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
   item.iItem = index;
   item.iSubItem = 0;
   item.state = 0;
   item.stateMask = 0;
   item.pszText = "";
   item.cchTextMax = MAX_STRLEN;
   item.lParam = id;
   ListView_InsertItem(hWnd, &item);
   linkedit_fill_line(hWnd,index,id,link);
   return index;

}


// start at item #start (0 means start at beginning of list)
int linkedit_find_selected(HWND hWnd, int start)
{
   if (ListView_GetSelectedCount(hWnd))
      for (int i=start; i<ListView_GetItemCount(hWnd); i++)
         if (ListView_GetItemState(hWnd, i, LVIS_SELECTED))
            return i;

   return -1;
}

// this is to be filled in by the AddLinkDlgProc
// this is static because there can be only one link add dialog at a time, and we
// can't attach a LinkEditor* to the dialog
static sLinkAndData dlg_add_link;

#pragma off(unreferenced)
// add link dialog DlgProc
BOOL CALLBACK AddLinkDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
      case WM_INITDIALOG:
         ILinkManager*        pLinkMan;
         const sRelationDesc* reldesc;
         sRelationIter        iter;
         RelationID           flavor;

         flavor = 0;
         pLinkMan = AppGetObj(ILinkManager);

         // go through the list of all flavors and add them to the dropdown box
         pLinkMan->IterStart(&iter);
         while (pLinkMan->IterNext(&iter, &flavor))
         {
            IRelation* pRel = pLinkMan->GetRelation(flavor);
            if (pRel)
               reldesc = pRel->Describe();
            if (show_internal_links || !(reldesc->flags & kRelationNoEdit))
            {
               int idx = SendDlgItemMessage(hDlg, IDC_FLAVORCOMBO, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)reldesc->name);
               SendDlgItemMessage(hDlg, IDC_FLAVORCOMBO, CB_SETITEMDATA, (WPARAM)idx, (LPARAM)pRel->GetID());

            }
            SafeRelease(pRel);
         }
         pLinkMan->IterStop(&iter);
         SafeRelease(pLinkMan);
         return TRUE;

      case WM_COMMAND:
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               // add the new link from the dialog
               char buf[16];

               GetDlgItemText(hDlg, IDC_FROMEDIT, buf, 15);
               dlg_add_link.source = EditGetObjNamed(buf);

               GetDlgItemText(hDlg, IDC_TOEDIT, buf, 15);
               dlg_add_link.dest = EditGetObjNamed(buf);

               dlg_add_link.data = NULL;
               int idx = SendDlgItemMessage(hDlg, IDC_FLAVORCOMBO, CB_GETCURSEL, 0, 0);
               int id = SendDlgItemMessage(hDlg, IDC_FLAVORCOMBO, CB_GETITEMDATA, (WPARAM)idx, 0);

               dlg_add_link.flavor = (RelationID)id;
               EndDialog(hDlg, TRUE);
            }
            return TRUE;

            case IDCANCEL:
               EndDialog(hDlg, FALSE);
               return FALSE;

            case IDC_DATAEDIT:
               MessageBox(hDlg, "edit data here", "Info", MB_OK | MB_ICONINFORMATION);
               break;
         }
         break;
   }

   return FALSE;
}
#pragma on(unreferenced)

// puts up the dialog to add a link and does it
void linkedit_add_link(LinkEditor* ed, HWND hWnd)
{
   sLinkAndData& link = dlg_add_link;

   memset(&link, 0, sizeof(sLinkAndData));

   if (DialogBox(ed->hInst, "AddLink", hWnd, AddLinkDlgProc))
   {
      AutoAppIPtr_(LinkManager,pLinkMan);
      const sRelationDesc* reldesc = NULL;

      if (link.flavor == 0)
      {
         MessageBeep(MB_ICONEXCLAMATION);
         MessageBox(hWnd, "Flavor is invalid!", "Add Link Error", MB_OK | MB_ICONEXCLAMATION);
         return;
      }

      // get the text description of the flavor
      pLinkMan = AppGetObj(ILinkManager);
      cAutoIPtr<IRelation> pRel (pLinkMan->GetRelation(link.flavor));
      if (pRel)
         reldesc = pRel->Describe();

      // can't link myself to myself
      if (link.source == link.dest)
      {
         MessageBeep(MB_ICONEXCLAMATION);
         MessageBox(hWnd, "Source and dest ObjID's can't be the same", "Add Link Error", MB_OK | MB_ICONEXCLAMATION);
         return;
      }
      // check for valid objids
      if (!ObjectExists(link.source))
      {
         char str[80];

         sprintf(str, "Source ObjID %d is invalid!\n", link.source);
         MessageBeep(MB_ICONEXCLAMATION);
         MessageBox(hWnd, str, "Add Link Error", MB_OK | MB_ICONEXCLAMATION);
         return;
      }
      if (!ObjectExists(link.dest))
      {
         char str[80];

         sprintf(str, "Dest ObjID %d is invalid!\n", link.dest);
         MessageBeep(MB_ICONEXCLAMATION);
         MessageBox(hWnd, str, "Add Link Error", MB_OK | MB_ICONEXCLAMATION);
         return;
      }

      LinkID id = pLinkMan->Add(link.source,link.dest,link.flavor);
      link.data = pLinkMan->GetData(id);
      int idx = linkedit_add_line(hWnd, ListView_GetItemCount(hWnd), id, &link);
      if (link.data != NULL)
         linkedit_edit_data(ed,idx);
   }
}

// called when the delete button is pressed and a link is selected
void linkedit_delete_link(LinkEditor* ed, HWND hWnd, int index)
{
   LinkID id = get_item_linkid(ed,index);
   if (id == LINKID_NULL) return;
   AutoAppIPtr_(LinkManager,LinkMan);
   LinkMan->Remove(id);
   ListView_DeleteItem(hWnd, index);
}

// finds all selected links and deletes them all
void linkedit_delete_selected(LinkEditor* ed, HWND hWnd)
{
   int num = linkedit_find_selected(hWnd, -1);
   if (num != -1)
   {
      MessageBeep(MB_ICONHAND);
      int result = MessageBox(hWnd, "Are you sure?", "Delete Link", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
      if (result == IDYES)
      {
         do
         {
            linkedit_delete_link(ed, hWnd, num);
         }
         while ((num = linkedit_find_selected(hWnd, num)) != -1);
      }
   }
}

// add all links for one objid to the list box
void linkedit_add_all_items(HWND hWnd, ObjID objid_src, ObjID objid_dest, RelationID relid)
{
   ILinkManager*        pLinkMan = AppGetObj(ILinkManager);
   ILinkQuery*          pQuery;
   int                  index = 0;

   // add the items which match the given query
   pQuery = pLinkMan->Query(objid_src, objid_dest, relid);
   for (; !pQuery->Done(); pQuery->Next())
   {
      sLinkAndData link;
      memset(&link,0,sizeof(link));

      LinkID id = pQuery->ID();
      IRelation* rel = pLinkMan->GetRelation(LINKID_RELATION(id));
      const sRelationDesc* desc = rel->Describe();
      if (!show_internal_links && (desc->flags & kRelationNoEdit))
      {
         SafeRelease(rel);
         continue;
      }
      SafeRelease(rel);
      pQuery->Link(&link);
      link.data = pQuery->Data();
      linkedit_add_line(hWnd, index++, id, &link);
   }
   SafeRelease(pQuery);
   SafeRelease(pLinkMan);
}

// WndProc for the link editor
LRESULT CALLBACK LinkEditorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   HWND parent;
   LinkEditor *ed;

   // get my parent so I can get the LinkEditor* from it
   parent = GetParent(hWnd);
   if (parent == NULL)
      parent = hWnd;
   ed = (LinkEditor*) GetProp(parent, ID_LINKED);
   
   switch(msg)
   {
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case ID_ADDBUTTON:
               linkedit_add_link(ed, ed->hWndList);
               break;

            case ID_DELETEBUTTON:
               linkedit_delete_selected(ed, ed->hWndList);
               break;

            case ID_DONEBUTTON:
               DestroyWindow(ed->hWnd);
               break;
         }

         // keep the list view active so we can see what's highlighted
         SetFocus(ed->hWndList);
         return 0;

      case WM_NOTIFY:
         NMHDR *hdr;
         hdr = (NMHDR*)lParam;
         if (hdr->code == NM_DBLCLK)
         {
            int idx = linkedit_find_selected(ed->hWndList, 0);
            if (idx != -1)
            {
               linkedit_edit_data(ed,idx);
            }
         }
         break;

      case WM_DESTROY:
         LinkEditorDestroy(ed);
         return 0;
   }

   return DefWindowProc(hWnd, msg, wParam, lParam);
}

static const char* relid_name(RelationID id)
{
   if (id == RELID_NULL) return "any";

   ILinkManager* linkman = AppGetObj(ILinkManager);
   IRelation* rel = linkman->GetRelation(id);   
   const char* out = rel->Describe()->name;
   SafeRelease(linkman);
   SafeRelease(rel);
   return out;
}

#define objid_name(obj) ((obj == LINKOBJ_WILDCARD) ? "any" : ObjEditName(obj))

static const char* default_title(ObjID src, ObjID dst, RelationID relid)
{
   static char buf[96];

   sprintf(buf,"Links: %s --[%s]--> %s",objid_name(src),relid_name(relid),objid_name(dst));

   return buf;
}

// create me a link editor for an objid
LinkEditor* LinkEditorCreate(ObjID objid_src, ObjID objid_dest, RelationID relid, LinkEditorDesc* editdesc)
{
   LinkEditor*          ed;
   const char*          szName = "Link Editor";
   WNDCLASS             wc;
   HWND                 hMainWnd;
   HINSTANCE            hMainInst;
   IWinApp*             pWA;
   LV_COLUMN            col;
   int                  width, height, left, top;

   ed = (LinkEditor*)Malloc(sizeof(LinkEditor));

   // we need the main app's HWND
   pWA = AppGetObj(IWinApp);
   hMainWnd = pWA->GetMainWnd();
   SafeRelease(pWA);

   // we also need the main app's HINSTANCE
   hMainInst = (HINSTANCE) GetWindowLong(hMainWnd, GWL_HINSTANCE);

   // set up the new window's class
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = LinkEditorWndProc;
   wc.hInstance = hMainInst;
   wc.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
   wc.lpszClassName = szName;

   // and register it
   RegisterClass(&wc);

   // make it so big
   width = 500;
   height = 400;
   left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
   top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

   //
   const char* title = editdesc->title;
   if (*title == '\0')
      title = default_title(objid_src,objid_dest,relid);

   // and create it!
   ed->hWnd = CreateWindow(szName, (char*)title, WS_OVERLAPPED, left, top, width, height,
                           hMainWnd, NULL, hMainInst, NULL);

   ShowWindow(ed->hWnd, SW_SHOWNORMAL);
   UpdateWindow(ed->hWnd);

   // Create a nice standard Windows font for the controls.
   LOGFONT logFont;
   memset(&logFont, 0, sizeof(LOGFONT));
   logFont.lfCharSet = DEFAULT_CHARSET;
   logFont.lfHeight = 12;
   strcpy(logFont.lfFaceName, "MS Sans Serif");
   ed->hWndFont = CreateFontIndirect(&logFont);

   // get our new window's HINSTANCE
   ed->hInst = (HINSTANCE) GetWindowLong(ed->hWnd, GWL_HINSTANCE);

   SetProp(ed->hWnd, ID_LINKED, (HANDLE)ed);
   ed->hMainWnd = hMainWnd;
   ed->objid_src = objid_src;
   ed->objid_dest = objid_dest;
   ed->relid = relid;

   // create some buttons
   HWND btnWnd;
   btnWnd = CreateWindow("BUTTON", "Add", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                60, 340, 65, 25, ed->hWnd, (HMENU) ID_ADDBUTTON, ed->hInst, NULL);
   if (ed->hWndFont)
      SendMessage(btnWnd, WM_SETFONT, (WPARAM)ed->hWndFont, 0);

   btnWnd = CreateWindow("BUTTON", "Delete", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                135, 340, 65, 25, ed->hWnd, (HMENU) ID_DELETEBUTTON, ed->hInst, NULL);
   if (ed->hWndFont)
      SendMessage(btnWnd, WM_SETFONT, (WPARAM)ed->hWndFont, 0);

   btnWnd = CreateWindow("BUTTON", "Done", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                375, 340, 65, 25, ed->hWnd, (HMENU) ID_DONEBUTTON, ed->hInst, NULL);
   if (ed->hWndFont)
      SendMessage(btnWnd, WM_SETFONT, (WPARAM)ed->hWndFont, 0);


   if (ed->hWndFont)
      SendMessage(btnWnd, WM_SETFONT, (WPARAM)ed->hWndFont, 0);

   // make it fill most of the window
   width = 500 - GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
   height = 330;
   left = 0;
   top = 0;

   // now, let's create the list view
   ed->hWndList = CreateWindowEx(0L, WC_LISTVIEW, "", WS_VISIBLE | WS_CHILD | LVS_REPORT, top,
                                 left, width, height, ed->hWnd, (HMENU) ID_LISTVIEW, ed->hInst, NULL);

   // set up the columns
   col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
   col.fmt = LVCFMT_LEFT;
   col.cx = 125;
   col.pszText = "Flavor";
   ListView_InsertColumn(ed->hWndList, 0, &col);
   col.cx = 75;
   col.pszText = "ID";
   ListView_InsertColumn(ed->hWndList, 1, &col);
   col.cx = 100;
   col.pszText = "From";
   ListView_InsertColumn(ed->hWndList, 2, &col);
   col.pszText = "To";
   ListView_InsertColumn(ed->hWndList, 3, &col);
   col.cx = 75;
   col.fmt = LVCFMT_RIGHT;
   col.pszText = "Data";
   ListView_InsertColumn(ed->hWndList, 4, &col);

   // insert all of the items
   linkedit_add_all_items(ed->hWndList, ed->objid_src, ed->objid_dest, ed->relid);

   return ed;
}

// destroy the link editor
void LinkEditorDestroy(LinkEditor* ed)
{
   if (ed->hWndFont)
   {
      DeleteObject(ed->hWndFont);
      ed->hWndFont = 0;
   }
   SetFocus(ed->hMainWnd);
   RemoveProp(ed->hWnd, ID_LINKED);
   Free(ed);
}

static void refresh_all_fields(LinkEditor* ed)
{
   AutoAppIPtr_(LinkManager,LinkMan);
   int max = ListView_GetItemCount(ed->hWndList);
   for (int idx = 0; idx < max; idx++)
   {
      LinkID id = get_item_linkid(ed,idx);
      if (id == LINKID_NULL)
         continue;
      sLinkAndData link;
      LinkMan->Get(id,&link);
      link.data = LinkMan->GetData(id);
      linkedit_fill_line(ed->hWndList,idx,id,&link);
   }
}

static void LGAPI edit_cb(sStructEditEvent* , StructEditCBData data)
{  
   LinkEditor* ed = (LinkEditor*)data;
   refresh_all_fields(ed);
}
