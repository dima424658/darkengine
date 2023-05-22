#pragma once

#include <loopapi.h>
#include <loopstk.h>
#include <aggmemb.h>

class cLoopManager;

class cLoop : public ILoop
{
public:
	static cLoop* gm_pLoop;

public:
	// Constructor
	cLoop(IUnknown* pOuterUnknown, cLoopManager* pLoopManager);

	DECLARE_SIMPLE_AGGREGATION(cLoop);


	// Run the loop
	STDMETHOD_(int, Go)(THIS_ sLoopInstantiator* loop) override;


	// Unwind stack and exit loop at end of current loop iteration
	STDMETHOD(EndAllModes)(THIS_ int goRetVal) override;


	// Clean up all outstanding loops and prepare to shut down the app.
	// Only intended to be used for cleaning up state right before calling exit();
	STDMETHOD(Terminate)(THIS) override;


	// Query for current frame info. Values undefined if not actually looping
	STDMETHOD_(const sLoopFrameInfo*, GetFrameInfo)(THIS) override;


	// Change mode at end of current loop iteration
	STDMETHOD(ChangeMode)(THIS_ eLoopModeChangeKind, sLoopInstantiator* loop) override;


	// End current mode at end of current loop iteration,
	// exit loop if current mode is bottom of stack
	STDMETHOD(EndMode)(THIS_ int goRetVal) override;


	// Get the currently running mode
	STDMETHOD_(ILoopMode*, GetCurrentMode)(THIS) override;


	// Get the current mode's dispatcher
	STDMETHOD_(ILoopDispatch*, GetCurrentDispatch)(THIS) override;


	// Pause/unpause the game
	STDMETHOD_(void, Pause)(THIS_ BOOL) override;
	STDMETHOD_(BOOL, IsPaused)(THIS) override;

	// Change the minor mode
	STDMETHOD(ChangeMinorMode)(THIS_ int minorMode) override;
	STDMETHOD_(int, GetMinorMode)(THIS) override;

	// Convenience functions to dispatch messsages to current loop mode dispatcher
	STDMETHOD(SendMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags) override;
	STDMETHOD(SendSimpleMessage)(THIS_ eLoopMessage) override;
	STDMETHOD(PostMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags) override;
	STDMETHOD(PostSimpleMessage)(THIS_ eLoopMessage) override;
	STDMETHOD(ProcessQueue)(THIS) override;

	// Debugging features
#ifndef SHIP
	STDMETHOD_(void, SetDiagnostics)(THIS_ uint fDiagnostics, tLoopMessageSet messages) override;
	STDMETHOD_(void, GetDiagnostics)(THIS_ uint* pfDiagnostics, tLoopMessageSet* pMessages) override;

	// Set messages and optional client to use the profileable dispatcher
	STDMETHOD_(void, SetProfile)(THIS_ tLoopMessageSet messages, tLoopClientID*) override;
	STDMETHOD_(void, GetProfile)(THIS_ tLoopMessageSet* pMessages, tLoopClientID**) override;
#endif

	static CDECL void OnExit();

private:
	ILoopDispatch* m_pCurrentDispatch;
	unsigned int m_fState;
	sLoopFrameInfo m_FrameInfo;

	int m_fNewMinorMode;
	int m_fGoReturn;

	ILoopManager* m_pLoopManager;
	cLoopModeStack* m_pLoopStack;
	ILoopDispatch* m_pNextDispatch;

	uint m_fTempDiagnostics;
	ulong m_tempDiagnosticSet;
	
	ulong m_TempProfileSet;
	tLoopClientID* m_pTempProfileClientId;
};