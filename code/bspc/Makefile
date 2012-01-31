#
# Makefile for the BSPC tool for the Gladiator Bot
# Intended for gcc/Linux
#
# TTimo 5/15/2001
# some cleanup .. only used on i386 for GtkRadiant setups AFAIK .. removing the i386 tag
# TODO: the intermediate object files should go into their own directory
#   specially for ../botlib and ../qcommon, the compilation flags on those might not be what you expect

#ARCH=i386
CC=gcc
BASE_CFLAGS=-Dstricmp=strcasecmp

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -m486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2 \
	-malign-jumps=2 -malign-functions=2 -DLINUX -DBSPC
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

LDFLAGS=-ldl -lm -lpthread

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<

#############################################################################
# SETUP AND BUILD BSPC
#############################################################################

.c.o:
	$(DO_CC)

GAME_OBJS = \
	_files.o\
	aas_areamerging.o\
	aas_cfg.o\
	aas_create.o\
	aas_edgemelting.o\
	aas_facemerging.o\
	aas_file.o\
	aas_gsubdiv.o\
	aas_map.o\
	aas_prunenodes.o\
	aas_store.o\
	be_aas_bspc.o\
	../botlib/be_aas_bspq3.o\
	../botlib/be_aas_cluster.o\
	../botlib/be_aas_move.o\
	../botlib/be_aas_optimize.o\
	../botlib/be_aas_reach.o\
	../botlib/be_aas_sample.o\
	brushbsp.o\
	bspc.o\
	../qcommon/cm_load.o\
	../qcommon/cm_patch.o\
	../qcommon/cm_test.o\
	../qcommon/cm_trace.o\
	csg.o\
	glfile.o\
	l_bsp_ent.o\
	l_bsp_hl.o\
	l_bsp_q1.o\
	l_bsp_q2.o\
	l_bsp_q3.o\
	l_bsp_sin.o\
	l_cmd.o\
	../botlib/l_libvar.o\
	l_log.o\
	l_math.o\
	l_mem.o\
	l_poly.o\
	../botlib/l_precomp.o\
	l_qfiles.o\
	../botlib/l_script.o\
	../botlib/l_struct.o\
	l_threads.o\
	l_utils.o\
	leakfile.o\
	map.o\
	map_hl.o\
	map_q1.o\
	map_q2.o\
	map_q3.o\
	map_sin.o\
	../qcommon/md4.o\
	nodraw.o\
	portals.o\
	textures.o\
	tree.o\
	../qcommon/unzip.o

        #tetrahedron.o

bspc : $(GAME_OBJS)
	$(CC) $(CFLAGS) -o $@ $(GAME_OBJS) $(LDFLAGS)
	strip $@


#############################################################################
# MISC
#############################################################################

clean:
	-rm -f $(GAME_OBJS)

depend:
	gcc -MM $(GAME_OBJS:.o=.c)

#install:
#	cp bspci386 ..

#
# From "make depend"
#

