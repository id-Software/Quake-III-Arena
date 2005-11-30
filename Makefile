VERSION=1.33_SVN$(shell LANG=C svnversion .)

release debug clean distclean copyfiles installer:
	$(MAKE) -C code/unix $@

dist:
	rm -rf quake3-$(VERSION)
	svn export . quake3-$(VERSION)
	which convert >/dev/null 2>&1 && convert web/images/thenameofthisprojectis3.jpg quake3-$(VERSION)/code/unix/setup/splash.xpm || true
	rm -rf quake3-$(VERSION)/web
	tar --force-local -cjf quake3-$(VERSION).tar.bz2 quake3-$(VERSION)
	rm -rf quake3-$(VERSION)

.PHONY: release debug clean distclean copyfiles installer
