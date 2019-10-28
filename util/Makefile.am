WAIMEA_PATH=$(pkgdatadir)

INCLUDES =\
-DWAIMEA_PATH=\"$(WAIMEA_PATH)\"\
$(X_CFLAGS)\
$(DEBUG_CFLAGS)

meawm_ngctl_LDADD =\
$(X_LIBS)

bin_PROGRAMS = meawm_ngctl

meawm_ngctl_SOURCES = \
	meawm_ngctl.c
