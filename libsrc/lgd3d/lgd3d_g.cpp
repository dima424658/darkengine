#include <lgd3d.h>

extern "C" {
    uchar lgd3d_punt_buffer;
    BOOL lgd3d_punt_d3d;

    uchar* texture_clut;
    uchar* lgd3d_clut;

    BOOL zlinear;

    void lgd3d_set_zlinear(BOOL lin)
    {
        zlinear = lin;
    }
}