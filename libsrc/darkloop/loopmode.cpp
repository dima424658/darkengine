#include <memory>

#include <loopapi.h>
#include <loopdisp.h>
#include <lgassert.h>

class cLoopMode : public cCTUnaggregated<ILoopMode, &IID_ILoopMode, kCTU_Default>
{
public:
	cLoopMode(sLoopModeDesc* desc)
	{
		AssertMsg(desc->ppClientIDs, "Empty loop modes are not supported");
		std::memcpy(&m_desc, desc, sizeof(sLoopModeDesc));
	}

	virtual ~cLoopMode() = default;

	// Get the info on the loop mode
	const sLoopModeName* STDMETHODCALLTYPE GetName() override
	{
		return &m_desc.name;
	}

	// Create the mode dispatch chain
	HRESULT STDMETHODCALLTYPE CreateDispatch(sLoopModeInitParmList paramList, ILoopDispatch** dispatch) override
	{
		return CreatePartialDispatch(paramList, -1, dispatch);
	}

	// int a1, _DWORD *Src, int a3, int *a4
	// Create a dispatch chain that only handles certain messages
	HRESULT STDMETHODCALLTYPE CreatePartialDispatch(sLoopModeInitParmList paramList, tLoopMessageSet messageSet, ILoopDispatch** dispatch) override
	{
		*dispatch = new cLoopDispatch(this, paramList, messageSet);
		if (*dispatch)
			return S_OK;
		else
			return E_FAIL;
	}

	// Describe this mode
	const sLoopModeDesc* STDMETHODCALLTYPE Describe() override
	{
		return &m_desc;
	}

private:
	sLoopModeDesc m_desc;
};

