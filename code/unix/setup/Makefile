VERSION=FIXME
RELEASE=1

all:
	VERSION=$(VERSION) RELEASE=$(RELEASE) ./doit

sign:
	for i in *.run; do \
		gpg -bao $$i.asc $$i; \
	done

clean:
	rm -rf *.run image

.PHONY: all sign clean
