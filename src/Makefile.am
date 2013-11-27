bin_PROGRAMS = WMmp

WMmp_SOURCES =   \
	wmmp.c \
	dockapp.c \
	dockapp.h \
	mouse_regions.c \
	mouse_regions.h \
	backlight_off.xpm \
	backlight_on.xpm \
	parts.xpm \
	mpd_func.c \
	mpd_func.h \
	libmpdclient.c \
	libmpdclient.h

CPPFLAGS = @CPPFLAGS@ @DFLAGS@ -Wall

INCLUDES = @HEADER_SEARCH_PATH@

WMmp_LDADD = @LIBRARY_SEARCH_PATH@ @XLIBS@

LDFLAGS = 

INSTALL_PROGRAM = \
	@INSTALL_PROGRAM@ \
	@SETGID_FLAGS@ \
	$(AM_INSTALL_PROGRAM_FLAGS)
