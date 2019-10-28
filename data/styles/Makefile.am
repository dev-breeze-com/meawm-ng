stylesdir = $(pkgdatadir)/styles
SUBDIRS = freedesktop
styles_DATA = logo.conf

configfiles = $(styles_DATA)
CLEANFILES = $(configfiles)
EXTRA_DIST = logo.conf.in
all-local: $(configfiles)

$(configfiles): %.conf: %.conf.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@," $< > $@

