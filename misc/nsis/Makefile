ifndef VERSION
VERSION=1.36_SVN
endif
ifndef RELEASE
RELEASE=0
endif
ifndef ARCH
ARCH=x86
endif
ifndef INSTALLDIR
INSTALLDIR=.
endif

ifeq ($(ARCH),x64)
  SDLDLL=SDL64.dll
else
  SDLDLL=SDL.dll
endif

all: ioquake3-$(VERSION)-$(RELEASE).$(ARCH).exe

ioquake3.$(ARCH).nsi: ioquake3.nsi.in
	sed 's/XXXVERSIONXXX/$(VERSION)/;s/XXXRELEASEXXX/$(RELEASE)/;s/x86/$(ARCH)/;s/SDL.dll/$(SDLDLL)/g' < $< > $@

ioquake3-$(VERSION)-$(RELEASE).$(ARCH).exe: ioquake3.$(ARCH).nsi
	makensis ioquake3.$(ARCH).nsi

clean:
	rm -rf *.exe ioquake3.$(ARCH).nsi

install:
	mkdir -p $(INSTALLDIR)
	mv ioquake3-$(VERSION)-$(RELEASE).$(ARCH).exe $(INSTALLDIR)

.PHONY: all clean

