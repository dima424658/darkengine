#include <memory>

#include <loopapi.h>
#include <loopdisp.h>
#include <lgassert.h>

class cLoopMode : public cCTUnaggregated<ILoopMode, &IID_ILoopMode, kCTU_Default>
{
public:
	cLoopMode(const sLoopModeDesc* pDescription)
	{
		AssertMsg(pDescription->ppClientIDs, "Empty loop modes are not supported");

		m_desc = *pDescription;
		m_desc.ppClientIDs = new tLoopClientID * [m_desc.nClients];
		memcpy(m_desc.ppClientIDs, pDescription->ppClientIDs, sizeof(tLoopClientID*) * pDescription->nClients);
	}

	virtual ~cLoopMode()
	{
		delete m_desc.ppClientIDs;
		m_desc.ppClientIDs = nullptr;
	}

	// Get the info on the loop mode
	STDMETHOD_(const sLoopModeName*, GetName)() override
	{
		return &m_desc.name;
	}

	// Create the mode dispatch chain
	STDMETHOD(CreateDispatch)(sLoopModeInitParmList paramList, ILoopDispatch** dispatch) override
	{
		return CreatePartialDispatch(paramList, -1u, dispatch);
	}

	// int a1, _DWORD *Src, int a3, int *a4
	// Create a dispatch chain that only handles certain messages
	STDMETHOD(CreatePartialDispatch)(sLoopModeInitParmList paramList, tLoopMessageSet messageSet, ILoopDispatch** dispatch) override
	{
		*dispatch = new cLoopDispatch{ this, paramList, messageSet };
		if (*dispatch)
			return S_OK;
		else
			return E_FAIL;
	}

	// Describe this mode
	STDMETHOD_(const sLoopModeDesc*, Describe)() override
	{
		return &m_desc;
	}

private:
	sLoopModeDesc m_desc;
};

ILoopMode* LGAPI _LoopModeCreate(const sLoopModeDesc* pDescription)
{
	return new cLoopMode{ pDescription };
}