#include <windows.h>
_Static_assert(sizeof(DWORD) == 4, "DWORD width");
_Static_assert(sizeof(IUnknownVtbl) == 3 * sizeof(void *), "IUnknown layout");
int CheckCompatC(const GUID *id)
{
 return id->Data1 != 0x12345678 || id->Data2 != 0xabcd || id->Data4[7] != 8;
}
