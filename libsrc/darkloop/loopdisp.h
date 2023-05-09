#pragma once

#include <loopapi.h>

class cLoopDispatch : public cCTUnaggregated<ILoopDispatch, &IID_ILoopDispatch, kCTU_Default>
{
public:
    cLoopDispatch(ILoopMode* mode, sLoopModeInitParmList paramList, tLoopMessageSet messageSet);

    // Send a message down/up the dispatch chain
    HRESULT STDMETHODCALLTYPE SendMessage(eLoopMessage, tLoopMessageData hData, int flags) override;

    // Send a message forward down the dispatch chain with no data
    HRESULT STDMETHODCALLTYPE SendSimpleMessage(eLoopMessage) override;

    // Post a message to be sent down/up the dispatch chain
    // the next time ProcessQueue() is called
    HRESULT STDMETHODCALLTYPE PostMessage(eLoopMessage, tLoopMessageData hData, int flags) override;

    // Post a message to be sent forward down the dispatch chain with no data
    // the next time ProcessQueue() is called
    HRESULT STDMETHODCALLTYPE PostSimpleMessage(eLoopMessage) override;

    // Send all posted messages in queue
    HRESULT STDMETHODCALLTYPE ProcessQueue() override;

    // Describe my instantiaion
    const sLoopModeName* STDMETHODCALLTYPE Describe(sLoopModeInitParmList* list) override;

#ifndef SHIP
    // Set/Get the diagnostic mode
    void STDMETHODCALLTYPE SetDiagnostics(unsigned fDiagnostics, tLoopMessageSet messages) override;
    void STDMETHODCALLTYPE GetDiagnostics(unsigned* pfDiagnostics, tLoopMessageSet* pMessages) override;

    // Set messages and optional client to use the profileable dispatcher
    void STDMETHODCALLTYPE SetProfile(tLoopMessageSet messages, tLoopClientID*) override;
    void STDMETHODCALLTYPE GetProfile(tLoopMessageSet* pMessages, tLoopClientID**) override;

    void STDMETHODCALLTYPE ClearTimers() override;
    void STDMETHODCALLTYPE DumpTimerInfo() override;
#endif
};