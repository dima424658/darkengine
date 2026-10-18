///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/g2/RCS/g2makewr.c $
// $Author: KEVIN $
// $Date: 1997/05/16 09:51:51 $
// $Revision: 1.5 $
//

void g2_make_writable(void)
{
    /*
     * The original x86 rasterizers patched constants into their own machine
     * code.  Portable C/C++ replacements keep their state in ordinary data,
     * so executable pages never need to be made writable.
     */
}
