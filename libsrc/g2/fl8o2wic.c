// Fl8o2wi2 Assembly function C wrappers.  Needed for MSVC++.
// started 9/9/96 by phs

#ifdef _MSC_VER

#include <types.h>
#include <fix.h>

void flat8_flat8_opaque_p2_wrap_il (int x_left, int x_right, fix u, fix v)
{
	extern void flat8_flat8_opaque_p2_wrap_il_asm (int x_left, int x_right, fix u, fix v);
	__asm
	{
		mov		eax, x_left
		mov		edx, x_right
		mov		ebx, u
		mov		ecx, v
		call	flat8_flat8_opaque_p2_wrap_il_asm
	}
}



#endif