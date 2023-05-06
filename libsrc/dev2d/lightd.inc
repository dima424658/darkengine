; $Header: x:/prj/tech/libsrc/dev2d/RCS/lightd.inc 1.1 1997/02/17 14:47:02 KEVIN Exp $

extern _gdd_light_buffer:byte

GDC_LIGHT_BUFFER_SIZE   equ 1024t
light_buffer_end        equ _gdd_light_buffer[GDC_LIGHT_BUFFER_SIZE]
light_buffer_last       equ _gdd_light_buffer[GDC_LIGHT_BUFFER_SIZE-1]
