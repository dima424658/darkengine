# $Header: r:/t2repos/thief2/src/portal/makefile,v 1.36 1998/05/15 18:23:43 buzzard Exp $

MAKETOP = $(CWD)

PROJECT = cam
PRJBACK ?= r:\prj

%include $(MRC)\env.rc

%include $(MRC)\init.rc

EXETARGS = merge 

#
# all portal objects, divvied up into major types as self-commenting

world_rep_objs = wrfunc wrcast bspsphr
world_rep_dbg_objs = wrdbg
portal_support_objs = animlit oracle
portal_render_objs = portal portdraw portlit portclip portbg portpick \
                        portwatr porthw portsky
portal_surface_objs = portsurf surfaces ptsurf ptsurf16
portal_mapper_objs = pt_mem pt_clut

# the following have been replaced by equivalent code in the g2 library:
# duv mapper pt_main
# ptfloat ptpersp ptlinear ptmap ptmap_8 ptmap_n ptmapper

#
# all portal objects

portal_objs = $(world_rep_objs) \
              $(world_rep_dbg_objs) \
              $(portal_support_objs) \
              $(portal_render_objs) \
              $(portal_surface_objs) \
              $(portal_mapper_objs)

portal_HDRS = wr.h portal.h portsky.h pt.h animlit.h wrobj.h wrtype.h bspsphr.h

portal_newline_hack=" "
portal_newline=$(portal_newline_hack,W\n)
portal_dependent_includes="\#include \<r3ds.h\>"$(portal_newline)"\#include \<wr.h\>"$(portal_newline)"\#include \<objtype.h\>"

portal.h .REREAD: port.h 
	$(eko1) $(@,F) \<-- $(?,F)
	%do comp_h h_pre=$(portal_dependent_includes) h_in="$^" h_out="$(@,F)"

wr_newline_hack=" "
wr_newline=$(portal_newline_hack,W\n)
wr_dependent_includes="\#include \<r3d.h\>"$(portal_newline)"\#include \<wrtype.h\>"

wr.h .REREAD: wrlimit.h wrdb.h wrfunc.h
	$(eko1) $(@,F) \<-- $(?,F)
	%do comp_h h_pre=$(wr_dependent_includes) h_in="$^" h_out="$(@,F)"

bspsphr.h .REREAD: bspsphr.h
	$(eko1) $(@,F) \<-- $(?,F)
        %do comp_h h_pre=$h_in="$^" h_out="$(@,F)"

# standalone test program
port_objs = main wrdb
port_libs = fix g2 dev2d lg r3d fault mprintf matrix portal eos prof timer

# standalone test program which doesn't go via library,
# so it's simpler to link
pt_objs = $(portal_objs) $(port_objs)
pt_libs = fix g2 dev2d lg r3d fault mprintf matrix eos prof timer

# program to test portal-tmap-memory-manager
memtest_objs = pt_mem memtest
memtest_libs = lg mprintf

# program to optimize portal databases
merge_objs = merge
merge_libs = lg mprintf matrix lgalloc
merge_STACK = 16384

# program to performance test portal & 2d
#perftest_objs = perftest perfcore char unlit
#perftest_libs = fix g2 dev2d lg r3d mprintf matrix portal timer

dev_lib_flav = dbg

dev_pt = pt
dev_port = port
dev_memtest = memtest
#dev_perftest = perftest

%include $(MRC)\make.rc
