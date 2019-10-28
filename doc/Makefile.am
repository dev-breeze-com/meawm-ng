man_MANS = meawm_ng.1

all-local: meawm_ng.1
CLEANFILES = meawm_ng.1
EXTRA_DIST = meawm_ng.1.in

meawm_ng.1: meawm_ng.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," -e "s,@pkgversion@,@VERSION@,"  $< > $@
