#
# Makefile for the BSPC tool for the Gladiator Bot
# Intended for LCC-Win32
#

CC=lcc
CFLAGS=-DC_ONLY -o
OBJS= _files.obj\
	aas_areamerging.obj\
	aas_cfg.obj\
	aas_create.obj\
	aas_edgemelting.obj\
	aas_facemerging.obj\
	aas_file.obj\
	aas_gsubdiv.obj\
	aas_map.obj\
	aas_prunenodes.obj\
	aas_store.obj\
	brushbsp.obj\
	bspc.obj\
	csg.obj\
	faces.obj\
	glfile.obj\
	l_bsp_hl.obj\
	l_bsp_q1.obj\
	l_bsp_q2.obj\
	l_bsp_sin.obj\
	l_cmd.obj\
	l_log.obj\
	l_math.obj\
	l_mem.obj\
	l_poly.obj\
	l_qfiles.obj\
	l_script.obj\
	l_threads.obj\
	l_utils.obj\
	leakfile.obj\
	map.obj\
	map_hl.obj\
	map_q1.obj\
	map_q2.obj\
	map_q2_new.obj\
	map_sin.obj\
	nodraw.obj\
	portals.obj\
	prtfile.obj\
	textures.obj\
	tree.obj\
	writebsp.obj

all:	bspc.exe

bspc.exe:	$(OBJS)
	lcclnk

clean:
	del *.obj bspc.exe

%.obj: %.c
	$(CC) $(CFLAGS) $<

