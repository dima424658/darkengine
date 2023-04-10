//
// $Header: x:/prj/tech/winsrc/input/RCS/joystick.cpp 1.44 1998/02/06 18:02:01 PATMAC Exp $
//
// DirectInput implementation of joystick "driver"

#include <joynew.h>
#include <inperr.h>
#include <inpdynf.h>
#include <inpbase.h>
#include <inpcompo.h>

#include <lg.h>
#include <aggmemb.h>
#include <appagg.h>
#include <wappapi.h>
#include <dbg.h>
#include <recapi.h>
#include <mprintf.h>

// we try to create an exact duplicate of the default joystick format.
// should be able to just uncomment the SetDataFormat call which uses
// c_dfDIJoystick, but to use this your app must staticly link in dinput.lib
// Our normal joystick format is close to the default joystick format
// anyway, it has all the fields it uses at the same offsets as the default,
// but there are some fields which are not filled in.  This was done
// because otherwise some joystick drivers (CH ForceFX & Virtual Pilot Pro)
// just won't work (POV hats are missing).

// total number of DI "objects"/joystick
const int kJoyBase = 8;      // # of non-button data items
const int kJoystickObjsNum = kJoyBase+kJoyButtonsMax;

// hat positions
const DWORD kJoyHatCenter = 65535;
const DWORD kJoyHatUp = 0;
const DWORD kJoyHatRight = 9000;
const DWORD kJoyHatDown = 18000;
const DWORD kJoyHatLeft = 27000;

// this flag is undocumented, but seems to indicate data format elements which
//  are optional - if you ask for a data element which the attached joystick
//  doesn't generate, SetDataFormat will fail unless this bit is set
#define WACKY_DI_FLAG 0x80000000

#define COMMON_FLAGS        (DIDFT_ANYINSTANCE | WACKY_DI_FLAG)
 
// following defines the data format we tell DI to use when reporting joy state
static DIOBJECTDATAFORMAT g_joystickObjDataFormat[kJoystickObjsNum] = 
{
   { &GUID_XAxis,  FIELD_OFFSET(sJoyState, x), COMMON_FLAGS | DIDFT_AXIS, DIDOI_ASPECTPOSITION },
   { &GUID_YAxis,  FIELD_OFFSET(sJoyState, y), COMMON_FLAGS | DIDFT_AXIS, DIDOI_ASPECTPOSITION },
   { &GUID_ZAxis,  FIELD_OFFSET(sJoyState, z), COMMON_FLAGS | DIDFT_AXIS, DIDOI_ASPECTPOSITION },
   { &GUID_RzAxis, FIELD_OFFSET(sJoyState, rz), COMMON_FLAGS | DIDFT_AXIS, DIDOI_ASPECTPOSITION },
   { &GUID_Slider, FIELD_OFFSET(sJoyState, u), COMMON_FLAGS | DIDFT_AXIS, DIDOI_ASPECTPOSITION },
   { &GUID_Slider, FIELD_OFFSET(sJoyState, u), COMMON_FLAGS | DIDFT_AXIS, DIDOI_ASPECTPOSITION },
   { &GUID_POV,    FIELD_OFFSET(sJoyState, POV[0]), COMMON_FLAGS | DIDFT_POV,  0 },
   { &GUID_POV,    FIELD_OFFSET(sJoyState, POV[1]), COMMON_FLAGS | DIDFT_POV,  0 },
};

// have we set up the button object data yet?
static BOOL g_joystickObjDataInitialized = FALSE;
// actual joystick data format
static DIDATAFORMAT g_joystickDataFormat =
{
   sizeof(DIDATAFORMAT),
   sizeof(DIOBJECTDATAFORMAT),
   DIDF_ABSAXIS,
   sizeof(sJoyState),
   kJoystickObjsNum,
   g_joystickObjDataFormat
};


// recording info
static const char pszJoyStateRes[] = "JoyStateRes";
static const char pszJoyState[] = "JoyState";

// IUnknown methods
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cJoystick, IJoystick);

long cJoystick::FieldOffset(eJoystickObjs axis)
{
   switch (axis)
   {
      case kJoystickXAxis: return FIELD_OFFSET(sJoyState, x);
      case kJoystickYAxis: return FIELD_OFFSET(sJoyState, y);
      case kJoystickZAxis: return FIELD_OFFSET(sJoyState, z);
      case kJoystickZRot: return FIELD_OFFSET(sJoyState, rz);
      case kJoystickUSlider: return FIELD_OFFSET(sJoyState, u);
      case kJoystickVSlider: return FIELD_OFFSET(sJoyState, v);
      default:
         Warning(("cJoystick::FieldOffset - unknown axis\n"));
         return -1;
   }
}

//
// set joystick axis range 
//
STDMETHODIMP cJoystick::SetAxisRange(eJoystickObjs axis, long min, long max)
{
   DIPROPRANGE diprg;
   long axisOffset = FieldOffset(axis);

   Assert_(m_pDev!=NULL);
   if (axisOffset<0)
   {
      Warning(("cJoystick::SetAxisRange - invalid axis\n"));
      return E_FAIL;
   }
   diprg.diph.dwSize = sizeof(diprg);
   diprg.diph.dwHeaderSize = sizeof(diprg.diph);
   diprg.diph.dwObj = axisOffset;
   diprg.diph.dwHow = DIPH_BYOFFSET;
   diprg.lMin = min;
   diprg.lMax = max;
   if (FAILED(m_pDev->SetProperty(DIPROP_RANGE, &diprg.diph)))
   {
      Warning(("cJoystick::SetAxisRange - SetProperty() FAILED\n"));
      return E_FAIL;
   }
   return S_OK;
}

//
// get joystick axis range 
//
STDMETHODIMP cJoystick::GetAxisRange(eJoystickObjs axis, long *pMin, long *pMax)
{
   DIPROPRANGE diprg;
   long axisOffset = FieldOffset(axis);

   Assert_(m_pDev!=NULL);
   if (axisOffset<0)
   {
      Warning(("cJoystick::GetAxisRange - invalid axis\n"));
      return E_FAIL;
   }
   diprg.diph.dwSize = sizeof(diprg);
   diprg.diph.dwHeaderSize = sizeof(diprg.diph);
   diprg.diph.dwObj = axisOffset;
   diprg.diph.dwHow = DIPH_BYOFFSET;
   if (FAILED(m_pDev->GetProperty(DIPROP_RANGE, &diprg.diph)))
   {
      Warning(("cJoystick::GetAxisRange - GetProperty() FAILED\n"));
      return E_FAIL;
   }
   *pMin = diprg.lMin;
   *pMax = diprg.lMax;
   return S_OK;
}

//
// set "dead zone" as a percentage of axis range * 10000 (default is 500 = 5%)
//
STDMETHODIMP cJoystick::SetAxisDeadZone(eJoystickObjs axis, DWORD deadZone)
{
   DIPROPDWORD dipdw;
   long axisOffset = FieldOffset(axis);

   Assert_(m_pDev!=NULL);
   if (axisOffset<0)
   {
      Warning(("cJoystick::SetAxisDeadZone - invalid axis\n"));
      return E_FAIL;
   }
   dipdw.diph.dwSize = sizeof(dipdw);
   dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
   dipdw.diph.dwObj = axisOffset;
   dipdw.diph.dwHow = DIPH_BYOFFSET;
   dipdw.dwData = deadZone;
   if (FAILED(m_pDev->SetProperty(DIPROP_DEADZONE, &dipdw.diph)))
   {
      Warning(("cJoystick::SetAxisRange - SetProperty() FAILED\n"));
      return E_FAIL;
   }
   return S_OK;
}

cJoystick::cJoystick():
   m_pDev(NULL)
{
   if (!g_joystickObjDataInitialized)
   {
      int i;

      for (i=0; i<kJoyButtonsMax; i++)
      {
         g_joystickObjDataFormat[kJoyBase+i].pguid = &GUID_Button;
         g_joystickObjDataFormat[kJoyBase+i].dwOfs = (DWORD)&((sJoyState *)0)->buttons[i];
         g_joystickObjDataFormat[kJoyBase+i].dwType = COMMON_FLAGS | DIDFT_BUTTON;
         g_joystickObjDataFormat[kJoyBase+i].dwFlags = 0;
      }
      g_joystickObjDataInitialized = TRUE;
   }
}

// globalJoyDevice - good name for a band, eh?
// this is a stupid hack for Flight2 to give it access to the
// DirectInput joystick device so it can do force feedback
// This should go away when force feedback is done in a library - patmc
static LPDIRECTINPUTDEVICE2 globalJoyDevice = NULL;

//
// return ptr to ptr to DirectInputDevice2
//
void *
GetGlobalJoyDevice( void )
{
   return (void *) &globalJoyDevice;
}


// init a joystick from a LPDIRECTINPUTDEVICE
// tries to convert to LPDIRECTINPUTDEVICE2, m_pDev may be NULL if this fails
// but how could it?
STDMETHODIMP cJoystick::Init(LPDIRECTINPUTDEVICE pDev)
{
   HRESULT hRes;
   DIDEVICEOBJECTINSTANCE  didoi;
   int   i;

   SafeRelease(m_pDev);
   globalJoyDevice = NULL;
   // Convert to a Device2 so we can Poll() it.
   hRes = pDev->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&m_pDev);
   if (FAILED(hRes)) 
   {
      Warning(("cJoystick::cJoystick - can't get IDirectInputDevice2 interface\n"));
      m_pDev = NULL;
      return E_FAIL;
   }
   // set the cooperative level
   // we need the main app's HWND
   IWinApp *pWA = AppGetObj(IWinApp);
   Assert_(pWA != NULL);
   HWND hMainWnd = pWA->GetMainWnd();
   SafeRelease(pWA);
   if (FAILED(m_pDev->SetCooperativeLevel(hMainWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)))
      // TODO: put back to foreground?
   {
      Warning(("cJoystick::cJoystick - SetCooperativeLevel FAILED\n"));
      pDev->Release();
      m_pDev = NULL;
      return E_FAIL;
   }

   // set joystick data format

   //hRes = m_pDev->SetDataFormat(&c_dfDIJoystick);
   hRes = m_pDev->SetDataFormat(&g_joystickDataFormat);
   if (FAILED(hRes))
   {
      Warning(("cJoystick::cJoystick - SetDataFormat FAILED\n"));
      DIErrorSpew(hRes);
      m_pDev->Release();
      m_pDev = NULL;
      return E_FAIL;
   }

   // query the device capabilities
   mDevCaps.dwSize = sizeof( mDevCaps );
   hRes = m_pDev->GetCapabilities( &mDevCaps );
   didoi.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);
   hRes = m_pDev->GetObjectInfo( &didoi, DIJOFS_RZ, DIPH_BYOFFSET ); 
   mHasRZAxis = SUCCEEDED( hRes );
   hRes = m_pDev->GetObjectInfo( &didoi, DIJOFS_Z, DIPH_BYOFFSET ); 
   mHasZAxis = SUCCEEDED( hRes );
   mNumSliders = 0;
   for ( i = 0; i < kJoySlidersMax; i++ ) {
      hRes = m_pDev->GetObjectInfo( &didoi, DIJOFS_SLIDER(i), DIPH_BYOFFSET ); 
      if SUCCEEDED( hRes ) {
         mNumSliders++;
      }
   }

   // set default axis characteristics
//   SetAxisRange(kJoystickXAxis, -128, 127);
//   SetAxisRange(kJoystickYAxis, -128, 127);
//   SetAxisDeadZone(kJoystickXAxis, 5000);
//   SetAxisDeadZone(kJoystickYAxis, 5000);

   globalJoyDevice = m_pDev;

   // acquire the joystick - this is needed for Flight 2 force feedback
   ReacquireInput();
   return S_OK;
}


cJoystick::~cJoystick(void)
{
   HRESULT hRes;

   if ( m_pDev != NULL ) {
      hRes = m_pDev->Unacquire();
      if (FAILED(hRes))
         // unacquisition failed
         DIErrorSpew(hRes);
   }

   globalJoyDevice = NULL;      
   SafeRelease(m_pDev);
}

//
// get info about this device
//
STDMETHODIMP cJoystick::GetInfo(sInputDeviceInfo *pInfo)
{
   pInfo->interfaceID = IID_IJoystick;
   return S_OK;
}

// Reacquires the current input device.  If Acquire() returns S_FALSE,
// that means that we are already acquired and that DirectInput did nothing.
HRESULT cJoystick::ReacquireInput(void)
{
    HRESULT hRes;

    Assert_(m_pDev!=NULL);
    // acquire the device
    hRes = m_pDev->Acquire();
    if (FAILED(hRes)) {
       // acquisition failed
       DIErrorSpew(hRes);
    }
    return hRes;
}

// Poll the joystick - set state information
// Return FALSE if fail
STDMETHODIMP cJoystick::GetState(sJoyState *pState)
{
   HRESULT                 hRes;
   int                     i;

   Assert_(m_pDev!=NULL);

   // clear the "unused" hats now - this allows broken drivers which report
   //  fewer hats than they really have (like CH ForceFX) to work
   for ( i = mDevCaps.dwPOVs; i < kJoyPOVsMax; i++ ) {
      pState->POV[i] = -1;
   }

   // poll the joystick to read the current state
   hRes = m_pDev->Poll();
   // get data from the joystick
   hRes = m_pDev->GetDeviceState(sizeof(sJoyState), pState);

   if (hRes != DI_OK)
   {
      // did the read fail because we lost input for some reason?
      // if so, then attempt to reacquire.  
      if ((hRes == DIERR_INPUTLOST) || (hRes == DIERR_NOTACQUIRED))
         ReacquireInput();
      else
         DIErrorSpew(hRes);
   } 

   // clear fields which device does not support
   if ( !mHasZAxis ) {
      pState->z = 0;
   }
   if ( !mHasRZAxis ) {
      pState->rz = 0;
   }
   switch ( mNumSliders ) {
      case 0:
         pState->u = 0;
         pState->v = 0;
         break;
      case 1:
         pState->u = 0;
         break;
   }
   for ( i = mDevCaps.dwButtons; i < kJoyButtonsMax; i++ ) {
      pState->buttons[i] = 0;
   }

   // make or playback recording
   RecStreamAddOrExtract(g_pInputRecorder, &hRes, sizeof(HRESULT), pszJoyStateRes);
   RecStreamAddOrExtract(g_pInputRecorder, pState, sizeof(sJoyState), pszJoyState);

   if (hRes != DI_OK)
      return E_FAIL;
   return S_OK;
}

