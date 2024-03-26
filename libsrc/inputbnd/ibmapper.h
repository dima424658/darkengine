#pragma once

#include <aatree.h>
#include <inpbnd_i.h>
#include <str.h>
#include <dlist.h>

struct sControlDown
{
	cAnsiStr control;
	char* pCmd;
};

typedef cContDListNode<cAnsiStr, 0> cAnsiStrNode;
typedef cContDListNode<sControlDown, 0> cControlDownNode;

#pragma pack(1)
class cIBInputMapper
{
public:
	cIBInputMapper();
	~cIBInputMapper();

	char* Bind(char** tokens);
	void CDECL RemoveCmdCtrlNode(cAnsiStrNode* pList, const char* pszCtrl);
	char* Unbind(const char* control, char** var_name);
	char* PeekBind(const char* control, int strip_control);
	void TrapBind(const char* p_cmd, int (CDECL* filter_cb)(char*, char*, void*), void (CDECL* post_cb)(int), void* data);
	BOOL CDECL StaticTrapHandler(uiEvent* p_event);
	BOOL TrapHandler(uiEvent* p_event);
	char* GetControlFromCmdStart(char* pszCmd, char* pszCtrlBuf);
	char* GetControlFromCmdNext(char* pszCtrlBuf);
	char* SaveBnd(const char* filename, char* header);
	void PollAllKeys();
	void FlushKeys();
	
	cControlDownNode* CDECL GetControlDownNode(char* pControl);
	int SendButtonCmd(char* control, int down, int mod);
	int ProcessRawKey(short scancode, BOOL action, int polling);
	char* GetRawControl(short scancode, int shifted, int* strip_shift);
	int AttachMods(char* control, BOOL action, int strip_shift);
	int ProcessRawMod(char* mod, BOOL action, int general);
	BOOL GetControlMask(char(*controls)[32], int num_controls);
	BOOL GetStateMask();
	int ProcessCookedKey(short code);
	int ProcessMouseButton(_ui_mouse_event* event);
	int ProcessMouseMove(_ui_mouse_event* event);
	void StripSendDoubleCmd(char* control, long double a);
	int ProcessJoystick(_ui_joy_event* event);
	long double CDECL ShortScaleDouble(short a);
	char* DecomposeControl(char* control_str, char(*controls)[32], long* num_controls);
	void RecomposeControl(char* control_str, char(*controls)[32], int num_controls);
	void StripControl(char* dest, char* src);

public:
	static BOOL CDECL InputBindingHandler(uiEvent* p_event);

	aatree<char> m_control_binds;
	char m_misc_str[128];
	unsigned int m_context;
	unsigned int m_mod_states;
	cIBJoyAxisProcess* m_joyproc;
	unsigned char m_trapping;
	char* m_bind_cmd;
	int(*CDECL m_filter_cb)(char*, char*, void*);
	void(*CDECL m_post_cb)(int);
	void* m_filter_data;
	cAnsiStrNode m_pCtrlIterNode;
	unsigned int m_valid_events;
};
#pragma pack()