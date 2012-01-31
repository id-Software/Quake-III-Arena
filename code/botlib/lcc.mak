#
# Makefile for Gladiator Bot library: gladiator.dll
# Intended for LCC-Win32
#

CC=lcc
CFLAGS=-DC_ONLY -o
OBJS= be_aas_bspq2.obj \
	be_aas_bsphl.obj \
	be_aas_cluster.obj \
	be_aas_debug.obj \
	be_aas_entity.obj \
	be_aas_file.obj \
	be_aas_light.obj \
	be_aas_main.obj \
	be_aas_move.obj \
	be_aas_optimize.obj \
	be_aas_reach.obj \
	be_aas_route.obj \
	be_aas_routealt.obj \
	be_aas_sample.obj \
	be_aas_sound.obj \
	be_ai2_dm.obj \
	be_ai2_dmnet.obj \
	be_ai2_main.obj \
	be_ai_char.obj \
	be_ai_chat.obj \
	be_ai_goal.obj \
	be_ai_load.obj \
	be_ai_move.obj \
	be_ai_weap.obj \
	be_ai_weight.obj \
	be_ea.obj \
	be_interface.obj \
	l_crc.obj \
	l_libvar.obj \
	l_log.obj \
	l_memory.obj \
	l_precomp.obj \
	l_script.obj \
	l_struct.obj \
	l_utils.obj \
	q_shared.obj

all:	gladiator.dll

gladiator.dll:	$(OBJS)
	lcclnk -dll -entry GetBotAPI *.obj botlib.def -o gladiator.dll

clean:
	del *.obj gladiator.dll

%.obj: %.c
	$(CC) $(CFLAGS) $<

