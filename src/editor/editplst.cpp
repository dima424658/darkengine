/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editplst.cpp,v 1.14 1998/03/14 21:07:57 mahk Exp $

// Property List Editor

#include <editplst.h>
#include <editprop.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <property.h>
#include <propbase.h>
#include <propman.h>
#include <propraw.h>

#include <appagg.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>
#include <iobjsys.h>
#include <objedit.h>
#include <donorq.h>
#include <label.h>


// for the windows controls
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>

#include <comtools.h>
#include <wappapi.h>
#include <appagg.h>
#include <stdlib.H>

// must be last header
#include <dbmem.h>

// control IDs
#define ID_VIEWBUTTON 0
#define ID_DELETEBUTTON 1
#define ID_DONEBUTTON 2
#define ID_PROPBOX 3
#define ID_METABOX 4

#define NUM_BUTTONS EDPLIST_NUM_BUTTONS

#define ID_PLISTED "PropertyListEditor: ed"
// END for the windows controls

static const char* button_names[] = { "Add", "Delete", "Done" };

// Dimensions

#define BUTTON_W (65 +GetSystemMetrics(SM_CXBORDER)*2)
#define BUTTON_H (25 +GetSystemMetrics(SM_CYBORDER)*2)


#define PROPBOX_W 200
#define PROPBOX_H 200

#define METABOX_W 200
#define METABOX_H 100

#define MARGIN_X 5
#define MARGIN_Y 5

#define BUTTONS_W (NUM_BUTTONS*BUTTON_W + (NUM_BUTTONS-1)*MARGIN_X)

#define WINDOW_W  (2*MARGIN_X +BUTTONS_W)
#define WINDOW_H  (MARGIN_Y+METABOX_H+MARGIN_Y+PROPBOX_H+MARGIN_Y+BUTTON_H+MARGIN_Y)

//----------------
// GLOBALS
//----------------

static int num_editors = 0;  // number of editors currently out there.

//----------------
// Prototypes
//----------------
void BuildUI(PropertyListEditor* ed, PropertyListEditorDesc* editdesc);
void PropertyListEditorDestroy(PropertyListEditor* ed);

// these flags tell PropertyListEditorFillBox which properties to display
#define kIrrelevant 0
#define kRelevant 1

static char* view_button_text[] = { "Cancel", "Add" }; 


//------------------------------------------------------------
// General ops
//

void PropertyListEditorFillBox(PropertyListEditor *ed, int mode);
void PropertyListEditorFillMetaBox(PropertyListEditor *ed, int mode);


static void change_edit_mode(PropertyListEditor* ed, int mode)
{
   if (mode == ed->mode)
      return; 

   ed->mode = mode;
   SetWindowText(ed->hWndButtons[ID_VIEWBUTTON],view_button_text[mode]);
   EnableWindow(ed->hWndButtons[ID_DELETEBUTTON], mode == kRelevant);
 
   PropertyListEditorFillBox(ed, mode);
   PropertyListEditorFillMetaBox(ed, mode);
}


//------------------------------------------------------------
// THE PROPERTY LIST BOX
//

//
// Return the string for an item, concats together string with value
//

static char* prop_item_string(ObjID obj, IProperty* prop)
{
   static char buf[80];
   sprintf(buf,"%s",prop->Describe()->name);
   
   if (!prop->IsSimplyRelevant(obj))
      return buf;

   AutoAppIPtr_(StructDescTools,pTools); 
   const sPropertyTypeDesc* tdesc = prop->DescribeType();
   const sStructDesc* sdesc = pTools->Lookup(tdesc->type); 
   if (tdesc->size > 0 && sdesc )
   {
      IPropertyRaw* raw;
      if (FAILED(COMQueryInterface(prop,IID_IPropertyRaw,(void**)&raw)))
         return buf;

      // grab the data
      char* val = new char[tdesc->size];
      raw->Get(obj,val);
      
      // add a colon and the value
      strcat(buf,": ");
      pTools->UnparseSimple(sdesc,val,buf+strlen(buf),sizeof(buf)-strlen(buf)); 
      delete val;
   }

   return buf;
}

extern "C"
{
   BOOL property_show_all = FALSE;
}

// puts all the wanted properties in a listbox
void PropertyListEditorFillBox(PropertyListEditor *ed, int mode)
{
   IPropertyManager* propMan = AppGetObj(IPropertyManager);
   sPropertyIter iter;
   IProperty* prop;

   // clear out the listbox
   SendMessage(ed->hWndPropBox, LB_RESETCONTENT, 0, 0);

   // go through all properties and put them in the list box
   propMan->BeginIter(&iter);
   while ((prop = propMan->NextIter(&iter)) != NULL)
   {
      if (!property_show_all && (prop->Describe()->flags & kPropertyNoEdit))
         continue;

      BOOL isIrrel = mode==kIrrelevant;
      if (isIrrel == !prop->IsSimplyRelevant(ed->editID))
      {
         SendMessage(ed->hWndPropBox, LB_ADDSTRING, 0, (LPARAM) prop_item_string(ed->editID,prop));
      }
      SafeRelease(prop);
   }
   SafeRelease(propMan);
}

static void proplist_edit_cb(PropEditEvent* , PropEditCBData data)
{  
   PropertyListEditor* ed = (PropertyListEditor*)data;
   PropertyListEditorFillBox(ed,ed->mode);
}



static IProperty* selected_property(PropertyListEditor* ed)
{
   char buf[80];
   AutoAppIPtr_(PropertyManager,propMan);

   int index = SendMessage(ed->hWndPropBox, LB_GETCURSEL, 0, 0);
   if (index == LB_ERR)
      return propMan->GetProperty(PROPID_NULL);
   SendMessage(ed->hWndPropBox, LB_GETTEXT, index, (LPARAM)buf);
   
   // strip off first token
   char* space = strchr(buf,':');
   if (space == NULL) space = strchr(buf,' ');
   if (space) *space = '\0';

   IProperty* prop = propMan->GetPropertyNamed(buf);

   return prop;

}

#pragma off(unreferenced)
static void handle_propbox_command(PropertyListEditor* ed, WPARAM wParam, LPARAM lParam) 
{
   switch (HIWORD(wParam))
   {
      case LBN_DBLCLK:
      {
         IProperty* prop = selected_property(ed);
         if (ed->mode == kIrrelevant)
            prop->Create(ed->editID);
         PropertyEditor* ped = PropertyEditorCreate(prop, ed->editID);
         if (ped)
         {
            // Unset the property, to make listeners & the cancel button happy 
            if (ed->mode == kIrrelevant)
               prop->Delete(ed->editID);

            PropertyEditorInstallCallback(ped,proplist_edit_cb,(PropEditCBData)ed);
         }
         // snap back to relevant mode
         change_edit_mode(ed,kRelevant);

         SafeRelease(prop);
      }
      break;

      case LBN_SELCHANGE:
      {
         int index = SendMessage(ed->hWndPropBox, LB_GETCURSEL, 0, 0);
         if (index != LB_ERR) // selection is defined
         {
            // de-select the meta prop box
            SendMessage(ed->hWndMetaBox, LB_SETCURSEL, (WPARAM)-1, 0);
         }
      }
      break;
   }
}
#pragma on(unrefereced)

//------------------------------------------------------------
// THE METAPROPERTY LIST BOX
//


#define MAKE_META_DATA(pri,obj)  (((pri)<< 16) | ((obj) & 0xFFFF))
#define META_DATA_OBJ(dat)       (short)((dat) & 0xFFFF)
#define META_DATA_PRI(dat)       (tDonorPriority)((dat) >> 16)

static void PropertyListEditorFillMetaBox(PropertyListEditor* ed, int mode)
{
   

   IObjectQuery* donors;
   IDonorQuery* dq = NULL;
   AutoAppIPtr_(TraitManager,traitman);
   AutoAppIPtr_(ObjectSystem,objsys);

   // clear out the listbox
   SendMessage(ed->hWndMetaBox, LB_RESETCONTENT, 0, 0);

   // metaproperties can't have metaproperties, sorry
   if (mode == kIrrelevant && traitman->IsMetaProperty(ed->editID))
      return; 

   if (mode == kRelevant)
   {
      donors = traitman->Query(ed->editID,kTraitQueryDonors);
      COMQueryInterface(donors,IID_IDonorQuery,(void**)&dq);
   }
   else
   {
      donors = traitman->Query(traitman->RootMetaProperty(),kTraitQueryAllDescendents);
   }
   
   for (; !donors->Done(); donors->Next())
   {
      ObjID obj= donors->Object();
      if (mode == kIrrelevant && !traitman->IsMetaProperty(obj))
         continue;

      const Label* name = objsys->GetName(obj);
      Assert_(name);
      int index = SendMessage(ed->hWndMetaBox, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) name->text);

      tDonorPriority pri = (dq) ? dq->Priority() : 0;
      SendMessage(ed->hWndMetaBox, LB_SETITEMDATA, (WPARAM)index, (LPARAM) MAKE_META_DATA(pri,obj));
   }

   SafeRelease(dq);
   SafeRelease(donors);
}

static ObjID selected_metaproperty(PropertyListEditor* ed)
{
   //   char buf[80];

   int index = SendMessage(ed->hWndMetaBox, LB_GETCURSEL, 0, 0);
   long data = SendMessage(ed->hWndMetaBox, LB_GETITEMDATA, index, 0);

   return META_DATA_OBJ(data);
#ifdef PARSE_METAPROP
   AutoAppIPtr(ObjectSystem);

   Label label;
   strncpy(label.text,buf,sizeof(label.text));  
   label.text[sizeof(label.text)-1] = '\0';
      
   
   return pObjectSystem->GetObjectNamed(&label);
#endif 

}



BOOL confirm(HWND hWnd, const char* question);

void remove_selected_metaproperty(PropertyListEditor* ed)
{
   int index = SendMessage(ed->hWndMetaBox, LB_GETCURSEL, 0, 0);
   if (index == LB_ERR)
      return; 
   long data = SendMessage(ed->hWndMetaBox, LB_GETITEMDATA, index, 0);

   if (confirm(ed->hWnd,"Delete metaproperty?"))
   {
      AutoAppIPtr_(TraitManager,traitMan);
      traitMan->RemoveObjMetaPropertyPrioritized(ed->editID,META_DATA_OBJ(data),META_DATA_PRI(data));
      PropertyListEditorFillMetaBox(ed, ed->mode);
   }
}

#pragma off(unreferenced)
static void handle_metabox_command(PropertyListEditor* ed, WPARAM wParam, LPARAM lParam) 
{
   switch (HIWORD(wParam))
   {
      case LBN_DBLCLK:
      {

         ObjID mprop = selected_metaproperty(ed);
         if (mprop == OBJ_NULL) break;

         if (ed->mode == kRelevant)
         {
            PropertyListEditorDesc edesc = { "" };
            PropertyListEditorCreate(mprop, &edesc);
         }
         else
         {
            AutoAppIPtr_(TraitManager,traitman);
            traitman->AddObjMetaProperty(ed->editID,mprop);
            // snap back to relevant mode
            change_edit_mode(ed,kRelevant);
         }
      }
      break;

      case LBN_SELCHANGE:
      {
         int index = SendMessage(ed->hWndMetaBox, LB_GETCURSEL, 0, 0);
         if (index != LB_ERR) // selection is defined
         {
            // de-select the prop box
            SendMessage(ed->hWndPropBox, LB_SETCURSEL, (WPARAM)-1, 0);
         }
      }
      break;
   }
}
#pragma on(unreferenced)

//------------------------------------------------------------
// RANDOM UI TOOLS
//
static BOOL confirm(HWND hWnd, const char* question)
{
   MessageBeep(MB_ICONHAND);
   int result = MessageBox(hWnd, question, "Confirm:" , MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
   return result == IDYES;
}


//------------------------------------------------------------
// WndProc for PropertyListEditor
//
LRESULT CALLBACK PropertyListEditorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   HWND parent;
   PropertyListEditor *ed;

   // get the PropertyListEditor* from the parent window
   parent = GetParent(hWnd);
   if (parent == NULL)
      parent = hWnd;
   ed = (PropertyListEditor*) GetProp(parent, ID_PLISTED);
   
   switch(msg)
   {
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case ID_VIEWBUTTON:
               // switch from add mode to view mode
               change_edit_mode(ed,!ed->mode);

               break;

            case ID_DELETEBUTTON:
            {
               IProperty* prop = selected_property(ed);

               // delete the currently selected property
               if (prop->GetID() != PROPID_NULL)
               {
                  if (confirm(hWnd, "Remove property?"))
                  {
                     prop->Delete(ed->editID);
                     PropertyListEditorFillBox(ed, ed->mode);
                  }
               }
               else
               {
                  remove_selected_metaproperty(ed);
               }

               SafeRelease(prop);

            }
            break;

            case ID_DONEBUTTON:
               DestroyWindow(ed->hWnd);
               return 0;

            case ID_PROPBOX:
               handle_propbox_command(ed,wParam,lParam);
               break;

            case ID_METABOX:
               handle_metabox_command(ed,wParam,lParam);
               break;

         }
         return 0;

      case WM_DESTROY:
         PropertyListEditorDestroy(ed);
         return 0;
   }

   return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Present a view with a list of all the properties for an object.  Each
// property can then be edited with the property editor.
//-----------------------------------------------------------------------------
PropertyListEditor* PropertyListEditorCreate(ObjID editID, PropertyListEditorDesc* editdesc)
{
   PropertyListEditor *ed;

   ed = (PropertyListEditor*)Malloc(sizeof(PropertyListEditor));
   memset(ed, 0, sizeof(PropertyListEditor));

   ed->editID = editID;
   ed->mode = kRelevant;
   
   num_editors++;
   BuildUI(ed, editdesc);
   SetProp(ed->hWnd, ID_PLISTED, (HANDLE)ed);
   PropertyListEditorFillBox(ed, ed->mode);
   PropertyListEditorFillMetaBox(ed, ed->mode);

   return ed;
}

//
// Create default title
//

static const char* default_title(ObjID obj)
{
   static char buf[80];
   sprintf(buf,"Properties for %s",ObjEditName(obj));
   return buf;
}
   
//-----------------------------------------------------------------------------
// Create all the GUI elements needed to show the property list.
//-----------------------------------------------------------------------------
void BuildUI(PropertyListEditor* ed, PropertyListEditorDesc* editdesc)
{
   IWinApp*    pWA;
   WNDCLASS    wc;
   HINSTANCE   hMainInst;
   const char* szName = "Property List Editor";
   int         width, height, top, left;

   // we need the main app's HWND
   pWA = AppGetObj(IWinApp);
   ed->hMainWnd = pWA->GetMainWnd();
   SafeRelease(pWA);

   // we also need the main app's HINSTANCE
   hMainInst = (HINSTANCE) GetWindowLong(ed->hMainWnd, GWL_HINSTANCE);

   // set up the new window's class
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = PropertyListEditorWndProc;
   wc.hInstance = hMainInst;
   wc.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
   wc.lpszClassName = szName;

   // and register it
   RegisterClass(&wc);

   RECT r; 
   GetWindowRect(ed->hMainWnd,&r);

   // make it so big
   width = WINDOW_W + GetSystemMetrics(SM_CXBORDER)*2;
   height = WINDOW_H + GetSystemMetrics(SM_CYBORDER)*2 + GetSystemMetrics(SM_CYCAPTION);

   // chose top left so that not all property editors start in the same place
   left = (r.left+r.right)/4 + num_editors*MARGIN_X;
   top = r.top + num_editors*(MARGIN_Y+GetSystemMetrics(SM_CYCAPTION));

   const char* title = editdesc->title;
   if (*title == '\0')
      title = default_title(ed->editID);

   // and create it!
   ed->hWnd = CreateWindow(szName, title, WS_OVERLAPPED, left, top, width, height,
                           ed->hMainWnd, NULL, hMainInst, NULL);

   ShowWindow(ed->hWnd, SW_SHOWNORMAL);
   UpdateWindow(ed->hWnd);

   // Create a nice standard Windows font for the controls.
   LOGFONT logFont;
   memset(&logFont, 0, sizeof(LOGFONT));
   logFont.lfCharSet = DEFAULT_CHARSET;
   logFont.lfHeight = 12;
   strcpy(logFont.lfFaceName, "MS Sans Serif");
   ed->hWndFont = CreateFontIndirect(&logFont);

   left = MARGIN_X;
   top = MARGIN_Y;

   // get our new window's HINSTANCE
   ed->hInst = (HINSTANCE) GetWindowLong(ed->hWnd, GWL_HINSTANCE);

   // now, let's create the meta property box
   ed->hWndMetaBox = CreateWindow("LISTBOX", "MetaProperties", WS_VISIBLE | WS_CHILD | LBS_STANDARD,
                                  left, top, METABOX_W, METABOX_H, ed->hWnd, (HMENU) ID_METABOX, 
                                  ed->hInst, NULL);
   top += METABOX_H + MARGIN_Y;

   if (ed->hWndFont)
      SendMessage(ed->hWndMetaBox, WM_SETFONT, (WPARAM)ed->hWndFont, 0);

   // now, let's create the property box
   ed->hWndPropBox = CreateWindow("LISTBOX", "Properties", WS_VISIBLE | WS_CHILD | LBS_STANDARD,
                                  left, top, PROPBOX_W, PROPBOX_H, ed->hWnd, (HMENU) ID_PROPBOX, 
                                  ed->hInst, NULL);
   top += PROPBOX_H + MARGIN_Y;

   if (ed->hWndFont)
      SendMessage(ed->hWndPropBox, WM_SETFONT, (WPARAM)ed->hWndFont, 0);

   // compute first button position
   left = (width - BUTTONS_W)/2;

   // create some buttons
   for (int i = 0; i < NUM_BUTTONS; i++, left += BUTTON_W + MARGIN_X)
   {
      ed->hWndButtons[i] 
         = CreateWindow("BUTTON", button_names[i], WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                left, top, BUTTON_W, BUTTON_H, ed->hWnd, (HMENU) i, ed->hInst, NULL);
      if (ed->hWndFont)
         SendMessage(ed->hWndButtons[i], WM_SETFONT, (WPARAM)ed->hWndFont, 0);
   }
}

//-----------------------------------------------------------------------------
// Tears down all the GUI stuff, after "Done" is clicked.
//-----------------------------------------------------------------------------
void PropertyListEditorDestroy(PropertyListEditor* ed)
{
   // Tear down our UI.
   if (ed)
   {
      if (ed->hWndFont)
      {
         DeleteObject(ed->hWndFont);
         ed->hWndFont = 0;
      }
      ed->hWnd = 0;
      ed->hInst = 0;
      SetFocus(ed->hMainWnd);
      Free(ed);
      num_editors--;
   }
   else
      Warning(("PropertyListEditorDestroy: Not editing an object!\n"));
}
