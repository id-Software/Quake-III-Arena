#
# Makefile for Gladiator Bot library: gladiator.so
# Intended for gcc/Linux
#

ARCH=i386
CC=gcc
BASE_CFLAGS=-Dstricmp=strcasecmp

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -m486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2 \
	-malign-jumps=2 -malign-functions=2
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

LDFLAGS=-ldl -lm
SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

DO_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<

#############################################################################
# SETUP AND BUILD
# GLADIATOR BOT
#############################################################################

.c.o:
	$(DO_CC)

GAME_OBJS = \
	be_aas_bsphl.o\
	be_aas_bspq2.o\
	be_aas_cluster.o\
	be_aas_debug.o\
	be_aas_entity.o\
	be_aas_file.o\
	be_aas_light.o\
	be_aas_main.o\
	be_aas_move.o\
	be_aas_optimize.o\
	be_aas_reach.o\
	be_aas_route.o\
	be_aas_routealt.o\
	be_aas_sample.o\
	be_aas_sound.o\
	be_ai2_dmq2.o\
	be_ai2_dmhl.o\
	be_ai2_dmnet.o\
	be_ai2_main.o\
	be_ai_char.o\
	be_ai_chat.o\
	be_ai_goal.o\
	be_ai_load.o\
	be_ai_move.o\
	be_ai_weap.o\
	be_ai_weight.o\
	be_ea.o\
	be_interface.o\
	l_crc.o\
	l_libvar.o\
	l_log.o\
	l_memory.o\
	l_precomp.o\
	l_script.o\
	l_struct.o\
	l_utils.o\
	q_shared.o

glad$(ARCH).$(SHLIBEXT) : $(GAME_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(GAME_OBJS)


#############################################################################
# MISC
#############################################################################

clean:
	-rm -f $(GAME_OBJS)

depend:
	gcc -MM $(GAME_OBJS:.o=.c)


install:
	cp gladiator.so ..

#
# From "make depend"
#

