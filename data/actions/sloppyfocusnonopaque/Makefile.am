sloppyfocusnonopaquedir = $(pkgdatadir)/actions/sloppyfocusnonopaque
sloppyfocusnonopaque_DATA = action.conf

configfiles = $(sloppyfocusnonopaque_DATA)
CLEANFILES = $(configfiles)
EXTRA_DIST = action.conf.in
all-local: $(configfiles)

$(configfiles): %.conf: %.conf.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@," $< > $@
