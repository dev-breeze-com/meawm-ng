menusdir = $(pkgdatadir)/menus
SUBDIRS = icons
menus_DATA = menu.conf

configfiles = $(menus_DATA)
CLEANFILES = $(configfiles)
EXTRA_DIST = menu.conf.in
all-local: $(configfiles)

$(configfiles): %.conf: %.conf.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@," $< > $@

