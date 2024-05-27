#include "inpbnd_i.h"

#include <cstdlib>


BOOL CDECL IBMaxActiveAgg(intrnl_var_channel** chans, long num_chans, char* str)
{
	bool first = false;
	double max = 0.0;
	const char* maxStr = nullptr;

	for (int i = 0; i < num_chans; ++i)
	{
		if (chans[i]->active == TRUE)
		{
			auto cur = atof(chans[i]->val);
			if (cur > max || !first)
			{
				first = 1;
				max = cur;
				maxStr = chans[i]->val;
			}
		}
	}

	if (first)
		strcpy(str, maxStr);

	return first;
}

BOOL CDECL IBAddActiveAgg(intrnl_var_channel** chans, long num_chans, char* str)
{
	auto found = false;
	auto sum = 0.0;
	for (int i = 0; i < num_chans; ++i)
	{
		if (chans[i]->active)
		{
			found = true;
			sum = atof(chans[i]->val) + sum;
		}
	}

	if (found)
		sprintf(str, "%8.8f", sum);

	return found;
}

cIBJoyAxisProcess::cIBJoyAxisProcess()
	: m_deadzone_x{}, m_deadzone_y{}, m_deadzone_z{}, m_deadzone_r{} { }

void cIBJoyAxisProcess::ProcessXY(double* x, double* y)
{
	ApplyDeadzone(x, m_deadzone_x);
	ApplyDeadzone(y, m_deadzone_y);
}

void cIBJoyAxisProcess::ProcessZR(double* z, double* r)
{
	ApplyDeadzone(z, m_deadzone_z);
	ApplyDeadzone(r, m_deadzone_r);
}

void cIBJoyAxisProcess::ApplyDeadzone(double* axis, double deadzone)
{
	auto aAxis = std::abs(*axis);

	if (aAxis >= deadzone)
		*axis = std::abs((aAxis - deadzone) * (1.0 / (1.0 - deadzone)));
	else
		*axis = 0;
}