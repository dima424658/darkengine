///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/g2/RCS/g2makewr.c $
// $Author: KEVIN $
// $Date: 1997/05/16 09:51:51 $
// $Revision: 1.5 $
//

#ifdef _WIN32

#include <lg.h>
#include <codewrit.h>

extern void opaque_8to8_buffer_il();
extern void opaque_8to8_raster();
extern void opaque_clut_8to8_raster();
extern void gri_check_edge_y_update();
extern void opaque_clut_nowrap_8to8_il();
extern void trans_clut_nowrap_8to8_il();

void g2_make_writable(void)
{
    MakeFunctionWritable(opaque_8to8_buffer_il,       128);
    MakeFunctionWritable(opaque_8to8_raster,          384);
    MakeFunctionWritable(opaque_clut_8to8_raster,     384);
    MakeFunctionWritable(opaque_clut_nowrap_8to8_il,  128);
    MakeFunctionWritable(trans_clut_nowrap_8to8_il,   128);
    MakeFunctionWritable(gri_check_edge_y_update,     384);
}

#endif
