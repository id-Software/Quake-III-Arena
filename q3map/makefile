
CFLAGS = -c
LDFLAGS =
ODIR = /q3/q3map

EXEBASE = q3map
EXE = $(ODIR)/$(EXEBASE)
all: $(EXE)

_irix:
	make "CFLAGS = -c -Ofast=ip27 -OPT:IEEE_arithmetic=3 -I../common -Xcpluscomm " "LDFLAGS = -Ofast=ip27 -OPT:IEEE_arithmetic=3 -g"
	
_irixdebug:
	make "CFLAGS = -c -O2 -g -I../common -Xcpluscomm" "LDFLAGS = -g"
	
_irixinst:
	make "_irix"
	make "install"

clean:
	rm -f $(ODIR)/*.o $(EXE)

install:
	cp $(EXE) /quake3_bin
	chmod 0777 /quake3_bin/$(EXEBASE)

installtest:
	cp $(EXE) /quake3_bin/$(EXEBASE)_test
	chmod 0777 /quake3_bin/$(EXEBASE)_test


FILES = $(ODIR)/fog.o $(ODIR)/brush.o $(ODIR)/tjunction.o $(ODIR)/vis.o $(ODIR)/visflow.o \
$(ODIR)/light.o $(ODIR)/lightmaps.o $(ODIR)/bspfile.o \
$(ODIR)/cmdlib.o $(ODIR)/patch.o $(ODIR)/mesh.o $(ODIR)/nodraw.o $(ODIR)/glfile.o \
$(ODIR)/leakfile.o $(ODIR)/map.o $(ODIR)/mathlib.o $(ODIR)/polylib.o $(ODIR)/aselib.o \
$(ODIR)/imagelib.o $(ODIR)/portals.o $(ODIR)/prtfile.o $(ODIR)/bsp.o $(ODIR)/surface.o \
$(ODIR)/scriplib.o $(ODIR)/shaders.o $(ODIR)/threads.o $(ODIR)/tree.o \
$(ODIR)/writebsp.o $(ODIR)/facebsp.o $(ODIR)/misc_model.o $(ODIR)/light_trace.o

$(EXE) : $(FILES)
	cc -o $(EXE) $(LDFLAGS) $(FILES) -lm
	
$(ODIR)/surface.o : surface.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/fog.o : fog.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/brush.o : brush.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/tjunction.o : tjunction.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/lightmaps.o : lightmaps.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/brushbsp.o : brushbsp.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/facebsp.o : facebsp.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/patch.o : patch.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/mesh.o : mesh.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/misc_model.o : misc_model.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/nodraw.o : nodraw.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/glfile.o : glfile.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/leakfile.o : leakfile.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/map.o : map.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/portals.o : portals.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/prtfile.o : prtfile.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/bsp.o : bsp.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/tree.o : tree.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/shaders.o : shaders.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/writebsp.o : writebsp.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/csg.o : csg.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i


$(ODIR)/vis.o : vis.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/visflow.o : visflow.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i


$(ODIR)/light.o : light.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/light_trace.o : light_trace.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i

$(ODIR)/cmdlib.o : ../common/cmdlib.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/mathlib.o : ../common/mathlib.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/polylib.o : ../common/polylib.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/aselib.o : ../common/aselib.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/imagelib.o : ../common/imagelib.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/scriplib.o : ../common/scriplib.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/threads.o : ../common/threads.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i
$(ODIR)/bspfile.o : ../common/bspfile.c
	cc $(CFLAGS) -E $? | tr -d '\015' > /tmp/temp.i
	cc $(CFLAGS) -o $@ /tmp/temp.i


