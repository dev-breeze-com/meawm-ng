clicktofocusnonopaquedir = $(pkgdatadir)/actions/clicktofocusnonopaque
clicktofocusnonopaque_DATA = action.conf

configfiles = $(clicktofocusnonopaque_DATA)
CLEANFILES = $(configfiles)
EXTRA_DIST = action.conf.in
all-local: $(configfiles)

$(configfiles): %.conf: %.conf.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@," $< > $@
