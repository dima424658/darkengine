#pragma once

#include <inpbnd_i.h>

class cIBInputMapper;

class cIBVariableManager
{
public:
	cIBVariableManager();
	cIBVariableManager(IB_var* var);
	cIBVariableManager(IB_var* var, int num);
	~cIBVariableManager();

	void Init();
	int VarSet(IB_var* vars, int alias);
	int VarSet(IB_var* vars, int num, int alias);
	const char* VarUnset(const char* const* var_names, int user);
	const char* VarUnset(const char* const* var_names, int num, int user);
	const char* VarUnsetAll();
	const char* Cmd(const char* cmd, int already_down);
	void SetMasterAggregation(tBindAggCallback func);
	void SetMasterProcessCallback(tBindProcCallback func);
	void SetBndSearchPath(const char* p_path);
	const char* ProcessCommand(const char** tokens, int num_tokens, int already_down);
	const char* GetVarVal(char* dest, const char* val_name);
	const char* AddChannel(const char* const* control);
	const char* SubtractChannel(const char* var_name, const char* control);
	intrnl_var_channel* FindContextChannel(intrnl_var* var, const char* control);
	void LoadBndContexted(const char* pFName, sBindContext* pContexts, unsigned int iNumContexts, cIBInputMapper** m_ppMappers);
	const char* LoadBnd(const char* bnd_filename, const char* prefix);
	char** Tokenize(const char* cmd, int* num_tokens, int expand_alias);
	
	void GlueTokens(char* dest, const char* const* tokens, int num_tokens);
	void FreeTokens(char** tokens, int num_tokens);
	void TokensToStr(char* str, unsigned int str_len, const char** tokens, int num_tokens);

private:
	aatree<intrnl_var> m_vars;
	tBindAggCallback m_glob_agg;
	tBindProcCallback m_glob_cb;
	char m_misc_str[128];
	char* m_bnd_path;
};
