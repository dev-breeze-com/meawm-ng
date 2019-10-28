freedesktopdir = $(pkgdatadir)/styles/freedesktop
freedesktop_DATA = style.conf \
	window.conf \
	menu.conf \
	dockapp.conf \
	root.conf \
	freedesktop.png \
	freedesktop.svg

configfiles = style.conf \
	window.conf \
	menu.conf \
	dockapp.conf \
	root.conf
CLEANFILES = $(configfiles)
EXTRA_DIST = \
	style.conf.in \
	window.conf.in \
	menu.conf.in \
	dockapp.conf.in \
	root.conf.in \
	freedesktop.png \
	freedesktop.svg
all-local: $(configfiles)

$(configfiles): %.conf: %.conf.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@," $< > $@

