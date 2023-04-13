/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/wintools/RCS/winmenu.h $
// $Author: TOML $
// $Date: 1997/08/05 13:08:50 $
// $Revision: 1.1 $
//

#ifndef __WINMENU_H
#define __WINMENU_H

class cAnsiStr;
class cUniStr;

#ifdef UNICODE
    typedef cUniStr cStr;
    #error "Need unicode version of cStr"
#else
    typedef cAnsiStr cStr;
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinMenu
//
// A thin wrapper around Windows menu APIs
//

class cWinMenu
{
public:

   ////////////////////////////////////
   //
   // Constructor & lifetime management
   //
   cWinMenu();
   ~cWinMenu();

   BOOL CreateMenu();
   BOOL CreatePopupMenu();
   BOOL LoadMenu(const char * pszResourceName, HINSTANCE = NULL);
   BOOL LoadMenu(UINT nIDResource, HINSTANCE = NULL);
   BOOL LoadMenuIndirect(const void * pMenuTemplate);
   BOOL DestroyMenu();

   ////////////////////////////////////
   //
   // Menu handle management
   //
   
   HMENU GetSafeHmenu() const;
   operator HMENU() const;

   BOOL  Attach(HMENU hMenu);
   BOOL  Attach(HWND hWnd);
   HMENU Detach();
   void  DetachAndSet(HWND hWnd);

   ////////////////////////////////////
   //
   // Menu level operations
   //
   
   BOOL DeleteMenu(uint nPosition, uint nFlags);
   BOOL TrackPopupMenu(uint nFlags, int x, int y,
                       HWND hWnd, const RECT * pRect = 0);

   ////////////////////////////////////
   //
   // Menu-item level operations
   //
   
   BOOL  AppendMenu(uint nFlags, uint nIDNewItem = 0, const char * pszNewItem = NULL);
   uint  CheckMenuItem(uint nIDCheckItem, uint nCheck);
   uint  EnableMenuItem(uint nIDEnableItem, uint nEnable);
   uint  GetMenuItemCount() const;
   uint  GetMenuItemID(int nPos) const;
   uint  GetMenuState(uint nID, uint nFlags) const;
   int   GetMenuString(uint nIDItem, char * pString, int nMaxCount, uint nFlags) const;
   int   GetMenuString(uint nIDItem, cStr & string, uint nFlags) const;
   HMENU GetSubMenu(int nPos) const;
   BOOL  InsertMenu(uint nPosition, uint nFlags, uint nIDNewItem = 0, const char * pszNewItem = NULL);
   BOOL  ModifyMenu(uint nPosition, uint nFlags, uint nIDNewItem = 0, const char * pszNewItem = NULL);
   BOOL  RemoveMenu(uint nPosition, uint nFlags);
   BOOL  SetMenuItemBitmaps(uint nPosition, uint nFlags, HBITMAP hBmpUnchecked, HBITMAP hBmpChecked);
   BOOL  CheckMenuRadioItem(uint nIDFirst, uint nIDLast, uint nIDItem, uint nFlags);
   
private:
   HMENU m_hMenu;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinMenu, inline functions
//

inline cWinMenu::cWinMenu()
 : m_hMenu(0)
{
}

///////////////////////////////////////

inline cWinMenu::~cWinMenu()
{
   if (m_hMenu)
      ::DestroyMenu(m_hMenu);
}

///////////////////////////////////////

inline BOOL cWinMenu::CreateMenu()
{
   if (m_hMenu)
      ::DestroyMenu(m_hMenu);
   m_hMenu = ::CreateMenu();
   return !!m_hMenu;
}

///////////////////////////////////////

inline BOOL cWinMenu::CreatePopupMenu()
{
   if (m_hMenu)
      ::DestroyMenu(m_hMenu);
   m_hMenu = ::CreatePopupMenu();
   return !!m_hMenu;
}

///////////////////////////////////////

inline BOOL cWinMenu::LoadMenu(const char * pszResourceName, HINSTANCE hInstance)
{
   if (m_hMenu)
      ::DestroyMenu(m_hMenu);
   if (!hInstance)
      hInstance = GetModuleHandle(NULL);
   m_hMenu = ::LoadMenu(hInstance, pszResourceName);
   return !!m_hMenu;
}

///////////////////////////////////////

inline BOOL cWinMenu::LoadMenu(UINT nIDResource, HINSTANCE hInstance)
{
   return LoadMenu((const char *)MAKEINTRESOURCE(nIDResource), hInstance);
}

///////////////////////////////////////

inline BOOL cWinMenu::LoadMenuIndirect(const void * pMenuTemplate)
{
   if (m_hMenu)
      ::DestroyMenu(m_hMenu);
   m_hMenu = ::LoadMenuIndirect(pMenuTemplate);
   return !!m_hMenu;
}

///////////////////////////////////////

inline BOOL cWinMenu::DestroyMenu()
{
   if (m_hMenu)
      ::DestroyMenu(m_hMenu);
   return TRUE;
}

///////////////////////////////////////

inline HMENU cWinMenu::GetSafeHmenu() const
{
   if (this)
      return m_hMenu;
   return NULL;
}

///////////////////////////////////////

inline cWinMenu::operator HMENU() const
{
   return m_hMenu;
}

///////////////////////////////////////

inline BOOL cWinMenu::Attach(HWND hWnd)
{
   m_hMenu = GetMenu(hWnd);
   return !!m_hMenu;
}

///////////////////////////////////////

inline BOOL cWinMenu::Attach(HMENU hMenu)
{
   m_hMenu = hMenu;
   return TRUE;
}

///////////////////////////////////////

inline HMENU cWinMenu::Detach()
{
   HMENU hMenu = m_hMenu;
   m_hMenu = NULL;
   return hMenu;
}

///////////////////////////////////////

inline void cWinMenu::DetachAndSet(HWND hWnd)
{
   SetMenu(hWnd, m_hMenu);
   m_hMenu = NULL;
}

///////////////////////////////////////

inline BOOL cWinMenu::DeleteMenu(uint nPosition, uint nFlags)
{
   return ::DeleteMenu(m_hMenu, nPosition, nFlags);
}

///////////////////////////////////////

inline BOOL cWinMenu::TrackPopupMenu(uint nFlags, int x, int y, HWND hWnd, const RECT * pRect)
{
   return ::TrackPopupMenu(m_hMenu, nFlags, x, y, 0, hWnd, pRect);
}

///////////////////////////////////////

inline BOOL cWinMenu::AppendMenu(uint nFlags, uint nIDNewItem, const char * pszNewItem)
{
   return ::AppendMenu(m_hMenu, nFlags, nIDNewItem, pszNewItem);
}

///////////////////////////////////////

inline uint cWinMenu::CheckMenuItem(uint nIDCheckItem, uint nCheck)
{
   return ::CheckMenuItem(m_hMenu, nIDCheckItem, nCheck);
}

///////////////////////////////////////

inline uint cWinMenu::EnableMenuItem(uint nIDEnableItem, uint nEnable)
{
   return ::EnableMenuItem(m_hMenu, nIDEnableItem, nEnable);
}

///////////////////////////////////////

inline uint cWinMenu::GetMenuItemCount() const
{
   return ::GetMenuItemCount(m_hMenu);
}

///////////////////////////////////////

inline uint cWinMenu::GetMenuItemID(int nPos) const
{
   return ::GetMenuItemID(m_hMenu, nPos);
}

///////////////////////////////////////

inline uint cWinMenu::GetMenuState(uint nID, uint nFlags) const
{
   return ::GetMenuState(m_hMenu, nID, nFlags);
}

///////////////////////////////////////

inline int cWinMenu::GetMenuString(uint nIDItem, char * pString, int nMaxCount, uint nFlags) const
{
   return ::GetMenuString(m_hMenu, nIDItem, pString, nMaxCount, nFlags);
}

///////////////////////////////////////

inline HMENU cWinMenu::GetSubMenu(int nPos) const
{
   return ::GetSubMenu(m_hMenu, nPos);
}

///////////////////////////////////////

inline BOOL cWinMenu::InsertMenu(uint nPosition, uint nFlags, uint nIDNewItem, const char * pszNewItem)
{
   return ::InsertMenu(m_hMenu, nPosition, nFlags, nIDNewItem, pszNewItem);
}

///////////////////////////////////////

inline BOOL cWinMenu::ModifyMenu(uint nPosition, uint nFlags, uint nIDNewItem, const char * pszNewItem )
{
   return ::ModifyMenu(m_hMenu, nPosition, nFlags, nIDNewItem, pszNewItem);
}

///////////////////////////////////////

inline BOOL cWinMenu::RemoveMenu(uint nPosition, uint nFlags)
{
   return ::RemoveMenu(m_hMenu, nPosition, nFlags);
}

///////////////////////////////////////

inline BOOL cWinMenu::SetMenuItemBitmaps(uint nPosition, uint nFlags, HBITMAP hBmpUnchecked, HBITMAP hBmpChecked)
{
   return ::SetMenuItemBitmaps(m_hMenu, nPosition, nFlags, hBmpUnchecked, hBmpChecked);
}

///////////////////////////////////////

inline BOOL cWinMenu::CheckMenuRadioItem(uint nIDFirst, uint nIDLast, uint nIDItem, uint nFlags)
{
   return ::CheckMenuRadioItem(m_hMenu, nIDFirst, nIDLast, nIDItem, nFlags);
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WINMENU_H */