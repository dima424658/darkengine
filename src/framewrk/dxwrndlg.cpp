/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////
//
// (c) 1998 Looking Glass Technologies Inc.
// David Teichholtz
//
// Description: Routines which deal with putting up the opening 
// dialog warning message.  As a famous writer once said, much ado about
// about nothing. 
////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <resource.h>
#include <memall.h>
#include <config.h>
#include <str.h>
#include <drkuires.h>
#include <dxwrndlg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

bool bNeverAgainSelected;



// the dialog callback routine 
BOOL CALLBACK GameEntryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,  LPARAM  /* lParam */)
   {

   HKEY   hKey;
   DWORD  retCode;
   DWORD  dwKeyBufferSize=sizeof(BOOL);
   BOOL   NeverAgain=TRUE;
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         SetWindowLong(hwndDlg, GWL_STYLE, 0x04);
         RECT rcDlg, rcWnd;
         GetWindowRect(hwndDlg, &rcDlg);
         GetWindowRect(GetDesktopWindow(), &rcWnd);
         int iWndCenterX = rcWnd.right/2, iWndCenterY = rcWnd.bottom/2;
         int iWidthWindow = rcDlg.right - rcDlg.left,
         iHeightWindow = rcDlg.bottom - rcDlg.top;
         int iNewX = iWndCenterX - (iWidthWindow/2),
         iNewY = ((iWndCenterY - (iHeightWindow/2)) * 66) / 100;
         MoveWindow(hwndDlg, iNewX, iNewY, iWidthWindow, iHeightWindow, TRUE);
         // get the text out of the LG resource system
         cStr key_str = FetchUIString("misc","driver_text");
         cStr never_str = FetchUIString("misc","neveragain");
         // and display it
         SetWindowText(GetDlgItem(hwndDlg,IDC_DXINFO),(const char*)key_str);
         SetWindowText(GetDlgItem(hwndDlg,IDC_NEVERAGAIN),(const char *)never_str);
         return 1;
      }
      case WM_COMMAND:
      {
         unsigned wNotifyCode = HIWORD(wParam); // notification code
         unsigned wID = LOWORD(wParam);
         unsigned wCheckedOrNot;
         switch (wID)
         {
            case IDOK:  // dialog box OK
               if (bNeverAgainSelected) // button was checked
               {
                       // going to have to get the app keys out
                       // of the resource system
                  cStr key_str = FetchUIString("misc","regkey");
                   // open the main key 
                   retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                             (const char *)key_str,
                              0,
                              KEY_WRITE,
                              &hKey);

                   if(retCode==ERROR_SUCCESS)  // oxymoron
                   {
                      retCode = RegSetValueEx(  // subkey
                                hKey,	
                                "NeverAgain",  // same in every language
                                NULL,
                                REG_DWORD, 
                                (LPBYTE) &NeverAgain,
                                dwKeyBufferSize); 	

                    }  // opened successfully
                    RegCloseKey(hKey);
                   }     // bNeverAgainSelected
                    EndDialog(hwndDlg, 0);
                    break;
                case IDC_NEVERAGAIN:
                    wCheckedOrNot = 
                       IsDlgButtonChecked(hwndDlg,
                          IDC_NEVERAGAIN);
                    if (wCheckedOrNot==BST_CHECKED) {
                       bNeverAgainSelected=TRUE;
                    }
                    else {
                       bNeverAgainSelected=FALSE;
                    }
                    break;
                  EndDialog(hwndDlg, 0);
                  break;
                }
            }
        }

    return 0;
    }

//
// Display 'Please shut down.... ' message
//
void
DxWarnDlg()
{

    HINSTANCE hInst;        // the instance of this exe

    HKEY   hKey;
    DWORD  retCode;
    DWORD  dwKeyBufferSize=sizeof(int);
    int    NeverAgain=FALSE;

     // open the main key (which had better be there!) 
     // get the key from the resource system
     cStr key_str = FetchUIString("misc","regkey");
     retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              (const char *)key_str,
                              0,
                              KEY_READ,
                              &hKey);

     if(retCode==ERROR_SUCCESS) { // oxymoron
        retCode = RegQueryValueEx(
          hKey,	
          "NeverAgain",
          NULL,
          NULL,	                              // I don't care about it's type 
          (unsigned char *) &NeverAgain,      // address of data buffer 
          &dwKeyBufferSize); 	              // address of data buffer size 
   
        RegCloseKey(hKey);                        // we are done either way 

    // we only display the warning if NeverAgain is TRUE and we can 
    // find the dialog.  Developers don't usually link in the .res. 
    
        if (retCode==ERROR_SUCCESS) 
        {
           
           if (!NeverAgain) 
           {   
              hInst = GetModuleHandle(NULL);
              if ( FindResource(hInst, MAKEINTRESOURCE(IDD_DXWARNDIALOG), 
                                RT_DIALOG))
              {
                 DialogBox(hInst, MAKEINTRESOURCE(IDD_DXWARNDIALOG), 
                           GetDesktopWindow(), (DLGPROC)GameEntryDialogProc);

              }     
           } // !NeverAgain
        }    // Found sub key
     }    // Found key


} // end of routine 




