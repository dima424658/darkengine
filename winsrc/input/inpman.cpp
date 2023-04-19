// $Header: x:/prj/tech/winsrc/input/RCS/inpman.cpp 1.1 1997/10/06 20:46:12 JON Exp $

#include <inpman.h>
#include <inpdynf.h>
#include <inpcompo.h>

#include <wappapi.h>
#include <appagg.h>
#include <lgassert.h>
#include <dbg.h>

//
// create input manager object and add to aggregate
//
tResult LGAPI _InputManagerCreate(IUnknown * pOuterUnknown)
{
    if (new cInputManager(pOuterUnknown) != NULL)
        return S_OK;
    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cInputManager
//

///////////////////////////////////////
//
// Pre-fab COM implementations
//
IMPLEMENT_AGGREGATION_SELF_DELETE(cInputManager);

///////////////////////////////////////

cInputManager::cInputManager(IUnknown *pOuterUnknown)
{
   // Add internal components to outer aggregate, ensuring IDisplayDevice comes first......
   sRelativeConstraint constraints[] =
   {
       {   kConstrainAfter, &IID_IGameShell },
       {   kNullConstraint, NULL }
   };

   // Add internal components to outer aggregate...
   INIT_AGGREGATION_1(pOuterUnknown, IID_IInputManager, this, kPriorityLibrary, constraints);
}

///////////////////////////////////////
//
// Iteration
//
BOOL cInputManager::FindType(sInputDeviceIter *pIter, GUID interfaceID)
{
   sInputDeviceInfo devInfo;
   cInputDeviceNode *pDev = (cInputDeviceNode*)(pIter->m_pDev);

   while (pDev != NULL)
   {
      if (SUCCEEDED(pDev->item->GetInfo(&devInfo)) && (devInfo.interfaceID == interfaceID))
      {
         pIter->m_pDev = (void*)pDev;
         return TRUE;
      }
      pDev = pDev->GetNext();
   }
   pIter->m_pDev = (void*)pDev;
   return FALSE;
}

STDMETHODIMP_(void) cInputManager::IterStart(sInputDeviceIter *pIter, GUID interfaceID)
{
   pIter->m_pDev = (void*)(m_devList.GetFirst());
   FindType(pIter, interfaceID);
}

STDMETHODIMP_(void) cInputManager::IterNext(sInputDeviceIter *pIter)
{
   sInputDeviceInfo devInfo;
   cInputDeviceNode *pDev = (cInputDeviceNode*)(pIter->m_pDev);
      
   if (FAILED(pDev->item->GetInfo(&devInfo)))
   {
      pIter->m_pDev = NULL;
      return;
   }
   pIter->m_pDev = (void*)(pDev->GetNext());
   FindType(pIter, devInfo.interfaceID);
}

STDMETHODIMP_(BOOL) cInputManager::IterFinished(sInputDeviceIter *pIter)
{
   return (pIter->m_pDev == NULL);
}

STDMETHODIMP_(IInputDevice*) cInputManager::IterGet(sInputDeviceIter *pIter)
{
   cInputDeviceNode *pDev = (cInputDeviceNode*)(pIter->m_pDev);

   if (pDev != NULL)
      return (pDev->item);
   else
      return NULL;
}

///////////////////////////////////////
//
// Set a DWORD property on a DirectInputDevice.
//
HRESULT cInputManager::SetDIDwordProperty(LPDIRECTINPUTDEVICE pDev, REFGUID guidProperty,
                              DWORD dwObject, DWORD dwHow, DWORD dwValue)
{
   DIPROPDWORD dipdw;

   dipdw.diph.dwSize       = sizeof(dipdw);
   dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
   dipdw.diph.dwObj        = dwObject;
   dipdw.diph.dwHow        = dwHow;
   dipdw.dwData            = dwValue;
   return pDev->SetProperty(guidProperty, &dipdw.diph);
}

///////////////////////////////////////
//
// Initializes DirectInput for an enumerated joystick device.
// Creates the device, sets the standard joystick data format and
// sets the cooperative level.
// If any step fails, just skip the device and go on to the next one.
//
static BOOL CALLBACK InitJoystickInput(LPCDIDEVICEINSTANCE pDInst, LPVOID pRef)
{
   cInputManager *pInputMan = (cInputManager*)pRef;
   LPDIRECTINPUTDEVICE pDev;

   // create the DirectInput joystick device
   if (pInputMan->m_pDI->CreateDevice(pDInst->guidInstance, &pDev, NULL) != DI_OK)
   {
      Warning(("IDirectInput::CreateDevice FAILED\n"));
      return DIENUM_CONTINUE;
   }
   // create joystick device
   cJoystick *pJoy = new cJoystick();
   if (FAILED(pJoy->Init(pDev)))
   {
      pDev->Release();
      return DIENUM_CONTINUE;
   }
   pDev->Release();
   // add device to out list
   cInputDeviceNode *pDevNode = new cInputDeviceNode;
   pDevNode->item = pJoy;
   // add to device list
   pInputMan->m_devList.Prepend(pDevNode);
   return DIENUM_CONTINUE;
}

///////////////////////////////////////

HRESULT cInputManager::Init()
{
   // hook up recording, if not already done
   GetInputComponents();

   // load direct input DLL
   if (!LoadDirectInput())
   {
      Warning(("cInputManager::Init - can't load dinput.dll\n"));
      return E_FAIL;
   }
   // we need the main app's HWND
   IWinApp *pWA = AppGetObj(IWinApp);
   Assert_(pWA != NULL);
   HWND hMainWnd = pWA->GetMainWnd();
   SafeRelease(pWA);

   // we also need the main app's HINSTANCE
   HINSTANCE hMainInst = (HINSTANCE) GetWindowLong(hMainWnd, GWL_HINSTANCE);

   // create the DirectInput interface object
   if (DynDirectInputCreate(hMainInst, DIRECTINPUT_VERSION, &m_pDI, NULL) == DI_OK)
   {
      // Enumerate the joystick devices.  
      m_pDI->EnumDevices(DI8DEVTYPE_JOYSTICK, InitJoystickInput, this, DIEDFL_ATTACHEDONLY);
      m_pDI->Release();    // Finished with DX 5.0.
   } 
   else 
   {
      Warning(("cInputManager::Init - DirectInputCreate %d FAILED\n", DIRECTINPUT_VERSION));
      return E_FAIL;
   }
   return S_OK;
}

///////////////////////////////////////

HRESULT cInputManager::End()
{
   sInputDeviceIter iter;
   cJoystick *pJoy;

   // delete all devices
   IterStart(&iter, IID_IJoystick); 
   while (!IterFinished(&iter))
   {
      pJoy = (cJoystick*)IterGet(&iter);      
      IterNext(&iter);
      delete pJoy;
   }      
   return S_OK;
}

///////////////////////////////////////
