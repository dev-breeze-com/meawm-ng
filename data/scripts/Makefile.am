scriptsdir = $(pkgdatadir)/scripts
SUBDIRS = icons
scripts_SCRIPTS = \
	actioncfgmenu.pl \
	stylecfgmenu.pl \
	file2textop.pl

scripts = $(scripts_SCRIPTS)
CLEANFILES = $(scripts)
EXTRA_DIST = \
	actioncfgmenu.pl.in \
	stylecfgmenu.pl.in \
	file2textop.pl.in
all-local: $(scripts)

$(scripts): %.pl: %.pl.in
	@regex_cmd@ -e "s,@pkgversion@,@VERSION@," -e "s,@pathperl@,@perl_cmd@," -e "s,@pkgdatadir@,$(pkgdatadir)," $< > $@
