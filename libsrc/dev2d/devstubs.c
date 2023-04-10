// File to stub out all the VGA & VBE functions that we don't need to use
// when compiling the 2D under MSVC.
// started by phs, 9/6/96

#ifdef _MSC_VER

#include <grnull.h>

void VBEInit(void)
{
	gr_warn();
}

void vbe_close(void)
{
	gr_warn();
}

void vbe_set_mode(void)
{
	gr_warn();
}

void vbe_get_mode(void)
{
	gr_warn();
}

void vbe_set_width(void)
{
	gr_warn();
}

void vbe_get_width(void)
{
	gr_warn();
}

void vbe_set_focus(void)
{
	gr_warn();
}

void vbe_get_focus(void)
{
	gr_warn();
}

void vga_set_mode(void)
{
	gr_warn();
}

void vga_save_state(void)
{
	gr_warn();
}

void vga_restore_state(void)
{
	gr_warn();
}

void vga_stat_htrace(void)
{
	gr_warn();
}

void vga_stat_vtrace(void)
{
	gr_warn();
}

void vga_set_pal(void)
{
	gr_warn();
}

void vga_get_pal(void)
{
	gr_warn();
}

void vga_set_width(void)
{
	gr_warn();
}

void vga_set_focus(void)
{
	gr_warn();
}

void vga_get_focus(void)
{
	gr_warn();
}

void save_gen_state(void)
{
	gr_warn();
}

void restore_gen_state(void)
{
	gr_warn();
}

#endif