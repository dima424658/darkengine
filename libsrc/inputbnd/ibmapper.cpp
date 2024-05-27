#include <algorithm>

#include "ibmapper.h"
#include <kbcook.h>
#include <ibvarman.h>

// implement hash sets
#include <hshsttem.h>

void cCmdCtrlHash::DestroyAll()
{
	if (!m_nItems)
		return;

	for (uint i = 0; i < m_nPts; i++)
	{
		sHashSetChunk* pNext;
		for (sHashSetChunk* p = m_Table[i]; p; p = pNext)
		{
			pNext = p->pNext;
			auto* pEntry = reinterpret_cast<sCmdCtrlEntry*>(p->node);
			auto pList = pEntry->ctrlList;
			while (true)
			{
				auto* pListNode = pList.GetFirst();
				if (!pListNode)
					break;
				delete pListNode;
			}
			delete pEntry;
			delete p;
		}

		m_Table[i] = 0;
	}

	m_nItems = 0;
}

typedef struct _input_code
{
	const char* name;
	short code;
} input_code;

EXTERN int kbd_modifier_state;
extern cIBVariableManager* g_IB_variable_manager;
cIBInputMapper* g_IB_input_mapper = nullptr;

int16 word_B25882[];
int16* word_B25884;

static uint8 mouseMask = 255;

const char* g_mods[3][3]{
	{
		"+shift", "+lshift", "+rshift"
	},
	{
		"+ctrl", "+lctrl", "+rctrl",
	},
	{
		"+alt", "+lalt", "+ralt"
	}
};

aatree<short> g_input_codes{};
aatree<char> g_input_controls[3]{};
aatree<unsigned char> g_input_down{};
cControlDownList g_ctrldown_list{};
cCmdCtrlHash g_CmdCtrlHash{};

int g_id{};
short g_prev_mousex{};
short g_prev_mousey{};
unsigned short g_shift_to_scan[0xE0];

#define MASK_CAPS_LOCK 0x000004
#define MASK_LSHIFT    0x000008
#define MASK_RSHIFT    0x000010
#define MASK_SHIFT     (MASK_LSHIFT & MASK_RSHIFT)
#define MASK_LCTRL     0x000020
#define MASK_RCTRL     0x000040
#define MASK_CTRL      (MASK_LCTRL & MASK_RCTRL)
#define MASK_LALT      0x000080
#define MASK_RALT      0x000100
#define MASK_ALT       (MASK_LALT & MASK_RALT)

#define SCANCODE_LCTRL 29
#define SCANCODE_RCTRL 157
#define SCANCODE_LSHIFT 42
#define SCANCODE_RSHIFT 54
#define SCANCODE_LALT 56
#define SCANCODE_RALT 184
#define SCANCODE_CAPS_LOCK 58

input_code g_valid_input_controls[202] = {
	{"esc", 27},
	{"pause", 2175},
	{"print_screen", 10295},
	{"scroll_lock", 2118},
	{"1", 0},
	{"2", 0},
	{"3", 0},
	{"4", 0},
	{"5", 0},
	{"6", 0},
	{"7", 0},
	{"8", 0},
	{"9", 0},
	{"0", 0},
	{"!", 0},
	{"@", 0},
	{"#", 0},
	{"$", 0},
	{"%", 0},
	{"^", 0},
	{"&", 0},
	{"*", 0},
	{"(", 0},
	{")", 0},
	{"a", 0},
	{"b", 0},
	{"c", 0},
	{"d", 0},
	{"e", 0},
	{"f", 0},
	{"g", 0},
	{"h", 0},
	{"i", 0},
	{"j", 0},
	{"k", 0},
	{"l", 0},
	{"m", 0},
	{"n", 0},
	{"o", 0},
	{"p", 0},
	{"q", 0},
	{"r", 0},
	{"s", 0},
	{"t", 0},
	{"u", 0},
	{"v", 0},
	{"w", 0},
	{"x", 0},
	{"y", 0},
	{"z", 0},
	{"`", 0},
	{"-", 0},
	{"=", 0},
	{"[", 0},
	{"]", 0},
	{"\\", 0},
	{";", 0},
	{"'", 0},
	{",", 0},
	{".", 0},
	{"/", 0},
	{"~", 0},
	{"_", 0},
	{"+", 0},
	{"{", 0},
	{"}", 0},
	{"|", 0},
	{":", 0},
	{"quote", 34},
	{"<", 0},
	{">", 0},
	{"?", 0},
	{"\x81", 0}, // ü
	{"\x82", 0}, // é
	{"\x80", 0}, // Ç
	{"\x83", 0}, // â
	{"\x84", 0}, // ä
	{"\x85", 0}, // à
	{"\x86", 0}, // å
	{"\x87", 0}, // ç
	{"\x88", 0}, // ê
	{"\x89", 0}, // ë
	{"\x8A", 0}, // è
	{"\x8B", 0}, // ï
	{"\x8C", 0}, // î
	{"\x8D", 0}, // ì
	{"\x93", 0}, // ô
	{"\x94", 0}, // ö
	{"\x95", 0}, // ò
	{"\x96", 0}, // û
	{"\x97", 0}, // ù
	{"\x98", 0}, // ÿ
	{"super_2", 253},
	{"\x0E1", 0}, // ß
	{"f1", 2107},
	{"f2", 2108},
	{"f3", 2109},
	{"f4", 2110},
	{"f5", 2111},
	{"f6", 2112},
	{"f7", 2113},
	{"f8", 2114},
	{"f9", 2115},
	{"f10", 2116},
	{"f11", 2135},
	{"f12", 2136},
	{"backspace", 8},
	{"tab", 9},
	{"enter", 13},
	{"space", 32},
	{"ins", 10322},
	{"del", 10323},
	{"home", 10311},
	{"end", 10319},
	{"pgup", 10313},
	{"pgdn", 10321},
	{"up", 10312},
	{"down", 10320},
	{"left", 10315},
	{"right", 10317},
	{"keypad_ins", 2130},
	{"keypad_del", 2131},
	{"keypad_end", 2127},
	{"keypad_down", 2128},
	{"keypad_pgdn", 2129},
	{"keypad_left", 2123},
	{"keypad_center", 2124},
	{"keypad_right", 2125},
	{"keypad_home", 2119},
	{"keypad_up", 2120},
	{"keypad_pgup", 2121},
	{"keypad_slash", 8239},
	{"keypad_star", 8234},
	{"keypad_minus", 8237},
	{"keypad_plus", 8235},
	{"keypad_enter", 8205},
	{"alt", -1},
	{"ctrl", -1},
	{"shift", -1},
	{"lalt", -1},
	{"lctrl", -1},
	{"lshift", -1},
	{"ralt", -1},
	{"rctrl", -1},
	{"rshift", -1},
	{"mouse1", 2},
	{"mouse2", 8},
	{"mouse3", 32},
	{"mouse4", -1},
	{"mousesingle1", 1024},
	{"mousesingle2", 2048},
	{"mousesingle3", 4096},
	{"mousesingle4", -1},
	{"mousedouble1", 128},
	{"mousedouble2", 256},
	{"mousedouble3", 512},
	{"mousedouble4", -1},
	{"mouse_axisx", -1},
	{"mouse_axisy", -1},
	{"mouse_axisz", -1},
	{"mouse_wheel", -1},
	{"joy1", 0},
	{"joy2", 0},
	{"joy3", 0},
	{"joy4", 0},
	{"joy5", 0},
	{"joy6", 0},
	{"joy7", 0},
	{"joy8", 0},
	{"joy9", 0},
	{"joy10", 0},
	{"joy11", 0},
	{"joy12", 0},
	{"joy13", 0},
	{"joy14", 0},
	{"joy15", 0},
	{"joy16", 0},
	{"joy17", 0},
	{"joy18", 0},
	{"joy19", 0},
	{"joy20", 0},
	{"joy21", 0},
	{"joy22", 0},
	{"joy23", 0},
	{"joy24", 0},
	{"joy25", 0},
	{"joy26", 0},
	{"joy27", 0},
	{"joy28", 0},
	{"joy29", 0},
	{"joy30", 0},
	{"joy_axisx", -1},
	{"joy_axisy", -1},
	{"joy_axisz", -1},
	{"joy_axisr", -1},
	{"joy_hataxisx", -1},
	{"joy_hataxisy", -1},
	{"joy_hatup", -1},
	{"joy_hatdn", -1},
	{"joy_hatrt", -1},
	{"joy_hatlt", -1},
	{NULL, 0} };

cIBInputMapper::cIBInputMapper()
	: m_control_binds{}, m_mod_states{ 0 }, m_trapping{ false }, m_valid_events{ std::numeric_limits<uint>::max() }, m_pCtrlIterNode{ nullptr }, m_joyproc{ nullptr }
{
	uiSetMouseMotionPolling(true);

	if (!g_input_codes.GetNumNodes())
	{
		for (input_code* cur_control = g_valid_input_controls; cur_control->name != nullptr; ++cur_control)
		{
			auto* code = new short;
			if (cur_control->code)
				*code = cur_control->code;
			else
				*code = cur_control->name[0];

			g_input_codes.Add(cur_control->name, code, 1);

			auto* control_name = new char[strlen(cur_control->name) + 2];
			strcpy(control_name, cur_control->name);

			char code_str[16] = {};
			sprintf(code_str, "%d", *code);

			if (!strncmp("mouse", control_name, 5))
			{
				g_input_controls[1].Add(code_str, control_name, strlen(control_name) + 1);
			}
			else if (!strncmp("joy", control_name, 3))
			{
				g_input_controls[2].Add(code_str, control_name, strlen(control_name) + 1);
			}
			else
			{
				g_input_controls[0].Add(code_str, control_name, strlen(control_name) + 1);
			}
		}
	}

	for (int i = 0; i < 224; ++i)
	{
		g_shift_to_scan[i] = 0;
		for (uint16 j = 0; j < 224; ++j)
		{
			if (i == kb_cnv_table[j][0])
			{
				g_shift_to_scan[i] = j;
				break;
			}
		}
	}

	g_IB_input_mapper = this;
}

cIBInputMapper::~cIBInputMapper()
{
}

const char* cIBInputMapper::Bind(char** tokens)
{
	char controls[4][32] = {};
	long num_controls = 0;
	auto* ret_val = DecomposeControl(*tokens, controls, &num_controls);
	if (ret_val)
		return ret_val;

	RecomposeControl(m_misc_str, controls, num_controls);

	delete[] tokens[0];
	tokens[0] = new char[strlen(m_misc_str) + 1] {};
	strcpy(tokens[0], m_misc_str);

	auto* cmd = m_control_binds.Find(m_misc_str);
	if (cmd)
	{
		delete[] cmd;
	}
	else
	{
		m_control_binds.Add(m_misc_str, 0, 1);
		g_input_down.Add(m_misc_str, new uchar[1]{}, 1);
	}

	cmd = tokens[1];
	auto* pCmdCtrlEntry = g_CmdCtrlHash.Search(cmd);
	if (!pCmdCtrlEntry)
	{
		pCmdCtrlEntry = new sCmdCtrlEntry{};
		pCmdCtrlEntry->cmd = cmd;
		g_CmdCtrlHash.Insert(pCmdCtrlEntry);
	}
	RemoveCmdCtrlNode(&pCmdCtrlEntry->ctrlList, m_misc_str);

	auto* pNode = new cAnsiStrNode{};
	pNode->item = m_misc_str;
	pCmdCtrlEntry->ctrlList.Append(pNode);

	cmd = new char[strlen(" control: ") + strlen(tokens[1]) + strlen(m_misc_str) + 1];
	strcpy(cmd, tokens[1]);
	strcat(cmd, " control: ");
	strcat(cmd, m_misc_str);
	m_control_binds.ChangeInfo(m_misc_str, cmd, strlen(cmd) + 1);
	return 0;
}

void CDECL cIBInputMapper::RemoveCmdCtrlNode(cAnsiStrList* pList, const char* pszCtrl)
{
	for (auto* pNode = pList->GetFirst(); pNode != nullptr; pNode = pNode->GetNext())
	{
		if (pNode->item == pszCtrl)
		{
			delete pList->Remove(pNode);
			break;
		}
	}
}

const char* cIBInputMapper::Unbind(const char* control, char** var_name)
{
	var_name[0][0] = '\0';

	char controls[4][32] = {};
	long num_controls = 0;
	auto* ret_val = DecomposeControl(control, controls, &num_controls);
	if (ret_val)
		return ret_val;

	RecomposeControl(m_misc_str, controls, num_controls);
	auto* cmd = m_control_binds.Find(m_misc_str);
	if (!cmd)
		return 0;

	char pszCmd[256] = {};
	strcpy(pszCmd, cmd);

	auto* pStr = strstr(pszCmd, " control: ");
	if (pStr == nullptr)
		return "Command control string malformed";
	*pStr = '\0';

	auto* pCmdCtrlEntry = g_CmdCtrlHash.Search(pszCmd);
	if (pCmdCtrlEntry != nullptr)
		RemoveCmdCtrlNode(&pCmdCtrlEntry->ctrlList, m_misc_str);

	int num_tokens = 0;
	auto** tokens = g_IB_variable_manager->Tokenize(cmd, &num_tokens, 1);
	auto active_toggle = tokens[0][0] == '+' || tokens[0][0] == '=';
	strcpy(*var_name, &(*tokens)[active_toggle]);

	m_control_binds.Delete(m_misc_str, 1, nullptr);
	g_input_down.Delete(m_misc_str, 1, nullptr);
	g_IB_variable_manager->FreeTokens(tokens, num_tokens);

	return 0;
}

const char* cIBInputMapper::PeekBind(const char* control, int strip_control)
{
	char controls[4][32] = {};
	long num_controls = 0;
	auto* ret_val = DecomposeControl(control, controls, &num_controls);
	if (ret_val)
		return ret_val;

	RecomposeControl(m_misc_str, controls, num_controls);

	const auto* cmd = m_control_binds.Find(m_misc_str);
	if (!cmd)
		return 0;
	if (strip_control)
		StripControl(m_misc_str, cmd);

	return m_misc_str;
}

void cIBInputMapper::TrapBind(const char* p_cmd, tTrapBindFilter filter_cb, tTrapBindPostFunc post_cb, void* data)
{
	if (m_trapping)
		return;

	m_trapping = true;

	Rect rect = {};
	rect.lr.x = grd_canvas->bm.w;
	rect.lr.y = grd_canvas->bm.h;

	mouse_get_xy(&g_prev_mousex, &g_prev_mousey);
	uiHideMouse(&rect);

	uiSlab* slab = nullptr;
	uiGetCurrentSlab(&slab);
	uiInstallRegionHandler(slab->creg, SCANCODE_LCTRL, cIBInputMapper::StaticTrapHandler, nullptr, &g_id);

	m_bind_cmd = p_cmd;
	m_filter_cb = filter_cb;
	m_post_cb = post_cb;
	m_filter_data = data;
	m_mod_states = 0;
}

BOOL CDECL cIBInputMapper::StaticTrapHandler(uiEvent* p_event, Region*, void*)
{
	return g_IB_input_mapper->TrapHandler(p_event);
}

BOOL cIBInputMapper::TrapHandler(uiEvent* p_event)
{
	char bind_control[128] = {};

	switch (p_event->type)
	{
	case UI_EVENT_KBD_RAW: {
		auto* key_event = reinterpret_cast<uiRawKeyEvent*>(p_event);
		uint mask = 0;
		auto shifted = !!(m_mod_states & MASK_CAPS_LOCK) ^ !!(m_mod_states & MASK_LSHIFT) ^ !!(m_mod_states & MASK_RSHIFT);
		int strip_shift = 0;
		switch (key_event->scancode)
		{
		case SCANCODE_LCTRL:
		case SCANCODE_RCTRL:
			mask = MASK_CTRL;
			break;
		case SCANCODE_LSHIFT:
		case SCANCODE_RSHIFT:
			mask = MASK_SHIFT;
			break;
		case SCANCODE_LALT:
		case SCANCODE_RALT:
			mask = MASK_ALT;
			break;
		default:
			break;
		}

		const auto* control = GetRawControl(key_event->scancode, shifted, &strip_shift);
		if (control || mask)
		{
			if (!key_event->action)
			{
				if (mask)
				{
					if (m_mod_states & MASK_LALT)
					{
						strcpy(bind_control, "alt");
					}
					else if (m_mod_states & MASK_LCTRL)
					{
						strcpy(bind_control, "ctrl");
					}
					else if (m_mod_states & MASK_LSHIFT)
					{
						strcpy(bind_control, "shift");
					}
				}
				else
				{
					strcpy(bind_control, control);
					if ((m_mod_states & MASK_LALT) != 0)
						strcat(bind_control, "+alt");
					if ((m_mod_states & MASK_LCTRL) != 0)
						strcat(bind_control, "+ctrl");
					if ((m_mod_states & MASK_LSHIFT) != 0 && !strip_shift)
						strcat(bind_control, "+shift");
				}
				break;
			}
			m_mod_states |= mask;
		}

		return true;
	}
	case UI_EVENT_MOUSE: {
		auto* mouse_event = reinterpret_cast<uiMouseEvent*>(p_event);
		uint8 action = p_event->subtype;
		if (action)
		{
			if (mouse_event->wheel && action == 128)
			{
				strcpy(bind_control, "mouse_wheel");
				break;
			}
			char butt_str[32] = {};
			sprintf(butt_str, "%d", action);
			auto* control = g_input_controls[1].Find(butt_str);
			if (control)
			{
				strcpy(bind_control, control);
				break;
			}
		}
		return true;
	}
	case UI_EVENT_MOUSE_MOVE: {
		return true;
	}
	case UI_EVENT_JOY: {
		auto* event = reinterpret_cast<uiJoyEvent*>(p_event);
		if (p_event->subtype == 3)
		{
			char str[32] = {};
			sprintf(str, "joy%d", event->joynum + 1);
			auto* control = g_input_controls[2].Find(str);
			if (control)
			{
				strcpy(bind_control, control);
				break;
			}
		}
		else
		{
			if (event->joynum == 2)
			{
				if (event->joypos.y == 1)
				{
					strcpy(bind_control, "joy_hatup");
				}
				else if (event->joypos.y == -1)
				{
					strcpy(bind_control, "joy_hatdn");
				}
				else if (event->joypos.x == -1)
				{
					strcpy(bind_control, "joy_hatlt");
				}
				else if (event->joypos.x == 1)
				{
					strcpy(bind_control, "joy_hatrt");
				}
				break;
			}
		}
		return true;
	}
	}


	auto bound = false;
	if (m_filter_cb == nullptr || m_filter_cb(bind_control, m_bind_cmd, m_filter_data))
	{
		bound = true;
		char cmd[128] = {};
		strcpy(cmd, "bind ");
		strcat(cmd, bind_control);
		strcat(cmd, " ");

		int num_tokens = 0;
		auto** tokens = g_IB_variable_manager->Tokenize(m_bind_cmd, &num_tokens, false);
		g_IB_variable_manager->FreeTokens(tokens, num_tokens);
		if (num_tokens <= 1)
		{
			strcat(cmd, m_bind_cmd);
		}
		else
		{
			strcat(cmd, "\"");
			strcat(cmd, m_bind_cmd);
			strcat(cmd, "\"");
		}

		g_IB_variable_manager->Cmd(cmd, false);
	}

	m_trapping = false;

	Rect rect = {};
	rect.ul.x = 0;
	rect.ul.y = 0;
	rect.lr.x = grd_canvas->bm.w;
	rect.lr.y = grd_canvas->bm.h;

	uiShowMouse(&rect);
	mouse_put_xy(g_prev_mousex, g_prev_mousey);

	uiSlab* slab = nullptr;
	uiGetCurrentSlab(&slab);
	uiRemoveRegionHandler(slab->creg, g_id);
	m_mod_states = 0;
	if (m_post_cb)
		m_post_cb(bound);

	return true;
}

char* cIBInputMapper::GetControlFromCmdStart(char* pszCmd, char* pszCtrlBuf)
{
	pszCtrlBuf[0] = '\0';

	auto* pCmdCtrlEntry = g_CmdCtrlHash.Search(pszCmd);
	if (!pCmdCtrlEntry)
		return pszCtrlBuf;

	m_pCtrlIterNode = pCmdCtrlEntry->ctrlList.GetFirst();
	if (m_pCtrlIterNode)
	{
		strcpy(pszCtrlBuf, m_pCtrlIterNode->item);
		m_pCtrlIterNode = m_pCtrlIterNode->GetNext();
	}

	return pszCtrlBuf;
}

char* cIBInputMapper::GetControlFromCmdNext(char* pszCtrlBuf)
{
	pszCtrlBuf[0] = '\0';

	if (!m_pCtrlIterNode)
		return pszCtrlBuf;

	strcpy(pszCtrlBuf, m_pCtrlIterNode->item);
	m_pCtrlIterNode = m_pCtrlIterNode->GetNext();

	return pszCtrlBuf;
}

const char* cIBInputMapper::SaveBnd(const char* filename, char* header)
{
	auto* fp = fopen(filename, "wt");
	if (!fp)
		return "Error opening file";

	if (header)
		fprintf(fp, "%s\n\n", header);

	m_control_binds.ResetVisited(nullptr);
	while (true)
	{
		char control_name[64] = {};
		const auto* cmd = m_control_binds.GetNextInOrder(control_name);
		if (!cmd)
			break;

		if (*cmd == '\0')
			continue;

		StripControl(m_misc_str, cmd);
		int num_tokens = 0;
		auto** tokens = g_IB_variable_manager->Tokenize(m_misc_str, &num_tokens, 0);
		g_IB_variable_manager->FreeTokens(tokens, num_tokens);
		if (num_tokens <= 1)
			fprintf(fp, "bind %s %s\r\n", control_name, m_misc_str);
		else
			fprintf(fp, "bind %s \"%s\"\r\n", control_name, m_misc_str);
	}

	fclose(fp);

	return nullptr;
}

void cIBInputMapper::PollAllKeys()
{
	ushort cooked = {};
	bool results[4] = {};

	FlushKeys();
	kbs_event keycode = {};
	for (int i = 0; i < 224; ++i)
	{
		keycode.code = i;
		kb_cook_real(keycode, &cooked, results, 0); // fixme: assert on failure
		ProcessRawKey(i, kb_state(cooked), 1);
	}
	kbd_modifier_state = 0;
}

void cIBInputMapper::FlushKeys()
{
	kb_flush();

	g_input_down.ResetVisited(nullptr);
	while (true)
	{
		auto* down = g_input_down.GetNextInOrder(nullptr);
		if (down == nullptr)
			break;

		*down = false;
	}
}

cControlDownNode* CDECL cIBInputMapper::GetControlDownNode(const char* pControl)
{
	for (auto* pNode = g_ctrldown_list.GetFirst(); pNode != nullptr; pNode = pNode->GetNext())
	{
		if (pNode->item.control == pControl)
			return pNode;
	}
}

int cIBInputMapper::SendButtonCmd(const char* control, int down, int mod)
{
	auto ret_val = 1;
	auto* already_down = g_input_down.Find(control);
	if (down)
	{
		const auto* cmd = m_control_binds.Find(control);
		if (cmd)
		{
			if (!GetControlDownNode(control))
			{
				auto node = new cControlDownNode;
				node->item.control = control;
				node->item.pCmd = cmd;
				g_ctrldown_list.Prepend(node);
			}

			g_IB_variable_manager->Cmd(cmd, *already_down);
		}
		else
		{
			ret_val = 0;
		}

		if (already_down)
			*already_down = true;
	}
	else
	{
		if (already_down)
			*already_down = false;
		auto* node = GetControlDownNode(control);
		char* pCmd = nullptr;
		if (node)
		{
			pCmd = new char[strlen(node->item.pCmd)]; // fixme: may be an error; no alloc in original version
			strcpy(pCmd, node->item.pCmd);
			delete g_ctrldown_list.Remove(node);
		}

		if (pCmd)
		{
			if (pCmd[0] == '+')
			{
				pCmd[0] = '-';
				g_IB_variable_manager->Cmd(pCmd, *already_down);
				pCmd[0] = '+';
			}
			else if (pCmd[0] == '-')
			{
				pCmd[0] = '+';
				g_IB_variable_manager->Cmd(pCmd, *already_down);
				pCmd[0] = '-';
			}

			delete[] pCmd;
		}
		else
		{
			ret_val = 0;
		}
	}

	if (mod)
	{
		uint mask = 0;
		if (!strcmp("shift", control))
		{
			mask = MASK_SHIFT;
		}
		else if (!strcmp("lshift", control))
		{
			mask = MASK_LSHIFT;
		}
		else if (!strcmp("rshift", control))
		{
			mask = MASK_RSHIFT;
		}
		else if (!strcmp("alt", control))
		{
			mask = MASK_ALT;
		}
		else if (!strcmp("lalt", control))
		{
			mask = MASK_LALT;
		}
		else if (!strcmp("ralt", control))
		{
			mask = MASK_RALT;
		}
		else if (!strcmp("ctrl", control))
		{
			mask = MASK_CTRL;
		}
		else if (!strcmp("lctrl", control))
		{
			mask = MASK_LCTRL;
		}
		else if (!strcmp("caps_lock", control))
		{
			m_mod_states ^= 4u;
			// fixme: control instead of shift??
			SendButtonCmd("shift", m_mod_states & 4, 0);
		}

		if (down)
			m_mod_states |= mask;
		else
			m_mod_states &= ~mask;
	}

	return ret_val;
}

int cIBInputMapper::ProcessRawKey(short scancode, BOOL action, int polling)
{
	switch (scancode)
	{
	case SCANCODE_LCTRL:
		if (m_mod_states & MASK_LCTRL && action)
			return 0;
		return ProcessRawMod("lctrl", action, FALSE);
		break;
	case SCANCODE_RCTRL:
		if (m_mod_states & MASK_RCTRL && action)
			return 0;
		return ProcessRawMod("rctrl", action, FALSE);
		break;
	case SCANCODE_LSHIFT:
		if (m_mod_states & MASK_CAPS_LOCK)
			return 0;
		if (m_mod_states & MASK_LSHIFT && action)
			return 0;
		return ProcessRawMod("lshift", action, FALSE);
		break;
	case SCANCODE_RSHIFT:
		if (m_mod_states & MASK_CAPS_LOCK)
			return 0;
		if (m_mod_states & MASK_RSHIFT && action)
			return 0;
		return ProcessRawMod("rshift", action, FALSE);
		break;
	case SCANCODE_LALT:
		if (m_mod_states & MASK_LALT && action)
			return 0;
		return ProcessRawMod("lalt", action, FALSE);
		break;
	case SCANCODE_RALT:
		if (m_mod_states & MASK_RALT && action)
			return 0;
		return ProcessRawMod("ralt", action, FALSE);
		break;
	case SCANCODE_CAPS_LOCK:
		if (m_mod_states & MASK_SHIFT)
			return 0;
		if (action || polling)
			return 0;
		return ProcessRawMod("caps_lock", 0, TRUE);
		break;
	default:
		auto shifted = !!(m_mod_states & MASK_CAPS_LOCK) != !!(m_mod_states & MASK_SHIFT);
		int strip_shift = 0;
		auto control = GetRawControl(scancode, shifted, &strip_shift);
		if (control == nullptr)
			return 0;

		auto down = g_input_down.Find(control);
		if (down)
		{
			if (*down != FALSE)
				*down = FALSE;
			else
				*down = action;
		}
		return AttachMods(control, action, strip_shift);
		break;
	}
}

char* cIBInputMapper::GetRawControl(short scancode, int shifted, int* strip_shift)
{
	ushort cnv = 0;

	auto unshifted_key = kb_cnv_table[scancode][0];
	if (shifted)
		cnv = kb_cnv_table[scancode][1];
	else
		cnv = unshifted_key;

	*strip_shift = 0;

	if (shifted)
	{
		if ((cnv & 0x800) != 0 || (uchar)cnv < 0x41u || (uchar)cnv > 0x5Au)
		{
			if (cnv != unshifted_key && (cnv & 0x4000) == 0 && (uchar)cnv != 9)
				*strip_shift = 1;
		}
		else
		{
			cnv += 'a' - 'A';
		}
	}

	if ((cnv & 0x4000) != 0)
	{
		cnv = scancode | 0x800;
	}
	else
	{
		cnv &= ~0xD700;
	}

	char key_str[32] = {};
	sprintf(key_str, "%d", cnv);
	return g_input_controls[0].Find(key_str);
}

bool valid_mod_fields[3][3] = {};

int cIBInputMapper::AttachMods(char* control, BOOL action, int strip_shift)
{
	valid_mod_fields[0][0] = !strip_shift && !!(m_mod_states & MASK_CAPS_LOCK) != !!(m_mod_states & MASK_SHIFT);
	valid_mod_fields[0][1] = !strip_shift && !!(m_mod_states & MASK_CAPS_LOCK) != !!(m_mod_states & MASK_LSHIFT);
	valid_mod_fields[0][2] = !strip_shift && !!(m_mod_states & MASK_CAPS_LOCK) != !!(m_mod_states & MASK_RSHIFT);
	valid_mod_fields[1][0] = m_mod_states & MASK_CTRL;
	valid_mod_fields[1][1] = m_mod_states & MASK_LCTRL;
	valid_mod_fields[1][2] = m_mod_states & MASK_RCTRL;
	valid_mod_fields[2][0] = m_mod_states & MASK_ALT;
	valid_mod_fields[2][1] = m_mod_states & MASK_LALT;
	valid_mod_fields[2][2] = m_mod_states & MASK_RALT;

	uint8 mod_mask = 0;
	for (int i = 0; i < 3; ++i)
		mod_mask |= valid_mod_fields[i][0] << i;

	int mod_loop_num[3] = {};
	for (int i = 0; i < 3; ++i)
		mod_loop_num[i] = valid_mod_fields[i][0] != 0 ? 3 : 1;

	int i;
	for (i = 0; i < 3; ++i)
	{
		if (i == 0)
		{
			if (!!(m_mod_states & MASK_CAPS_LOCK) == !!(m_mod_states & MASK_SHIFT))
				continue;
		}
		if (i == 0 || valid_mod_fields[i][0])
			break;
	}

	while (i < 3)
	{
		for (int s = 0; s < mod_loop_num[0]; ++s)
		{
			for (int c = 0; c < mod_loop_num[1]; ++c)
			{
				for (int a = 0; a < mod_loop_num[2]; ++a)
				{
					uint8 cur_mask = valid_mod_fields[0][s];
					cur_mask |= valid_mod_fields[1][c] << 1;
					cur_mask |= valid_mod_fields[2][a] << 2;
					if (cur_mask == mod_mask)
					{
						char final_control[64] = {};
						strcpy(final_control, control);
						if (valid_mod_fields[2][a])
							strcat(final_control, g_mods[2][a]);
						if (valid_mod_fields[1][c])
							strcat(final_control, g_mods[1][c]);
						if (valid_mod_fields[0][s])
							strcat(final_control, g_mods[0][s]);

						auto* down = g_input_down.Find(final_control);
						if (!action || down)
						{
							auto ret_val = SendButtonCmd(final_control, action, 0);
							if (action)
								return ret_val;
						}
					}
				}
			}
		}

		for (int j = 0; j < 3; ++j)
			valid_mod_fields[i][j] = false;

		if (strip_shift)
		{
			if (!i)
			{
				auto* key = g_input_codes.Find(control);
				int dummy = 0;
				auto* ret = GetRawControl(g_shift_to_scan[*key], 0, &dummy);
				if (ret)
					control = ret;
			}
		}

		++i;
		while (i < 3 && !valid_mod_fields[i][0])
			++i;
		++i;
	}

	return SendButtonCmd(control, action, 0);
}

int cIBInputMapper::ProcessRawMod(const char* cmod, BOOL action, BOOL general)
{
	assert(strlen(cmod) < 16);
	char mod[16]{};
	strcpy(mod, cmod);

	if (!general)
	{
		mod[0] = mod[0] != 'r' ? 'r' : 'l';
		const auto* down = g_input_down.Find(mod);
		if (down == nullptr || down[0] == '\0')
			ProcessRawMod(&mod[1], action, TRUE);

		mod[0] = mod[0] != 'r' ? 'r' : 'l'; // FIXME: remove this line?
	}

	SendButtonCmd(mod, action, 1);
	if (!strcmp(mod, "caps_lock"))
		return 0;

	m_control_binds.ResetVisited(nullptr);
	if (action)
		return 0;

	char control[64] = {};
	while (m_control_binds.GetNextInOrder(control))
	{
		const auto* down = g_input_down.Find(control);
		if (down == nullptr || *down == '\0')
			continue;

		char controls[4][32] = {};
		long num_controls = 0;
		DecomposeControl(control, controls, &num_controls);
		for (int i = 0; i < num_controls; ++i)
		{
			if (!strcmp(controls[i], mod))
			{
				SendButtonCmd(control, 0, 0);
				break;
			}
		}
	}

	return 0;
}

uint8 cIBInputMapper::GetControlMask(char(*controls)[32], int num_controls)
{
	uint8 mask = 0;

	for (int i = 1; i < num_controls; ++i)
	{
		if (!strcmp(&(*controls)[32 * i], "shift") || !strcmp(&(*controls)[32 * i], "lshift") || !strcmp(&(*controls)[32 * i], "rshift"))
		{
			mask |= 1u;
		}
		else if (!strcmp(&(*controls)[32 * i], "ctrl") || !strcmp(&(*controls)[32 * i], "lctrl") || !strcmp(&(*controls)[32 * i], "rctrl"))
		{
			mask |= 2u;
		}
		else if (!strcmp(&(*controls)[32 * i], "alt") || !strcmp(&(*controls)[32 * i], "lalt") || !strcmp(&(*controls)[32 * i], "ralt"))
		{
			mask |= 4u;
		}
	}

	return mask;
}

uint8 cIBInputMapper::GetStateMask()
{
	uint8 mask = 0;
	if ((m_mod_states & 0x8) || (m_mod_states & 0x10))
		mask = 1;

	if ((m_mod_states & 0x80) || (m_mod_states & 0x100))
		mask |= 2u;

	if ((m_mod_states & 0x20) || (m_mod_states & 0x40))
		mask |= 4u;

	return mask;
}

int cIBInputMapper::ProcessCookedKey(short code)
{
	short key = code & ~0x1700u;
	auto letter_upp = false;
	if (key >= 'A' && key <= 'Z')
	{
		key += 'a' - 'A';
		letter_upp = true;
	}

	char key_str[33] = {};
	sprintf(key_str, "%d", key);
	const auto* control = g_input_controls[0].Find(key_str);
	if (!control)
		return 0;

	strcpy(key_str, control);
	if ((code & 0x400) == 0x1000)
		strcat(key_str, "+alt");

	if ((code & 0x200) == 0x1000)
		strcat(key_str, "+ctrl");

	if ((code & 0x1000) == 0x1000 || letter_upp)
		strcat(key_str, "+shift");

	return SendButtonCmd(key_str, (code & 0x100) != 0, 0);
}

int cIBInputMapper::ProcessMouseButton(uiMouseEvent* event)
{
	auto action = event->action;
	if (!action)
		return 0;

	char butt_str[32]{};
	bool isMouseWheel = false;
	bool down = true;

	if (event->wheel && (action & 0x80u))
	{
		action &= ~0x80u;
		isMouseWheel = true;
	}

	for (int i = 0; i < 16; ++i)
	{
		auto action_bit = 1 << i;
		if (action & (1 << i))
		{
			sprintf(butt_str, "%d", action_bit);
			auto* control = g_input_controls[1].Find(butt_str);
			if (control)
			{
				down = true;
				if (SendButtonCmd(control, 1, 0))
					return 1;
			}
			else
			{
				down = false;
				sprintf(butt_str, "%d", action_bit >> 1);
				control = g_input_controls[1].Find(butt_str);
				if (control && SendButtonCmd(control, down, 0))
					return 1;
			}
		}
	}

	auto* control = m_control_binds.Find("mouse_wheel");
	if (isMouseWheel && control != nullptr)
	{
		char final[64]{};
		StripControl(final, control);
		sprintf(butt_str, " %d", event->wheel);
		strcat(final, butt_str);
		g_IB_variable_manager->Cmd(final, false);

		return 1;
	}

	if (control)
		return SendButtonCmd(control, down, 0);

	return 0;
}

int cIBInputMapper::ProcessMouseMove(uiMouseEvent* event)
{
	auto res_x = grd_canvas->bm.w;
	auto res_y = grd_canvas->bm.h;

	auto old_mouse_mask = mouseMask;
	mouseMask &= 254;
	mouse_put_xy(res_x >> 1, res_y >> 1);
	mouseMask = old_mouse_mask;

	auto x = std::clamp(event->pos.x, static_cast<short>(0), res_x);
	auto y = std::clamp(event->pos.y, static_cast<short>(0), res_y);

	char str[64] = {};
	char final[64] = {};

	const auto* cmd = m_control_binds.Find("mouse_axisx");
	if (cmd)
	{
		StripControl(final, cmd);
		float rel_x = x - (res_x >> 1);
		sprintf(str, " %8.8f", rel_x);
		strcat(final, str);
		g_IB_variable_manager->Cmd(final, 0);
	}

	cmd = m_control_binds.Find("mouse_axisy");
	if (cmd)
	{
		StripControl(final, cmd);
		float rel_y = y - (res_y >> 1);
		sprintf(str, " %8.8f", -rel_y);
		strcat(final, str);
		g_IB_variable_manager->Cmd(final, 0);
	}

	return 0;
}

void cIBInputMapper::StripSendDoubleCmd(const char* control, long double a)
{
	char str[64] = {};
	char final[64] = {};

	const auto* cmd = m_control_binds.Find(control);
	if (cmd == nullptr)
		return;

	sprintf(str, " %8.8f", (double)a);
	StripControl(final, cmd);
	strcat(final, str);
	g_IB_variable_manager->Cmd(final, 0);
}

int cIBInputMapper::ProcessJoystick(uiJoyEvent* event)
{
	if (event->action == 1 || event->action == 3)
	{
		char str[32] = {};
		sprintf(str, "joy%d", event->joynum + 1);
		auto* control = g_input_controls[2].Find(str);
		if (control != nullptr)
			return SendButtonCmd(control, event->action == 3, 0);

		return 0;
	}

	if (event->action)
		return 1;

	if (event->joynum == 0)
	{
		auto x = ShortScaleDouble(event->joypos.x);
		auto y = ShortScaleDouble(event->joypos.y);
		if (m_joyproc)
			m_joyproc->ProcessXY(&x, &y);

		StripSendDoubleCmd("joy_axisx", x);
		StripSendDoubleCmd("joy_axisy", y);

		return 1;
	}

	if (event->joynum == 1)
	{
		auto r = ShortScaleDouble(event->joypos.x);
		auto z = ShortScaleDouble(event->joypos.y);
		if (m_joyproc)
			m_joyproc->ProcessZR(&z, &r);

		StripSendDoubleCmd("joy_axisz", z);
		StripSendDoubleCmd("joy_axisr", r);

		return 1;
	}

	if (event->joynum == 2)
	{
		if (event->joypos.y == 1)
		{
			SendButtonCmd("joy_hatup", 1, 0);
		}
		else if (event->joypos.y == -1)
		{
			SendButtonCmd("joy_hatdn", 1, 0);
		}
		else
		{
			SendButtonCmd("joy_hatup", 0, 0);
			SendButtonCmd("joy_hatdn", 0, 0);
		}
		if (event->joypos.x == -1)
		{
			SendButtonCmd("joy_hatlt", 1, 0);
		}
		else if (event->joypos.x == 1)
		{
			SendButtonCmd("joy_hatrt", 1, 0);
		}
		else
		{
			SendButtonCmd("joy_hatlt", 0, 0);
			SendButtonCmd("joy_hatrt", 0, 0);
		}
	}

	return 1;
}

double CDECL cIBInputMapper::ShortScaleDouble(short m)
{
	return (double)(m - 0x7FFF) * m + 1.0;
}

const char* cIBInputMapper::DecomposeControl(const char* control_str, char(*controls)[32], long* num_controls)
{
	int num = 0;
	char tmp_controls[32][4] = {};

	for (int i = 0; ; ++i)
	{
		if (*control_str == '\0')
			break;

		if (*control_str == '+' && i != 0)
		{
			tmp_controls[num][i] = '\0';
			++num;
			i = 0;
			++control_str;
		}

		if (num == 4)
			break;

		tmp_controls[num][i] = *control_str;
		++control_str;
	}

	*num_controls = num;
	for (int i = 0; i < num; ++i)
	{
		if (strcmp(tmp_controls[i], "alt") && strcmp(tmp_controls[i], "ctrl") && strcmp(tmp_controls[i], "shift"))
		{
			if (i != 0)
			{
				std::swap(tmp_controls[0], tmp_controls[i]);
			}
			break;
		}

		if (num > 1 && i == num - 1)
			return "A non-modifier control is needed in compound bindings";
	}

	for (int i = num; i > 2; --i)
	{
		for (int j = 1; j < i - 1; ++j)
		{
			if (strcmp(tmp_controls[j], "alt") && strcmp(tmp_controls[j], "ctrl") && strcmp(tmp_controls[j], "shift")
				|| strcmp(tmp_controls[j + 1], "alt")
				&& strcmp(tmp_controls[j + 1], "ctrl")
				&& strcmp(tmp_controls[j + 1], "shift"))
			{
				return "Multiple non-modifier controls are not allowed";
			}

			auto cmp = strcmp(tmp_controls[j], tmp_controls[j + 1]);
			if (cmp == 0)
			{
				return "Duplicate modifier";
			}

			if (cmp > 0)
			{
				std::swap(tmp_controls[j], tmp_controls[j + 1]);
			}
		}
	}

	if (num == 2
		&& strcmp(tmp_controls[1], "alt")
		&& strcmp(tmp_controls[1], "ctrl")
		&& strcmp(tmp_controls[1], "shift"))
	{
		return "Multiple non-modifier controls are not allowed";
	}

	for (int i = 0; i < num; ++i)
		strcpy(controls[i], tmp_controls[i]);

	for (int i = num - 1; i >= 0; --i)
	{
		if (!g_input_codes.Find(controls[i]))
			return "Invalid input control";
	}

	return nullptr;
}

void cIBInputMapper::RecomposeControl(char* control_str, char(*controls)[32], int num_controls)
{
	control_str[0] = '\0';
	for (int i = 0; i < num_controls; ++i)
	{
		if (i != 0)
			strcat(control_str, "+");
		strcat(control_str, &(*controls)[32 * i]);
	}
}

void cIBInputMapper::StripControl(char* dest, const char* src)
{
	dest[0] = '\0';
	if (src == nullptr)
		return;

	int num_tokens = 0;
	auto** tokens = g_IB_variable_manager->Tokenize(src, &num_tokens, 0);
	if (num_tokens >= 3 && !strcmp("control:", tokens[num_tokens - 2]))
	{
		g_IB_variable_manager->TokensToStr(dest, 128, const_cast<const char**>(tokens), num_tokens - 2);
		g_IB_variable_manager->FreeTokens(tokens, num_tokens);
		return;
	}

	strcpy(dest, src);
	g_IB_variable_manager->FreeTokens(tokens, num_tokens);
}

BOOL CDECL cIBInputMapper::InputBindingHandler(uiEvent* p_event)
{
	auto valid_events = g_IB_input_mapper->m_valid_events;
	switch (p_event->type)
	{
	case UI_EVENT_KBD_RAW:
		if (valid_events & UI_EVENT_KBD_RAW)
			return g_IB_input_mapper->ProcessRawKey(p_event->subtype, p_event->data[0], 0);
		break;
	case UI_EVENT_KBD_COOKED:
		if (valid_events & UI_EVENT_KBD_COOKED)
			return g_IB_input_mapper->ProcessCookedKey(p_event->subtype);
		break;
	case UI_EVENT_MOUSE:
		if (valid_events & UI_EVENT_MOUSE)
			return g_IB_input_mapper->ProcessMouseButton(reinterpret_cast<uiMouseEvent*>(p_event));
		break;
	case UI_EVENT_MOUSE_MOVE:
		if (valid_events & UI_EVENT_MOUSE_MOVE)
			return g_IB_input_mapper->ProcessMouseMove(reinterpret_cast<uiMouseEvent*>(p_event));
		break;
	case UI_EVENT_JOY:
		if (valid_events & UI_EVENT_JOY)
			return g_IB_input_mapper->ProcessJoystick(reinterpret_cast<uiJoyEvent*>(p_event));
		break;
	default:
		return FALSE;
	}

	return FALSE;
}