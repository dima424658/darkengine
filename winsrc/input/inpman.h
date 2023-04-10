// $Header: x:/prj/tech/winsrc/input/RCS/inpman.h 1.1 1997/10/06 20:44:42 JON Exp $

#include <joynew.h>   // for some strange reason, this must come before inpapi.h?!?
#include <inpapi.h>
#include <inpbase.h>
#include <aggmemb.h>
#include <dlist.h>

class cInputManager;
class cDeviceIter;

//
// Device list implementation
//
typedef cContainerDList<IInputDevice*, 0> cInputDeviceList;
typedef cContDListNode<IInputDevice*, 0> cInputDeviceNode;

class cInputManager: public IInputManager
{
public:
    cInputManager(IUnknown *pOuterUnknown);

    ///////////////////////////////////
    //
    // Device iteration
    //
    STDMETHOD_(void, IterStart) (sInputDeviceIter *pIter, GUID interfaceID);
    STDMETHOD_(void, IterNext) (sInputDeviceIter *pIter);
    STDMETHOD_(IInputDevice*, IterGet) (sInputDeviceIter *pIter);
    STDMETHOD_(BOOL, IterFinished) (sInputDeviceIter *pIter);

    ///////////////////////////////////
    //
    // Aggregate protocol
    //
    HRESULT Init();
    HRESULT End();

private:
    ///////////////////////////////////
    //
    // IUnknown methods
    //
    DECLARE_AGGREGATION(cInputManager);

    ///////////////////////////////////
    //
    // Devices
    //
    cInputDeviceList m_devList;

    HRESULT SetDIDwordProperty(LPDIRECTINPUTDEVICE pDev, REFGUID guidProperty,
                                 DWORD dwObject, DWORD dwHow, DWORD dwValue);
    BOOL FindType(sInputDeviceIter *pIter, GUID interfaceID);
   
    LPDIRECTINPUT m_pDI;
    
    friend BOOL CALLBACK InitJoystickInput(LPCDIDEVICEINSTANCE pDInst, LPVOID pRef);
};

