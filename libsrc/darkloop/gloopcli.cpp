#include <loopapi.h>
#include <aggmemb.h>

class cGenericLoopClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default>
{
public:
	cGenericLoopClient(tLoopClientReceiveMessageFunc pCallback,
		void* pContext,
		const sLoopClientDesc* pClientDesc)
		: m_pCallback{ pCallback }, m_pDesc{ pClientDesc }, m_pContext{ pContext } {}

	virtual ~cGenericLoopClient() = default;

	//
	// Get the version of the loop client interface
	//
	STDMETHOD_(short, GetVersion)() override
	{
		return 1;
	}

	//
	// Get the loop client information
	//
	STDMETHOD_(const sLoopClientDesc*, GetDescription)() override
	{
		return m_pDesc;
	}

	// Handle a message from the owning loop mode dispatcher
	STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage hMessage, tLoopMessageData hData) override
	{
		return m_pCallback(m_pContext, hMessage, hData);
	}

private:
	tLoopClientReceiveMessageFunc m_pCallback;
	const sLoopClientDesc* m_pDesc;
	void* m_pContext;
};

ILoopClient* LGAPI CreateSimpleLoopClient(tLoopClientReceiveMessageFunc pCallback, void* pContext, const sLoopClientDesc* pClientDesc)
{
	return new cGenericLoopClient{ pCallback, pContext, pClientDesc };
}