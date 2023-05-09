#include <loopapi.h>

class cGenericLoopClient : public cLoopClient<kCTU_Default>
{
public:
	cGenericLoopClient(tLoopClientReceiveMessageFunc pCallback,
		void* pContext,
		const sLoopClientDesc* pClientDesc)
		: cLoopClient{ pClientDesc }, m_pCallback{ pCallback }, m_pContext{ pContext } {}

	virtual ~cGenericLoopClient() = default;

	// Handle a message from the owning loop mode dispatcher
	eLoopMessageResult ReceiveMessage(eLoopMessage hMessage, tLoopMessageData hData) override
	{
		return m_pCallback(m_pContext, hMessage, hData);
	}

private:
	tLoopClientReceiveMessageFunc m_pCallback;
	void* m_pContext;
};

ILoopClient* LGAPI CreateSimpleLoopClient(tLoopClientReceiveMessageFunc pCallback, void* pContext, const sLoopClientDesc* pClientDesc)
{
	return new cGenericLoopClient(pCallback, pContext, pClientDesc);
}