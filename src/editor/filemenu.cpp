/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/filemenu.cpp,v 1.5 1998/10/17 19:28:42 dc Exp $

#include <lg.h>
#include <comtools.h>
#include <filemenu.h>
#include <str.h>
#include <dbfile.h>

// for the windows controls
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <wappapi.h>
#include <loopapi.h>
#include <appagg.h>

// must be last header
#include <dbmem.h>

char* FileDialog(eFileMenuMode mode, sFileMenuFilter* pFilters, const char* def_ext, char* buffer, int buflen)
{
   // we need the main app's HWND
   AutoAppIPtr_(WinApp,pWA);

   buffer[buflen-1] = '\0';

   // clear out and init the Open Common Dialog information
   OPENFILENAME ofn;
   memset(&ofn, 0, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = pWA->GetMainWnd();
   ofn.hInstance = pWA->GetInstance();
   
   // build filters
   cStr filterstring = "";
   sFileMenuFilter* f;
   int idx = 0; 
   int i;
   int extlen = strlen(def_ext);

   for (i = 1, f = pFilters; f != NULL && f->text != NULL; f++, i++)
   {
      filterstring += f->text;
      filterstring += '\0';
      filterstring += f->pattern;
      filterstring += '\0';

      // look for def_ext inside pattern, in order to figure out default pattern.
      if (idx == 0)
         for (const char* s = f->pattern; *s != '\0'; s++)
            if (strnicmp(s,def_ext,extlen) == 0)
            {
               idx = i; 
               break;
            }
   }

   filterstring += '\0';
   filterstring += '\0';
   const char* fstring = filterstring;
   
   ofn.lpstrFilter = (char*)fstring;
   ofn.lpstrDefExt = def_ext;
   ofn.nFilterIndex = idx; 
   ofn.lpstrFile = buffer;
   ofn.nMaxFile = buflen;
   ofn.Flags = OFN_NOCHANGEDIR|OFN_HIDEREADONLY; 
   if (mode == kFileMenuLoad)
      ofn.Flags |= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

   // do the load or save (NOTE: we only get the file name here -- the actual loading and
   // saving is done in load_world or save_world)
   BOOL found = FALSE;
   switch (mode)
   {
      case kFileMenuLoad:
         found = GetOpenFileName(&ofn);
         break;
      case kFileMenuSave:
         found = GetSaveFileName(&ofn);
         break;
   }

   return (found) ? buffer : NULL;   
}

#ifdef EDITOR

////////////////////////////////////////

#define ID_LOAD   0
#define ID_SAVE   1
#define ID_CLEAR  2

// our stupid static storage
static HWND hMainWnd, hConWnd = NULL;

// in dbfile.c 
EXTERN void load_file(char *str);
EXTERN void save_world(char *arg);

// the WndProc for the file menu - this only gets messages for this small file menu
LRESULT CALLBACK FileMenuWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   int result;

   switch(msg)
   {
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case ID_LOAD:
               load_file(NULL);
               break;

            case ID_SAVE:
               save_world(NULL);
               break;

            case ID_CLEAR:
               result = MessageBox(hConWnd, "Are you sure?", "Clear World", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
               if (result == IDYES)
                  dbBuildDefault();
               break;
         }

         // restore the focus to the parent window
         SetFocus(hWnd);
         break;

      case WM_CLOSE:
         // on close, make the main app window active
         SetFocus(hMainWnd);
         hConWnd = NULL;
         break;
   }

   return DefWindowProc(hWnd, msg, wParam, lParam);
}

//
// create a new little file menu window and put some buttons on it
//

void FileMenu(void)
{
   IWinApp* pWA;
   HINSTANCE hMainInst, hConInst;
   WNDCLASS wc;
   static const char *szName = "File menu";
   int top, left, width, height;

   // are we already active?
   if (hConWnd)
   {
      Warning(("FileMenu: already active!\n"));
      return;
   }

   // we need the main app's HWND
   pWA = AppGetObj(IWinApp);
   hMainWnd = IWinApp_GetMainWnd(pWA);
   SafeRelease(pWA);

   // we also need the main app's HINSTANCE
   hMainInst = (HINSTANCE) GetWindowLong(hMainWnd, GWL_HINSTANCE);

   // set up the new window's class
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = FileMenuWndProc;
   wc.hInstance = hMainInst;
   wc.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
   wc.lpszClassName = szName;

   // and register it
   RegisterClass(&wc);

   // make it so big
   width = 140;
   height = 160;
   top = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
   left = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

   // and create it!
   hConWnd = CreateWindow(szName, "File menu", WS_OVERLAPPED | WS_SYSMENU, top, left, width, height,
                          hMainWnd, NULL, hMainInst, NULL);

   ShowWindow(hConWnd, SW_SHOWNORMAL);
   UpdateWindow(hConWnd);

   // get our new window's HINSTANCE
   hConInst = (HINSTANCE) GetWindowLong(hConWnd, GWL_HINSTANCE);

   // and create the buttons on it
   CreateWindow("BUTTON", "Load world", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                25, 20, 90, 25, hConWnd, (HMENU) ID_LOAD, hConInst, NULL);
   CreateWindow("BUTTON", "Save world", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                25, 55, 90, 25, hConWnd, (HMENU) ID_SAVE, hConInst, NULL);
   CreateWindow("BUTTON", "Clear world", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                25, 90, 90, 25, hConWnd, (HMENU) ID_CLEAR, hConInst, NULL);
}

#endif
