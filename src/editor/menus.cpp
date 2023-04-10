/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/editor/menus.cpp,v $
// $Author: henrys $
// $Date: 1999/10/29 19:18:50 $
// $Revision: 1.7 $
//
// @Note (toml 08-03-97): This is a temporary solution that will have to be
// rethought when dynamic menus are supported. No sub-menus right now

#include <windows.h>
#include <lg.h>

#include <appagg.h>
#include <wappapi.h>

#include <dynarray.h>
#include <str.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <winmenu.h>
#include <config.h>

#include <menus.h>
#include <command.h>
#include <mprintf.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

#define kMenuCommandBase 1000

class cMenuCommands
{
public:
   ~cMenuCommands();

   unsigned NewCommand(const char *);
   const char * Lookup(unsigned);
   void ClearAll();

private:
   cDynArray<const char *> m_CommandTexts;
};

///////////////////////////////////////

inline cMenuCommands::~cMenuCommands()
{
   ClearAll();
}

///////////////////////////////////////

inline unsigned cMenuCommands::NewCommand(const char * pszCommand)
{
   m_CommandTexts.Append(strdup(pszCommand));
   return kMenuCommandBase + m_CommandTexts.Size() - 1;
}

///////////////////////////////////////

inline const char * cMenuCommands::Lookup(unsigned id)
{
   unsigned index = id - kMenuCommandBase;
   if (index < m_CommandTexts.Size())
      return m_CommandTexts[index];
   else
      return NULL;
}

///////////////////////////////////////

void cMenuCommands::ClearAll()
{
   for (int i = 0; i < m_CommandTexts.Size(); i++)
      free((void *)(m_CommandTexts[i]));
   m_CommandTexts.SetSize(0);
}

///////////////////////////////////////////////////////////////////////////////

class cMenuSet
{
public:
   cMenuSet();
   ~cMenuSet();

   void BeginMenu(const char * = NULL);
   void AddItem(const char *, unsigned id);
   void AddSeparator();
   void EndMenu();

   void AttachToWindow(HWND hWnd);
   void DetachFromWindow();

   void DestroyAll();

   cWinMenu* GetMenuByNumber(int number);

private:
   HWND                  m_hWnd;
   int                   m_iCurrentMenu;
   cDynArray<cWinMenu *> m_Menus;
   cDynArray<int>        m_MenuStack;

};

///////////////////////////////////////

inline cMenuSet::cMenuSet()
 : m_hWnd(0),
   m_iCurrentMenu(-1)
{
}

///////////////////////////////////////

inline cMenuSet::~cMenuSet()
{
   DestroyAll();
}

///////////////////////////////////////

inline void cMenuSet::BeginMenu(const char * pszSubMenuName)
{
   cWinMenu * pMenu = new cWinMenu();

   pMenu->CreateMenu();
   m_Menus.Append(pMenu);

   m_MenuStack.Append(m_iCurrentMenu);

   int oldMenu = m_iCurrentMenu;

   m_iCurrentMenu = (int)m_Menus.Size() - 1;

   if (m_iCurrentMenu != 0)
      m_Menus[oldMenu]->AppendMenu(MF_POPUP, (uint)(HMENU)(*pMenu), pszSubMenuName);

}

///////////////////////////////////////

inline void cMenuSet::EndMenu()
{
   m_iCurrentMenu = m_MenuStack[m_MenuStack.Size() - 1];
   m_MenuStack.SetSize(m_MenuStack.Size() - 1);
}

///////////////////////////////////////

inline void cMenuSet::AddItem(const char * pszMenuItemName, unsigned id)
{
   if (m_Menus.Size() != 0)
      m_Menus[m_iCurrentMenu]->AppendMenu(MF_STRING, id, pszMenuItemName);
}

///////////////////////////////////////

inline void cMenuSet::AddSeparator()
{
   if (m_Menus.Size() != 0)
      m_Menus[m_iCurrentMenu]->AppendMenu(MF_SEPARATOR, 0, 0);
}

///////////////////////////////////////

void cMenuSet::AttachToWindow(HWND hWnd)
{
   if (m_Menus.Size() == 0)
      return;

   if (!hWnd)
      return;

   if (m_hWnd)
      SetMenu(m_hWnd, NULL);

   m_hWnd = hWnd;

   SetMenu(m_hWnd, *(m_Menus[0]));
}

///////////////////////////////////////

void cMenuSet::DetachFromWindow()
{
   if (m_hWnd)
      SetMenu(m_hWnd, NULL);
   m_hWnd = NULL;
}

///////////////////////////////////////

void cMenuSet::DestroyAll()
{
   if (m_Menus.Size() == 0)
      return;

   int iFirst = (m_hWnd) ? 1 : 0;

   for (int i = iFirst; i < m_Menus.Size(); i++)
   {
      delete m_Menus[i];
   }
   m_iCurrentMenu = -1;
   m_MenuStack.SetSize(0);
   m_Menus.SetSize(0);
}

///////////////////////////////////////

cWinMenu* cMenuSet::GetMenuByNumber(int number)
{
    return (m_Menus[number]);
}

///////////////////////////////////////////////////////////////////////////////

typedef cStrHashTable<BOOL> cMenusInProgress;

///////////////////////////////////////////////////////////////////////////////

static cMenuCommands g_MenuCommands;
static cMenuSet      g_MenuSet;

///////////////////////////////////////////////////////////////////////////////

#define kMaxMenuConfigEntry 512

void ParseMenu(const char * pszMenuText, const char * pszMenuTag, cMenuSet & menuSet, cMenusInProgress & menusInProgress)
{
   cStr menuDefStr;
   BOOL foundMenu = config_get_raw(pszMenuTag,
                                   menuDefStr.GetBuffer(kMaxMenuConfigEntry),
                                   kMaxMenuConfigEntry);
   menuDefStr.ReleaseBuffer();
   menuDefStr.Trim();

   if (foundMenu && !menuDefStr.IsEmpty())
   {
      if (menusInProgress.Lookup(pszMenuTag, &foundMenu))
         return;

      menusInProgress.Insert(pszMenuTag, TRUE);
      menuSet.BeginMenu(pszMenuText);

      cStr continuationMenuTag(pszMenuTag);
      int  iCurrentPart = 0;

      continuationMenuTag += "_0";

      while (foundMenu)
      {
         int  indexNextEntry = 0;
         int  indexSemicolon = 0;

         cStr menuEntry;
         cStr menuText;
         cStr menuValue;

         while (menuDefStr[indexNextEntry])
         {
            // Find semicolon
            indexSemicolon = indexNextEntry + menuDefStr.SpanExcluding("|", indexNextEntry);

            // If the string is non-zero, deal with it
            if (indexSemicolon - indexNextEntry != 0)
            {
               menuDefStr.Mid(menuEntry, indexNextEntry, indexSemicolon - indexNextEntry);

               int indexColon = menuEntry.Find(':');
               if (indexColon != -1)
               {
                  menuEntry.Mid(menuText, 0, indexColon);
                  menuEntry.Mid(menuValue, indexColon + 1, menuEntry.GetLength() - (indexColon + 1));
               }
               else
               {
                  menuText = menuEntry;
                  menuValue = menuEntry;
               }
               menuText.Trim();
               menuValue.Trim();

               if (menuValue.Find("menu_") == 0)
               {
                  ParseMenu(menuText, menuValue, menuSet, menusInProgress);
               }
               else if (menuText.Find("sep") == 0)
               {
                  menuSet.AddSeparator();
               }
               else
               {
                  menuSet.AddItem(menuText, g_MenuCommands.NewCommand(menuValue));
               }

            }

            menuEntry.Empty();
            menuText.Empty();
            menuValue.Empty();

            // Skip to the next non-semicolon
            indexNextEntry = indexSemicolon + menuDefStr.SpanIncluding("|", indexSemicolon);
         }
         iCurrentPart++;
         continuationMenuTag[continuationMenuTag.GetLength() - 1] = '0' + iCurrentPart;

         foundMenu = config_get_raw(continuationMenuTag,
                                    menuDefStr.GetBuffer(kMaxMenuConfigEntry),
                                    kMaxMenuConfigEntry);
         menuDefStr.ReleaseBuffer();
         menuDefStr.Trim();
      }
      menuSet.EndMenu();
      menusInProgress.Delete(pszMenuTag);
   }
   else
      menuSet.AddItem(pszMenuText, 0);
}

///////////////////////////////////////////////////////////////////////////////

BOOL CreateMenu(const char * pszMenuTag)
{
   cMenusInProgress menusInProgress;

   ParseMenu(NULL, pszMenuTag, g_MenuSet, menusInProgress);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void SetMainMenu(const char * pszName)
{
   AutoAppIPtr(WinApp);

   HWND hWnd = pWinApp->GetMainWnd();

   g_MenuSet.DetachFromWindow();
   g_MenuSet.DestroyAll();
   g_MenuCommands.ClearAll();

   if (pszName)
   {
      cStr menuNameStr;

      menuNameStr = "menu_";
      menuNameStr += pszName;

      menuNameStr.Trim();

      CreateMenu(menuNameStr);

      g_MenuSet.AttachToWindow(hWnd);
   }
}

///////////////////////////////////////////////////////////////////////////////

void MenuCommand(unsigned id)
{
   const char * pszMenuCommand = g_MenuCommands.Lookup(id);

   if (pszMenuCommand)
   {
      CommandExecute((char *)pszMenuCommand);
   }
}

///////////////////////////////////////////////////////////////////////////////

//  Sets an exclusive radio-button style bullet beside a menu item
//
extern "C"
{
    void SetRadioCheckmark (int menu, int firstPos, int lastPos, int checkPos)
    {
        g_MenuSet.GetMenuByNumber(menu)->CheckMenuRadioItem (firstPos, lastPos, checkPos, MF_BYPOSITION);
    }
}
