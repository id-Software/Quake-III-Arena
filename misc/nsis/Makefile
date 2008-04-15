VERSION=1.35_SVN
RELEASE=0

all: ioquake3-$(VERSION)-$(RELEASE).x86.exe

ioquake3.nsi: ioquake3.nsi.in
	sed 's/XXXVERSIONXXX/$(VERSION)/;s/XXXRELEASEXXX/$(RELEASE)/' < $< > $@

ioquake3-$(VERSION)-$(RELEASE).x86.exe: ioquake3.nsi
	makensis ioquake3.nsi

clean:
	rm -rf *.exe ioquake3.nsi

.PHONY: all clean
