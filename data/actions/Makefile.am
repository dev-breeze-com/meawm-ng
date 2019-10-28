SUBDIRS = \
   sloppyfocusopaque \
   sloppyfocusnonopaque \
   clicktofocusopaque \
   clicktofocusnonopaque
actionsdir = $(pkgdatadir)/actions
actions_DATA = \
	key-bindings.conf \
	button-mappings.conf \
	common.conf

configfiles = $(actions_DATA)
CLEANFILES = $(configfiles)
EXTRA_DIST = \
	key-bindings.conf.in \
	button-mappings.conf.in \
	common.conf.in
all-local: $(configfiles)

$(configfiles): %.conf: %.conf.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@," $< > $@
