#pragma once

#include <aatree.h>
#include <hashset.h>
#include <inpbnd_i.h>
#include <str.h>
#include <dlist.h>

struct sControlDown
{
	cAnsiStr control;
	const char* pCmd;
};

typedef cContainerDList<sControlDown, 0> cControlDownList;
typedef cContDListNode<sControlDown, 0> cControlDownNode;

typedef cContainerDList<cAnsiStr, 0> cAnsiStrList;
typedef cContDListNode<cAnsiStr, 0> cAnsiStrNode;

struct sCmdCtrlEntry
{
	cAnsiStr cmd;
	cAnsiStrList ctrlList;
};

class cCmdCtrlHash : public cStrIHashSet<sCmdCtrlEntry*>
{
public:
	cCmdCtrlHash()
		: cStrIHashSet<sCmdCtrlEntry*>{ kHS_Medium } {}

	tHashSetKey GetKey(tHashSetNode pItem) const override
	{
		return reinterpret_cast<tHashSetKey>(reinterpret_cast<sCmdCtrlEntry*>(pItem)->cmd.GetBuffer(0));
	}

	void DestroyAll() override;
};


#pragma pack(1)
class cIBInputMapper
{
public:
	cIBInputMapper();
	~cIBInputMapper();

	const char* Bind(char** tokens);
	const char* Unbind(const char* control, char** var_name);
	const char* PeekBind(const char* control, int strip_control);
	void TrapBind(const char* p_cmd, tTrapBindFilter filter_cb, tTrapBindPostFunc post_cb, void* data);
	BOOL TrapHandler(uiEvent* p_event);
	char* GetControlFromCmdStart(char* pszCmd, char* pszCtrlBuf);
	char* GetControlFromCmdNext(char* pszCtrlBuf);
	const char* SaveBnd(const char* filename, char* header);
	void PollAllKeys();

	const char* DecomposeControl(const char* control_str, char(*controls)[32], long* num_controls);
	void StripControl(char* dest, const char* src);
	void RecomposeControl(char* control_str, char(*controls)[32], int num_controls);

	static BOOL CDECL InputBindingHandler(uiEvent* p_event);
private:
	static void CDECL RemoveCmdCtrlNode(cAnsiStrList* pList, const char* pszCtrl);
	static BOOL CDECL StaticTrapHandler(uiEvent* p_event, Region* r, void* state);
	static cControlDownNode* CDECL GetControlDownNode(const char* pControl);
	static double CDECL ShortScaleDouble(short a);

	void FlushKeys();
	
	int SendButtonCmd(const char* control, int down, int mod);
	int AttachMods(char* control, BOOL action, int strip_shift);

	char* GetRawControl(short scancode, int shifted, int* strip_shift);
	uint8 GetControlMask(char(*controls)[32], int num_controls);
	uint8 GetStateMask();

	int ProcessRawKey(short scancode, BOOL action, int polling);
	int ProcessRawMod(const char* mod, BOOL action, BOOL general);
	int ProcessCookedKey(short code);
	int ProcessMouseButton(uiMouseEvent* event);
	int ProcessMouseMove(uiMouseEvent* event);
	int ProcessJoystick(uiJoyEvent* event);

	void StripSendDoubleCmd(const char* control, long double a);

public:
	aatree<char> m_control_binds;
	char m_misc_str[128];
	unsigned int m_context;
	unsigned int m_mod_states;
	cIBJoyAxisProcess* m_joyproc;
	bool m_trapping;
	const char* m_bind_cmd;
	tTrapBindFilter m_filter_cb;
	tTrapBindPostFunc m_post_cb;
	void* m_filter_data;
	cAnsiStrNode* m_pCtrlIterNode;
	unsigned int m_valid_events;
};
#pragma pack()