VERSION=1.35_SVN
RELEASE=0
ARCH=x86

all: ioquake3-$(VERSION)-$(RELEASE).$(ARCH).exe

ioquake3.nsi: ioquake3.nsi.in
	sed 's/XXXVERSIONXXX/$(VERSION)/;s/XXXRELEASEXXX/$(RELEASE)/;s/x86/$(ARCH)/g' < $< > $@

ioquake3-$(VERSION)-$(RELEASE).$(ARCH).exe: ioquake3.nsi
	makensis ioquake3.nsi

clean:
	rm -rf *.exe ioquake3.nsi

.PHONY: all clean
