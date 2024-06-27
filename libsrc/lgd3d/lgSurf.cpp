#include <lgSurf_i.h>

class cLGSurface : public ILGSurface, public ILGDD4Surface
{

};

BOOL CreateLGSurface(ILGSurface **ppILGSurf)
{
    *ppILGSurf = new cLGSurface();
    return *ppILGSurf != nullptr;
}