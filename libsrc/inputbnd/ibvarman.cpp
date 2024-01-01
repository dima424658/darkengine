#include <ibvarman.h>
#include <ibmapper.h>

#include <memory>

extern cIBInputMapper* g_IB_input_mapper;
extern cIBVariableManager* gIB_variable_manager;

static const char* SkipWhiteSpace(const char* str);
static void GetChunk(char* dest, const char** src, char ch1, char ch2);

cIBVariableManager::cIBVariableManager()
{
	m_vars = {};
	Init();
	SetMasterAggregation(IBMaxActiveAgg);
}

//----- (008D1AA2) --------------------------------------------------------
cIBVariableManager::cIBVariableManager(IB_var* var)
{
	m_vars = {};
	Init();
	VarSet(var, 0);
}

//----- (008D1AD0) --------------------------------------------------------
cIBVariableManager::cIBVariableManager(IB_var* var, int num)
{
	m_vars = {};
	Init();
	VarSet(var, num, 1);
}

//----- (008D1B02) --------------------------------------------------------
void cIBVariableManager::Init()
{
	m_glob_cb = 0;
	m_bnd_path = 0;
	SetBndSearchPath(".");
	gIB_variable_manager = this;
	SetMasterAggregation(IBMaxActiveAgg);
}

//----- (008D1B47) --------------------------------------------------------
cIBVariableManager::~cIBVariableManager()
{
	delete m_bnd_path;
	m_bnd_path = nullptr;
}

//----- (008D1B80) --------------------------------------------------------
int cIBVariableManager::VarSet(IB_var* vars, int alias)
{
	intrnl_var* v4; // eax
	unsigned int flags; // eax
	intrnl_var* v6; // [esp+0h] [ebp-54h]
	intrnl_var* v8; // [esp+14h] [ebp-40h]
	IB_var* cur_var; // [esp+24h] [ebp-30h]
	char name[32]; // [esp+2Ch] [ebp-28h] BYREF

	if (!vars)
		return 0;

	int ret_val = 1;

	char* cmd_line_chan[] = { new char[40], new char[40] };
	strcpy(cmd_line_chan[0], "cmd_line");

	for (auto* cur_var = vars; vars->name[0]; ++cur_var)
	{
		if (strlen(cur_var->name) >= 32 || strlen(cur_var->val) >= 32)
		{
			ret_val = 0;
			continue;
		}

		strcpy(name, cur_var->name);
		strlwr(name);

		auto* int_var = m_vars.Find(name);
		if (int_var)
		{
			strcpy(int_var->var.val, cur_var->val);
			strlwr(int_var->var.val);
			int_var->var.cb = cur_var->cb;
			int_var->var.agg = cur_var->agg;
			continue;
		}

		auto* int_var = new intrnl_var();
		memcpy(int_var, cur_var, sizeof(intrnl_var));
		if (alias)
			int_var->var.flags = int_var->var.flags | 2;
		if (!cur_var->val[0])
			sprintf(int_var->var.val, "%8.8f", 0.0);

		strcpy(int_var->var.last_val, int_var->var.val);
		strlwr(int_var->var.name);
		strlwr(int_var->var.val);
		m_vars.Add(int_var->var.name, int_var, 1);
		strcpy(cmd_line_chan[1], name);
		AddChannel(cmd_line_chan);
	}

	delete[] cmd_line_chan[0];
	delete[] cmd_line_chan[1];

	return ret_val;
}

//----- (008D1E04) --------------------------------------------------------
int cIBVariableManager::VarSet(IB_var* vars, int num, int alias)
{
	auto* new_vars = new IB_var[num + 1];
	memcpy(new_vars, vars, 140 * num);
	memset(&new_vars[num], 0, sizeof(IB_var));
	auto ret_val = VarSet(new_vars, alias);
	delete[] new_vars;

	return ret_val;
}

//----- (008D1E9B) --------------------------------------------------------
const char* cIBVariableManager::VarUnset(const char* const* var_names, int user)
{
	const char* ret_val = nullptr;

	for (int i = 0; var_names[i] != nullptr; ++i)
	{
		char name[32];
		strncpy(name, var_names[i], 32);
		strlwr(name);

		auto* int_var = m_vars.Find(name);
		if (!int_var)
		{
			ret_val = "Variable not found";
		}
		else
		{
			if (!user || (int_var->var.flags & 1))
			{
				int_var->channels.DeleteAll(1);
				m_vars.Delete(name, 1, 0);
			}
			else
			{
				ret_val = "Variable is reserved";
			}
		}
	}

	return ret_val;
}

//----- (008D1F4F) --------------------------------------------------------
const char* cIBVariableManager::VarUnset(const char* const* var_names, int num, int user)
{
	auto* new_names = new char* [num + 1];
	for (int i = 0; i < num; ++i)
	{
		new_names[i] = new char[strlen(var_names[i]) + 1];
		strcpy(new_names[i], var_names[i]);
	}

	new_names[num] = nullptr;

	auto* ret_val = VarUnset(new_names, user);
	for (int i = 0; i < num; ++i)
		delete[] new_names[i];
	delete[] new_names;

	return ret_val;
}

//----- (008D2068) --------------------------------------------------------
const char* cIBVariableManager::VarUnsetAll()
{
	while (true)
	{
		m_vars.ResetVisited(nullptr);
		auto* int_var = m_vars.GetNextInOrder(nullptr);
		if (int_var == nullptr)
			break;

		int_var->channels.DeleteAll(1);
		m_vars.Delete(int_var->var.name, 1, 0);
	}
	return nullptr;
}

//----- (008D20C1) --------------------------------------------------------
const char* cIBVariableManager::Cmd(const char* pCmd, int already_down)
{
	const char* ret_str = nullptr;

	auto* cmd = new char[strlen(pCmd) + 1];
	strcpy(cmd, pCmd);
	strlwr(cmd);

	int num_tokens = 0;
	auto** tokens = Tokenize(cmd, &num_tokens, 1);
	if (num_tokens == 0 || **tokens == ';')
	{
		ret_str = nullptr;
		goto cleanup;
	}

	if (strcmp("bind", tokens[0]) == 0)
	{
		if (num_tokens < 2)
		{
			ret_str = "Syntax error";
			goto cleanup;
		}

		if (strlen(tokens[1]) >= 32 || num_tokens == 3 && strlen(tokens[2]) >= 64)
		{
			ret_str = "Field too long";
			goto cleanup;
		}

		if (num_tokens == 2)
		{
			ret_str = g_IB_input_mapper->PeekBind(tokens[1], 1);
			goto cleanup;
		}

		int loc_num_tokens = 0;
		auto** loc_tokens = Tokenize(cmd, &loc_num_tokens, 0);
		if (loc_num_tokens != 2 && loc_num_tokens != 3)
		{
			ret_str = "Syntax error";
			FreeTokens(loc_tokens, loc_num_tokens);
			goto cleanup;
		}

		if (strcmp(tokens[1], "shift") == 0 || strcmp(tokens[1], "alt") == 0 || strcmp(tokens[1], "ctrl") == 0)
		{
			auto dest_len = strlen("bind ") + strlen(loc_tokens[1]) + strlen(loc_tokens[2]) + 3;
			auto dest = new char[dest_len];
			strcpy(dest, "bind ");
			strcat(dest, "l");
			strcat(dest, loc_tokens[1]);
			if (loc_num_tokens == 3)
			{
				strcat(dest, " ");
				strcat(dest, loc_tokens[2]);
			}

			Cmd(dest, 0);
			dest[5] = 'r';
			ret_str = Cmd(dest, 0);
			delete[] dest;
		}

		auto tmp_cmd_len = strlen("unbind ") + strlen(loc_tokens[1]) + 1;
		auto* tmp_cmd = new char[tmp_cmd_len];
		strcpy(tmp_cmd, "unbind ");
		strcat(tmp_cmd, loc_tokens[1]);
		Cmd(tmp_cmd, 0);
		delete[] tmp_cmd;

		ret_str = g_IB_input_mapper->Bind(loc_tokens + 1);
		if (!ret_str)
			AddChannel(tokens + 1);

		FreeTokens(loc_tokens, loc_num_tokens);

		goto cleanup;
	}

	if (!strcmp("unbind", tokens[0]))
	{
		if (num_tokens != 2)
		{
			ret_str = "Syntax error";
			goto cleanup;
		}
		
		if (strlen(tokens[1]) >= 32)
		{
			ret_str = "Field too long";
			goto cleanup;
		}

		if (!strcmp(tokens[1], "shift") || !strcmp(tokens[1], "alt") || !strcmp(tokens[1], "ctrl"))
		{
			auto dest_len = strlen("unbind ") + strlen(tokens[1]) + 2;
			auto* dest = new char[dest_len];
			strcpy(dest, "unbind ");
			strcat(dest, "l");
			strcat(dest, tokens[1]);
			Cmd(dest, 0);
			dest[7] = 'r';
			ret_str = Cmd(dest, 0);
			delete[] dest;
		}

		auto* dest = new char[32];
		ret_str = g_IB_input_mapper->Unbind(tokens[1], &dest);
		if (!ret_str && *dest)
			SubtractChannel(dest, tokens[1]);

		delete[] dest;
		goto cleanup;
	}
	
	if (!strcmp("ibset", tokens[0]) || !strcmp("alias", tokens[0]))
	{
		if (num_tokens != 2 && num_tokens != 3)
		{
			ret_str = "Syntax error";
			goto cleanup;
		}

		if (strlen(tokens[1]) >= 32 || num_tokens == 3 && strlen(tokens[2]) >= 32)
		{
			ret_str = "Field too long";
			goto cleanup;
		}

		char name[32];
		strcpy(name, tokens[1]);
		strlwr(name);
		auto* int_var = m_vars.Find(name);
		if (int_var)
		{
			if (num_tokens == 3)
			{
				strcpy(int_var->var.val, tokens[2]);
				strlwr(int_var->var.val);
			}
		}
		else
		{
			IB_var var{};

			bool alias = strcmp("alias", tokens[0]) == 0;
			strcpy(var.name, name);
			if (num_tokens == 3)
				strcpy(var.val, tokens[2]);
			else
				strcpy(var.val, "0");

			var.flags = 1;
			var.cb = 0;
			var.agg = 0;
			VarSet(&var, 1, alias);
		}

		goto cleanup;
	}
	
	if (!strcmp("ibunset", tokens[0]) || !strcmp("alias", tokens[0]))
	{
		if (num_tokens != 2)
		{
			ret_str = "Syntax error";
			goto cleanup;
		}

		if (strlen(tokens[1]) >= 32)
		{
			ret_str = "Field too long";
			goto cleanup;
		}

		ret_str = VarUnset(tokens + 1, 1, 1);
		goto cleanup;
	}

	if (!strcmp("echo", tokens[0]))
	{
		TokensToStr(m_misc_str, sizeof(m_misc_str) / sizeof(m_misc_str[0]), tokens + 1, num_tokens - 1);
		ret_str = m_misc_str;
		goto cleanup;
	}

	if (!strcmp("loadbnd", tokens[0]))
	{
		if (num_tokens != 2)
		{
			ret_str = "Syntax error";
			goto cleanup;
		}

		ret_str = LoadBnd(tokens[1], 0);
		goto cleanup;
	}

	if (!strcmp("savebnd", *tokens))
	{
		if (num_tokens != 2)
		{
			ret_str = "Syntax error";
			goto cleanup;
		}

		g_IB_input_mapper->SaveBnd(tokens[1], 0);
		goto cleanup;
	}
	
	if (num_tokens > 0 && **tokens)
	{
		ret_str = ProcessCommand(tokens, num_tokens, already_down);
		goto cleanup;
	}

cleanup:

	FreeTokens(tokens, num_tokens);

	return ret_str;
}

void cIBVariableManager::SetMasterAggregation(tBindAggCallback func)
{
	m_glob_agg = func;
}

void cIBVariableManager::SetMasterProcessCallback(tBindProcCallback func)
{
	m_glob_cb = func;
}

//----- (008D2933) --------------------------------------------------------
void cIBVariableManager::SetBndSearchPath(const char* p_path)
{
	delete[] m_bnd_path;
	m_bnd_path = new char[strlen(p_path) + 1];
	strcpy(m_bnd_path, p_path);
}

//----- (008D29AD) --------------------------------------------------------
const char* cIBVariableManager::ProcessCommand(
	const char** tokens,
	int num_tokens,
	int already_down)
{
	const char* ret_val = nullptr;

	char control[32] = {};
	strcpy(control, "cmd_line");

	auto active_toggle = tokens[0][0] == '+' || tokens[0][0] == '-';
	auto* var = m_vars.Find(&tokens[0][active_toggle ? 1 : 0]);
	if (var == nullptr)
	{
		if (m_glob_cb)
		{
			char str1[128] = {}, str2[128] = {};

			GlueTokens(str1, tokens, num_tokens);
			g_IB_input_mapper->StripControl(str2, str1);
			return m_glob_cb(str2, nullptr, already_down);
		}

		return nullptr;
	}

	if (num_tokens > 2 && !strcmp(tokens[num_tokens - 2], "control:"))
	{
		strcpy(control, tokens[num_tokens - 1]);
		num_tokens -= 2;
	}
	
	char* val = nullptr;
	if (active_toggle)
	{
		auto* channel = var->channels.Find(control);
		if (!channel)
			return "Channel not found";

		channel->active = tokens[0][0] == '+';
		channel->stamp = ++var->stamp;
		val = channel->val;
	}
	else
	{
		val = var->var.val;
	}

	if (num_tokens <= 1)
	{
		if (active_toggle)
			strcpy(val, tokens[0][0] == '+' ? "1.0" : "0.0");
		else
			sprintf(val, "%8.8f", 1.0 - floor(atof(val) + 0.5));
	}
	else if (!strcmp(tokens[1], "delta"))
	{
		if (num_tokens > 2)
			sprintf(val, "%8.8f", atof(tokens[2]) + atof(val));
	}
	else if (!strcmp(tokens[1], "toggle"))
	{
		if (num_tokens == 3)
			strcpy(val, atof(val) == 0.0 ? tokens[2] : "0");
		else
			sprintf(val, "%8.8f", 1.0 - floor(atof(val) + 0.5));
	}
	else
	{
		auto cur_token = 1;
		if (strcmp(tokens[cur_token], "abs") == 0)
			++cur_token;

		strcpy(val, tokens[cur_token]);
	}
	
	tBindAggCallback agg = nullptr;
	if (var->var.agg)
		agg = var->var.agg;
	else
		agg = m_glob_agg;
	
	bool chan_ret = false;
	char final_val[32] = {};

	if (agg)
	{
		auto num_chans = var->channels.GetNumNodes();
		auto* chan_array = new intrnl_var_channel*[num_chans];
		
		var->channels.ResetVisited(nullptr);
		for (int i = 0; i < num_chans; ++i)
			chan_array[i] = var->channels.GetNextInOrder(nullptr);

		chan_ret = agg(chan_array, num_chans, final_val);
		delete[] chan_array;
	}

	if (!chan_ret)
		strcpy(final_val, var->var.val);

	if (strcmp(var->var.last_val, final_val))
	{
		if (var->var.cb)
			ret_val = var->var.cb(var->var.name, final_val, already_down);
		else if (m_glob_cb)
			ret_val = m_glob_cb(var->var.name, final_val, already_down);

		strcpy(var->var.last_val, final_val);
	}

	return ret_val;
}

//----- (008D2E59) --------------------------------------------------------
const char* cIBVariableManager::GetVarVal(char* dest, const char* val_name)
{
	auto* var = m_vars.Find(val_name);
	if (var)
	{
		strcpy(dest, var->var.val);
		return nullptr;
	}
	else
	{
		*dest = '\0';
		return "Variable not found";
	}
}
// B3EE0C: using guessed type char *off_B3EE0C[7];

//----- (008D2E9F) --------------------------------------------------------
const char* cIBVariableManager::AddChannel(const char* const* control)
{
	int cur_token = 1;
	
	bool active_toggle = control[0][1] == '+' || control[0][1] == '-';
	
	int num_tokens = 0;
	auto* tokens = Tokenize(&control[1][active_toggle ? 1 : 0], &num_tokens, 0);
	if (num_tokens == 0)
	{
		FreeTokens(tokens, num_tokens);
		return "Syntax error";
	}

	auto* var = m_vars.Find(tokens[0]);
	if (var == nullptr)
	{
		FreeTokens(tokens, num_tokens);
		return "Variable not found";
	}

	auto* channel = new intrnl_var_channel();
	channel->active = FALSE;
	if (num_tokens <= 1)
	{
		channel->adj_type = 2;
		sprintf(channel->val, "%8.8f", 0.0);
	}
	else
	{
		channel->adj_type = 0;
		if (!strcmp(tokens[1], "delta"))
		{
			channel->adj_type = 1;
			cur_token = 2;
		}
		else if (!strcmp(tokens[1], "toggle"))
		{
			channel->adj_type = 2;
			cur_token = 2;
		}
		if (num_tokens <= cur_token)
			sprintf(channel->val, "%8.8f", 0.0);
		else
			strcpy(channel->val, tokens[cur_token]);
	}

	channel->context = g_IB_input_mapper->m_context;
	var->channels.Add(*control, channel, 1);
	FreeTokens(tokens, num_tokens);

	return nullptr;
}

//----- (008D3078) --------------------------------------------------------
const char* cIBVariableManager::SubtractChannel(const char* var_name, const char* control)
{
	auto* var = m_vars.Find(var_name);
	if (!var)
		return "Variable not found";

	auto* channel = FindContextChannel(var, control);
	if (!channel)
		return "Channel not found";

	var->channels.Delete(control, 1, channel);

	return nullptr;
}

//----- (008D30DD) --------------------------------------------------------
intrnl_var_channel* cIBVariableManager::FindContextChannel(intrnl_var* var, const char* control)
{
	auto* channel = var->channels.Find(control);
	if (!channel)
		return nullptr;

	var->channels.ResetVisited(nullptr);
	var->channels.VisitBefore(channel, nullptr);

	char str[128];
	do
	{
		if (channel->context == g_IB_input_mapper->m_context)
			return channel;

		channel = var->channels.GetNextInOrder(str);
	} while (channel && !strcmp(control, str));

	return nullptr;
}

const char* g_apszBindCmds[] =
{
  "bind",
  "unbind",
  "ibset",
  "ibunset",
  "alias",
  "unalias",
  "echo",
  "loadbnd",
  "savebnd",
  nullptr
};

//----- (008D3193) --------------------------------------------------------
void cIBVariableManager::LoadBndContexted(
	const char* pFName,
	sBindContext* pContexts,
	unsigned int iNumContexts,
	cIBInputMapper** m_ppMappers)
{
	auto* pFile = fopen(pFName, "r");
	if (pFile == nullptr)
		return;

	do
	{
		char aszBuf[256] = {};
		fgets(aszBuf, 255, pFile);

		int iNumTokens = 0;
		auto* ppszTokens = Tokenize(aszBuf, &iNumTokens, 1);
		char unk_EAC40C[32] = {}; // TODO
		auto* pszPrefix = ppszTokens[0];
		if (iNumTokens == 0)
			continue;

		for (int i = 0; g_apszBindCmds[i]; ++i)
		{
			if (!stricmp(pszPrefix, g_apszBindCmds[i]))
			{
				pszPrefix = unk_EAC40C;
				break;
			}
		}

		int i = 0;
		while (i < iNumContexts && stricmp(pszPrefix, pContexts[i].aszStr))
			++i;

		if (i < iNumContexts)
		{
			auto iContext = pContexts[i].iContext;
			for (int j = 0; j < 32; ++j)
			{
				if (((iContext >> j) & 1) != 0)
				{
					if (!m_ppMappers[j])
					{
						m_ppMappers[j] = new cIBInputMapper();
						m_ppMappers[j]->m_context = 1 << j;
					}
					g_IB_input_mapper = m_ppMappers[j];
					Cmd(&aszBuf[strlen(pszPrefix)], 0);
				}
			}
		}
		FreeTokens(ppszTokens, iNumTokens);
	} while (!feof(pFile));

	fclose(pFile);
}
// 8D3321: variable 'v5' is possibly undefined

//----- (008D33CB) --------------------------------------------------------
const char* cIBVariableManager::LoadBnd(const char* bnd_filename, const char* prefix)
{
	auto* fp = fopen(bnd_filename, "r");
	if (!fp)
		return "File not found";

	fseek(fp, 0, 0);
	
	do
	{
		char str[128];
		fgets(str, sizeof(str) / sizeof(str[0]) - 1, fp);
		if (prefix)
		{
			int num_tokens = 0;
			auto* tokens = Tokenize(str, &num_tokens, 1);
			if (num_tokens)
			{
				if (!strcmp(tokens[0], prefix))
					Cmd(&str[strlen(tokens[0])], 0);
			}
			FreeTokens(tokens, num_tokens);
		}
		else
		{
			Cmd(str, 0);
		}
	} while (!feof(fp));

	fclose(fp);

	return nullptr;
}
// B3EE18: using guessed type char *off_B3EE18[4];

//----- (008D34EF) --------------------------------------------------------
char** cIBVariableManager::Tokenize(const char* cmd, int* num_tokens, int expand_alias)
{
	if (!cmd)
		return nullptr;

	if (!cmd[0] || cmd[0] == '\r' || cmd[0] == '\n')
	{
		auto* tokens = new char* [1];
		tokens[0] = new char[1];
		tokens[0][0] = '\0';
		*num_tokens = 1;

		return tokens;
	}

	auto tmp_tokens_num = 32;
	auto* tmp_tokens = new char*[tmp_tokens_num];
	for (int i = 0; i < tmp_tokens_num; ++i)
		tmp_tokens[i] = new char[128];
	
	bool expand_var = false;
	cmd = SkipWhiteSpace(cmd);
	int num = 0;
	char separator = '\0';

	for (int i = 0; i < 32 && cmd[0] && cmd[0] != '\r' && cmd[0] != '\n'; ++i)
	{
		if (*cmd == '"')
		{
			++cmd;
			separator = '"';
		}
		else if (*cmd == ';')
		{
			if (num < 1)
				break;
			if (num == 1)
			{
				if (strcmp(*tmp_tokens, "bind") && strcmp(*tmp_tokens, "unbind"))
					break;
			}
			else if (strcmp(tmp_tokens[num - 1], "control:"))
			{
				break;
			}
			separator = ' ';
		}
		else
		{
			separator = ' ';
		}

		if (*cmd == '=')
		{
			strcpy(tmp_tokens[num], "=");
			++i;
			++num;
			++cmd;
		}
		else
		{
			if (*cmd == '$')
			{
				if (num - 1 >= 0)
				{
					if (strcmp(tmp_tokens[num - 1], "bind") && strcmp(tmp_tokens[num - 1], "unbind"))
						expand_var = 1;
				}
				else
				{
					expand_var = 1;
				}
			}
			GetChunk(tmp_tokens[num], &cmd, separator, '\0');
			auto* var = m_vars.Find(tmp_tokens[num]);
			if (expand_var || expand_alias && var && (var->var.flags & 2) != 0)
			{
				GetVarVal(tmp_tokens[num], &tmp_tokens[num][expand_var]);
				int num_expanded_tokens = 0;
				auto* expanded_tokens = Tokenize(tmp_tokens[num], &num_expanded_tokens, 1);
				for (int j = 0; j < num_expanded_tokens; ++j)
				{
					if (j + num < 32)
					{
						delete[] (&tmp_tokens[j])[num];
						(&tmp_tokens[j])[num] = expanded_tokens[j];
					}
					else
					{
						delete[] expanded_tokens[j];
					}
				}
				if (num_expanded_tokens)
					delete[] expanded_tokens;
				expand_var = 0;
				num = num + num_expanded_tokens - 1;
			}
			++num;
		}
		cmd = SkipWhiteSpace(cmd);
	}

	*num_tokens = num;
	auto* tokens = new char*[num];
	for (int i = 0; i < num; ++i)
	{
		tokens[i] = new char[strlen(tmp_tokens[i]) + 1];
		strcpy(tokens[i], tmp_tokens[i]);
	}

	FreeTokens(tmp_tokens, tmp_tokens_num);
	
	return tokens;
}

void cIBVariableManager::GlueTokens(char* dest, const char* const* tokens, int num_tokens)
{
	*dest = '\0';
	for (int i = 0; i < num_tokens; ++i)
	{
		if (i != 0)
			strcat(dest, " ");
		strcat(dest, tokens[i]);
	}
}

void cIBVariableManager::FreeTokens(char** tokens, int num_tokens)
{
	for (int i = 0; i < num_tokens; ++i)
		delete[] tokens[i];

	delete[] tokens;
}

void cIBVariableManager::TokensToStr(char* str, unsigned int str_len, const char** tokens, int num_tokens)
{
	m_misc_str[0] = '\0'; // TODO: ???
	for (int i = 0; i < num_tokens; ++i)
	{
		if (strlen(tokens[i]) + strlen(str) >= str_len)
			break;

		strcat(str, tokens[i]);
		if (i < num_tokens - 1)
			strcat(str, " ");
	}
}

const char* SkipWhiteSpace(const char* str)
{
	while (true)
	{
		if (*str != ' ' || *str == '\0' || *str == '\r' || *str == '\n')
			break;
		++str;
	}

	return str;
}

void GetChunk(char* dest, const char** src, char ch1, char ch2) // TODO: make src const
{
	char sch;
	while (true)
	{
		sch = **src;
		if (sch == ch1 || sch == ch2 || sch == '=' || sch == '\0' || sch == '\r' || sch == '\n')
			break;
		*dest++ = *(*src)++;
	}

	*dest = '\0';

	if (sch != '=' && sch != '\0' && sch != '\r' && sch != '\n')
		++*src;
}
