#include <comtools.h>
#include <aggmemb.h>
#include <dynarray.h>
#include <types.h>

#include <inpbnd_i.h>
#include <ibvarman.h>
#include <ibmapper.h>

cIBVariableManager* g_IB_variable_manager = nullptr;
extern cIBInputMapper* g_IB_input_mapper;

class cInputBinder : public cCTDelegating<IInputBinder>,
	public cCTAggregateMemberControl<kCTU_Default>
{
public:
	cInputBinder(IUnknown* pOuter);
	virtual ~cInputBinder();

	//
	// Can initialize binding vars and a .bnd file to load upon Init.
	// pVars and pBndFname can both be NULL.
	//
	STDMETHOD_(char*, Init) (THIS_ IB_var* pVars, char* pBndFname);
	STDMETHOD_(char*, Term) (THIS);

	//
	// Stuffs ppHandler with the input binder's ui handler.
	//
	STDMETHOD_(void, GetHandler) (THIS_ tBindHandler* ppHandler);

	//
	// Used to mask which events the binder will trap.
	// These are defined in event.h
	//
	STDMETHOD_(void, SetValidEvents) (THIS_ ulong iEventMask);
	STDMETHOD_(ulong, GetValidEvents) (THIS);

	//
	// Binds pCmd to pControl. Multiple worded commands should
	// be wrapped in quotes. Returns TRUE if all went well.
	//
	STDMETHOD_(BOOL, Bind) (THIS_ const char* pControl, const char* pCmd);

	//
	// Uninds pControl. Returns FALSE if pControl is invalid.
	//
	STDMETHOD_(BOOL, Unbind) (THIS_ const char* pControl);

	//
	// Stuffs pCmdBuf with whatever command pControl is bound to.
	//
	STDMETHOD_(void, QueryBind) (THIS_ const char* pControl, char* pCmdBuf, long iBufLen);

	//
	// Stuffs pValBuf with the value of pVarStr. Currently does not take
	// channels into account.
	//
	STDMETHOD_(void, GetVarValue) (THIS_ const char* pVarStr, char* pValBuf, long iBufLen);

	//
	// Accepts a string which contains commands
	// to interact with the binder. Most users
	// should not have to call this.
	//
	STDMETHOD_(const char*, ProcessCmd) (THIS_ const char* pCmdStr);

	//
	// Trap a control, and bind it to pCmdStr, if the filter callback allows it.
	// Both callbacks and pUserData may be NULL, however.
	//
	STDMETHOD_(void, TrapBind) (THIS_ const char* pCmdStr, tTrapBindFilter, tTrapBindPostFunc, void* pUserData);

	//
	// Merely sets the internal mapper to the correct context. 
	// Usually not needed.
	//
	STDMETHOD_(char*, Update) (THIS);

	//
	// Polls keys and sends out appropriate commands upon change of state.
	//
	STDMETHOD_(void, PollAllKeys) (THIS);

	//
	// Registers the application's joystick processor, inherited from above.
	//
	STDMETHOD_(void, RegisterJoyProcObj) (cIBJoyAxisProcess* pJoyProc);


	//
	// This is the preferred method of bind loading. You associate context bitmasks
	// and context strings with the ContextAssociate method, then LoadBndContexted
	// will parse the file only once, and map the contexted commands accordingly.
	//
	STDMETHOD_(void, LoadBndContexted) (THIS_ char* pBndFname);

	//
	// Accepts a pointer to a kBindContextNull-terminated array of sBindContext's.
	// A call to this method wipes out any previoius context associations. These
	// associations are then used when mapping contexted binds during a call to
	// LoadBndContexted.
	//
	STDMETHOD_(void, ContextAssociate) (THIS_ sBindContext* pBindContext);


	//
	// Loads a .bnd file into the specified iContext. If pPrefix is not NULL,
	// we bind only the matching prefixed binds.
	//
	STDMETHOD_(const char*, LoadBndFile) (THIS_ const char* pBndFname, unsigned long iContext, const char* pPrefix);

	//
	// Saves a bind file under the current context only. Should undoubtedly
	// change this in the future. If pHeader is non-NULL, the string will
	// be placed at the top of the file.
	//
	STDMETHOD_(char*, SaveBndFile) (THIS_ const char* pBndFname, char* pHeader);

	//
	// Methods for setting and unsetting bind command variables.
	//
	STDMETHOD_(BOOL, VarSet) (THIS_ IB_var* pVars);//must be NULL-terminated
	STDMETHOD_(BOOL, VarSetn) (THIS_ IB_var* pVars, long iNum);
	// if bUser is set, then the variable(s) can be unset by the player.
	STDMETHOD_(const char*, VarUnset) (THIS_ char** ppVarNames, BOOL bUser);//must be NULL-terminated
	STDMETHOD_(const char*, VarUnsetn) (THIS_ char** ppVarNames, long iNum, BOOL bUser);
	STDMETHOD_(const char*, VarUnsetAll) (THIS);


	//
	// For setting and getting out current context. This should be a bitmask.
	// Also, do we want to poll when changing to a different context?
	//
	STDMETHOD_(BOOL, SetContext) (THIS_ unsigned long iContext, BOOL bPoll);
	// Stuffs it
	STDMETHOD_(void, GetContext) (THIS_ unsigned long* pContext);

	//
	// Set globally master agg callback.
	// This is also set interally to IBMaxActiveAgg, so most don't need
	// to use this.
	//
	STDMETHOD_(void, SetMasterAggregation) (THIS_ tBindAggCallback);

	//
	// Set the default variable processing callback. When a command variable
	// doesn't have its own callback, or a bound command isn't an input command
	// this callback will be used. Useful for sending most commands along to
	// a game's command system.
	//
	STDMETHOD_(void, SetMasterProcessCallback) (THIS_ tBindProcCallback);

	//
	// Iterators for getting which controls are bound to a certain command.
	// pControlBuf really should be at least 32 chars long.
	//
	STDMETHOD_(char*, GetControlFromCmdStart) (THIS_ char* pCmd, char* pControlBuf);
	STDMETHOD_(char*, GetControlFromCmdNext) (THIS_ char* pControlBuf);

	//
	// Not currently used.
	//
	STDMETHOD_(void, SetResPath) (THIS_ char* pPath);

	//
	// Useful for taking a composed control (ie. "p+shift+alt") and separating
	// then into ppControls. Maximum controls is 4 (some control+alt+crtl+shift).
	// pNumControls will be stuffed with the number of controls separated.
	//
	STDMETHOD_(const char*, DecomposeControl) (THIS_ char* pControlStr, char ppControls[4][32], long* pNumControls);

	//
	// Clears all binds in the current context.
	//
	STDMETHOD_(void, Reset)(THIS);
private:
	void SetGlobObjs();

private:
	cIBVariableManager* m_IB_variable_manager;
	cIBInputMapper* m_IB_input_mappers[32];
	unsigned int m_cur_context_idx;
	cDynArray<_sBindContext> m_BndCtxtArray;
};

cInputBinder::cInputBinder(IUnknown* pOuter)
{
	MI_INIT_AGGREGATION_1(pOuter, IInputBinder, kPriorityNormal, nullptr);
}

cInputBinder::~cInputBinder()
{
}

STDMETHODIMP_(char*) cInputBinder::Init(IB_var* pVars, char* pBndFname)
{
	m_IB_variable_manager = new cIBVariableManager();

	memset(m_IB_input_mappers, 0, sizeof(m_IB_input_mappers));
	m_IB_input_mappers[0] = new cIBInputMapper();
	m_IB_input_mappers[0]->m_context = 1;
	m_cur_context_idx = 0;

	if (pVars)
		VarSet(pVars);

	if (pBndFname)
		LoadBndFile(pBndFname, 0, nullptr);

	return 0;
}

STDMETHODIMP_(char*) cInputBinder::Term(void)
{
	for (int i = 0; i < 32; ++i)
	{
		if (m_IB_input_mappers[i])
			delete m_IB_input_mappers[i];
	}

	if (m_IB_variable_manager)
		delete m_IB_variable_manager;

	m_BndCtxtArray.SetSize(0);

	return nullptr;
}

STDMETHODIMP_(void) cInputBinder::GetHandler(tBindHandler* ppHandler)
{
	SetGlobObjs();
	*ppHandler = cIBInputMapper::InputBindingHandler;
}

STDMETHODIMP_(void) cInputBinder::SetValidEvents(ulong iEventMask)
{
	SetGlobObjs();
	g_IB_input_mapper->m_valid_events = iEventMask;
}

STDMETHODIMP_(ulong) cInputBinder::GetValidEvents()
{
	SetGlobObjs();
	return g_IB_input_mapper->m_valid_events;
}

STDMETHODIMP_(BOOL) cInputBinder::Bind(const char* pControl, const char* pCmd)
{
	char pBuf[256] = {};

	snprintf(pBuf, 256, "bind %s %s", pControl, pCmd);
	return ProcessCmd(pBuf) == 0;
}

STDMETHODIMP_(BOOL) cInputBinder::Unbind(const char* pControl)
{
	char pBuf[256] = {};

	snprintf(pBuf, 256, "unbind %s", pControl);
	return ProcessCmd(pBuf) == 0;
}

STDMETHODIMP_(void) cInputBinder::QueryBind(const char* pControl, char* pCmdBuf, long iBufLen)
{
	char pBuf[256] = {};

	snprintf(pBuf, 256, "bind %s", pControl);
	strncpy(pCmdBuf, ProcessCmd(pBuf), iBufLen);
}

STDMETHODIMP_(void) cInputBinder::GetVarValue(const char* pVarStr, char* pValBuf, long iBufLen)
{
	char pBuf[256] = {};

	snprintf(pBuf, 256, "echo $%s", pVarStr);
	strncpy(pValBuf, ProcessCmd(pBuf), iBufLen);
}

STDMETHODIMP_(const char*) cInputBinder::ProcessCmd(const char* pCmdStr)
{
	SetGlobObjs();
	return m_IB_variable_manager->Cmd(pCmdStr, 0);
}

STDMETHODIMP_(void) cInputBinder::TrapBind(const char* pCmdStr, tTrapBindFilter filter_cb, tTrapBindPostFunc post_cb, void* pUserData)
{
	SetGlobObjs();
	g_IB_input_mapper->TrapBind(pCmdStr, filter_cb, post_cb, pUserData);
}

STDMETHODIMP_(char*) cInputBinder::Update()
{
	SetGlobObjs();
	return nullptr;
}

STDMETHODIMP_(void) cInputBinder::PollAllKeys()
{
	SetGlobObjs();
	g_IB_input_mapper->PollAllKeys();
}

STDMETHODIMP_(void) cInputBinder::RegisterJoyProcObj(cIBJoyAxisProcess* pJoyProc)
{
	SetGlobObjs();
	g_IB_input_mapper->m_joyproc = pJoyProc;
}

STDMETHODIMP_(void) cInputBinder::LoadBndContexted(char* pBndFname)
{
	m_IB_variable_manager->LoadBndContexted(pBndFname, m_BndCtxtArray.AsPointer(), m_BndCtxtArray.Size(), m_IB_input_mappers);
	SetGlobObjs();
}

STDMETHODIMP_(void) cInputBinder::ContextAssociate(sBindContext* pBindContext)
{
	sBindContext ctxtNull = {};

	m_BndCtxtArray.SetSize(0);
	for (int i = 0; memcmp(&pBindContext[i], &ctxtNull, sizeof(sBindContext)); ++i)
		m_BndCtxtArray.Append(pBindContext[i]);
}

STDMETHODIMP_(const char*) cInputBinder::LoadBndFile(const char* pBndFname, unsigned long iContext, const char* pPrefix)
{
	char str[128] = {};

	strcpy(str, "loadbnd ");
	strcat(str, pBndFname);

	SetGlobObjs();
	if (!iContext)
		return m_IB_variable_manager->Cmd(str, 0);

	int old_context = 1 << m_cur_context_idx;
	int bit = 0;
	while (iContext)
	{
		if ((iContext & 1) != 0)
		{
			if (m_IB_input_mappers[bit])
			{
				SetContext(1 << bit, TRUE);
				m_IB_variable_manager->LoadBnd(pBndFname, pPrefix);
			}
		}
		++bit;
		iContext >>= 1;
	}

	SetContext(old_context, TRUE);
	return nullptr;
}

STDMETHODIMP_(char*) cInputBinder::SaveBndFile(const char* pBndFname, char* pHeader)
{
	SetGlobObjs();
	g_IB_input_mapper->SaveBnd(pBndFname, pHeader);
	return nullptr;
}

STDMETHODIMP_(BOOL) cInputBinder::VarSet(IB_var* pVars)
{
	SetGlobObjs();
	return m_IB_variable_manager->VarSet(pVars, 0);
}

STDMETHODIMP_(BOOL) cInputBinder::VarSetn(IB_var* pVars, long iNum)
{
	SetGlobObjs();
	return m_IB_variable_manager->VarSet(pVars, iNum, 1);
}

STDMETHODIMP_(const char*) cInputBinder::VarUnset(char** ppVarNames, BOOL bUser)
{
	SetGlobObjs();
	return m_IB_variable_manager->VarUnset(ppVarNames, bUser);
}

STDMETHODIMP_(const char*) cInputBinder::VarUnsetn(char** ppVarNames, long iNum, BOOL bUser)
{
	SetGlobObjs();
	return m_IB_variable_manager->VarUnset(ppVarNames, iNum, bUser);
}

STDMETHODIMP_(const char*) cInputBinder::VarUnsetAll()
{
	return m_IB_variable_manager->VarUnsetAll();
}

STDMETHODIMP_(BOOL) cInputBinder::SetContext(unsigned long iContext, BOOL bPoll)
{
	if (m_IB_input_mappers[m_cur_context_idx]->m_context == iContext)
		return TRUE;

	if (!iContext || (iContext & -iContext) != iContext)
		return FALSE;

	int bit = 0;
	while ((iContext & 1) == 0)
	{
		++bit;
		iContext >>= 1;
	}

	m_cur_context_idx = bit;
	if (m_IB_input_mappers[m_cur_context_idx])
	{
		m_IB_input_mappers[m_cur_context_idx]->m_mod_states = 0;
		SetGlobObjs();
		if (bPoll)
			PollAllKeys();
		return TRUE;
	}

	m_IB_input_mappers[m_cur_context_idx] = new cIBInputMapper();
	m_IB_input_mappers[m_cur_context_idx]->m_context = 1 << bit;
	SetGlobObjs();
	return TRUE;
}

STDMETHODIMP_(void) cInputBinder::GetContext(unsigned long* pContext)
{
	*pContext = m_IB_input_mappers[m_cur_context_idx]->m_context;
}

STDMETHODIMP_(void) cInputBinder::SetMasterAggregation(tBindAggCallback func)
{
	SetGlobObjs();
	m_IB_variable_manager->SetMasterAggregation(func);
}

STDMETHODIMP_(void) cInputBinder::SetMasterProcessCallback(tBindProcCallback func)
{
	SetGlobObjs();
	m_IB_variable_manager->SetMasterProcessCallback(func);
}

STDMETHODIMP_(char*) cInputBinder::GetControlFromCmdStart(char* pCmd, char* pControlBuf)
{
	SetGlobObjs();
	return m_IB_input_mappers[m_cur_context_idx]->GetControlFromCmdStart(pCmd, pControlBuf);
}

STDMETHODIMP_(char*) cInputBinder::GetControlFromCmdNext(char* pControlBuf)
{
	SetGlobObjs();
	return m_IB_input_mappers[m_cur_context_idx]->GetControlFromCmdNext(pControlBuf);
}

STDMETHODIMP_(void) cInputBinder::SetResPath(char* pPath)
{
	SetGlobObjs();
	m_IB_variable_manager->SetBndSearchPath(pPath);
}

STDMETHODIMP_(const char*) cInputBinder::DecomposeControl(char* pControlStr, char ppControls[4][32], long* pNumControls)
{
	SetGlobObjs();

	assert(m_IB_input_mappers[m_cur_context_idx] != nullptr);
	return m_IB_input_mappers[m_cur_context_idx]->DecomposeControl(pControlStr, ppControls, pNumControls);
}

STDMETHODIMP_(void) cInputBinder::Reset(void)
{
	if (m_IB_input_mappers[m_cur_context_idx])
	{
		delete m_IB_input_mappers[m_cur_context_idx];
		m_IB_input_mappers[m_cur_context_idx] = new cIBInputMapper();
		m_IB_input_mappers[m_cur_context_idx]->m_context = 1 << m_cur_context_idx;
		SetGlobObjs();
	}
}

void cInputBinder::SetGlobObjs()
{
	g_IB_variable_manager = m_IB_variable_manager;
	g_IB_input_mapper = m_IB_input_mappers[m_cur_context_idx];
}

tResult LGAPI _CreateInputBinder(REFIID, IInputBinder** ppInputBinder, IUnknown* pOuter)
{
	assert(ppInputBinder != nullptr);

	*ppInputBinder = new cInputBinder(pOuter);

	return *ppInputBinder ? NOERROR : E_FAIL;
}