SUBDIRS = actions styles menus scripts
pkgdata_DATA = meawm_ngrc meawm_ng.dtd

configfiles = pre-action.conf \
	pre-style.conf \
	post-action.conf \
	post-style.conf

CLEANFILES = meawm_ngrc meawm_ng.dtd $(configfiles)
EXTRA_DIST = \
	meawm_ngrc.in \
	meawm_ng.dtd.in \
	pre-action.conf.in \
	pre-style.conf.in \
	post-action.conf.in \
	post-style.conf.in \
	Meawm_NG.session
all-local: meawm_ngrc meawm_ng.dtd $(configfiles)

meawm_ngrc: meawm_ngrc.in
	@regex_cmd@ -e "s,@dialogcommand@,@dialog_command@," -e "s,@pkgdatadir@,$(pkgdatadir)," $< > $@

meawm_ng.dtd: meawm_ng.dtd.in
	@regex_cmd@ -e "s,@pkgversion@,@VERSION@,"  $< > $@

$(configfiles): %.conf: %.conf.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@," $< > $@
