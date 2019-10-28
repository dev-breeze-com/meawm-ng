WAIMEA_PATH=$(pkgdatadir)

INCLUDES = \
-DWAIMEA_PATH=\"$(WAIMEA_PATH)\" \
$(CAIRO_CFLAGS) \
$(ICONV_CFLAGS) \
$(EXPAT_CFLAGS) \
$(THREAD_CFLAGS)\
$(X_CFLAGS) \
$(SHAPE_CFLAGS) \
$(XINERAMA_CFLAGS) \
$(RENDER_CFLAGS) \
$(RANDR_CFLAGS) \
$(LIBPNG_CFLAGS) \
$(LIBSVG_CAIRO_CFLAGS) \
$(XCURSOR_CFLAGS) \
$(DEBUG_CFLAGS)

meawm_ng_LDADD = \
$(CAIRO_LIBS) \
$(LOCALE_LIBS) \
$(ICONV_LIBS) \
$(EXPAT_LIBS) \
$(THREAD_LIBS) \
$(X_LIBS) \
$(XEXT_LIBS) \
$(XINERAMA_LIBS) \
$(RENDER_LIBS) \
$(RANDR_LIBS) \
$(LIBPNG_LIBS) \
$(LIBSVG_CAIRO_LIBS) \
$(XCURSOR_LIBS)

bin_PROGRAMS = meawm_ng

meawm_ng_SOURCES = \
	main.cc \
	Meawm_NG.cc \
	Meawm_NG.hh \
	Screen.cc \
	Screen.hh \
	Window.cc \
	Window.hh \
	Resources.cc \
	Resources.hh \
	Event.cc \
	Event.hh \
	Menu.cc \
	Menu.hh \
	Net.cc \
	Net.hh \
	Dockapp.cc \
	Dockapp.hh \
	Timer.cc \
	Timer.hh \
	Regex.cc \
	Regex.hh \
	Parser.cc \
	Parser.hh \
	Style.cc \
	Style.hh \
	Action.cc \
	Action.hh \
	Cursor.cc \
	Cursor.hh \
	Render.cc \
	Render.hh \
	Png.cc \
	Png.hh \
	Util.cc \
	Util.hh \
	RefCounted.hh \
	Tst.hh
