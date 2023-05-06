#include "inpbnd_i.h"

#include <cstdlib>

BOOL IBAddActiveAgg(_intrnl_var_channel**, long, char*)
{
	return FALSE; // TODO
}

tResult LGAPI _CreateInputBinder(REFIID, IInputBinder** ppInputBinder, IUnknown* pOuter)
{
	return 0; // TODO
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