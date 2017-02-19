dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)

AC_INIT(WMmp, 0.10.0, nicolasherry@gmail.com)
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_SRCDIR(src/wmmp.c)
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

AC_ARG_WITH(default-host,[  --with-default-host=HOST  Use as host when MPD_HOST environment variable is not set], DEFAULT_HOST="$withval",DEFAULT_HOST="localhost")
AC_ARG_WITH(default-port,[  --with-default-port=PORT  Use as port when MPD_PORT environment variable is not set], DEFAULT_PORT="$withval",DEFAULT_PORT="6600")
                                                                                
DFLAGS="$DFLAGS -DDEFAULT_HOST=\\\"$DEFAULT_HOST\\\""
DFLAGS="$DFLAGS -DDEFAULT_PORT=\\\"$DEFAULT_PORT\\\""
                                                                                
dnl
dnl Specify paths to look for libraries and headers
dnl ===============================================
AC_ARG_WITH(libs-from,
       [  --with-libs-from        pass compiler flags to look for libraries],
       [lib_search_path="$withval $lib_search_path"])

AC_ARG_WITH(incs-from,
       [  --with-incs-from        pass compiler flags to look for header files],
       [inc_search_path="$withval $inc_search_path"])


dnl ==========================================
dnl             Network Stuff
dnl ==========================================

AC_CHECK_LIB(socket,socket,LIBS="$LIBS -lsocket",)
AC_CHECK_LIB(nsl,gethostbyname,LIBS="$LIBS -lnsl",)

dnl ===========================================
dnl             Stuff that uses X
dnl ===========================================

AC_PATH_XTRA

X_LIBRARY_PATH=$x_libraries

XCFLAGS="$X_CFLAGS"

XLFLAGS="$X_LIBS"

XLIBS="-lX11 $X_EXTRA_LIBS"

lib_search_path="$lib_search_path $XLFLAGS"
inc_search_path="$inc_search_path $XCFLAGS"

AC_SUBST(X_LIBRARY_PATH)

dnl Shape extension
dnl ===============
AC_CHECK_LIB(Xext, XShapeCombineMask, [XLIBS="$XLIBS -lXext"],
    [echo "The shape extension stuff could not be found in the X client libraries"
     exit 1],
    $X_LIBS $X_EXTRA_LIBS -lX11)

dnl XPM library
dnl ===========
AC_CHECK_LIB(Xpm, XpmCreatePixmapFromData, [XLIBS="$XLIBS -lXpm"],
    [echo "The libXpm library was not found, but is necessary to build this library"
     exit 1],
    $X_LIBS $X_EXTRA_LIBS -lX11)

AC_SUBST(XCFLAGS)
AC_SUBST(XLFLAGS)
AC_SUBST(XLIBS)

AC_SUBST(X_EXTRA_LIBS)

dnl ===============================================
dnl             End of stuff that uses X
dnl ===============================================

dnl =========
dnl Debugging
dnl =========
AC_ARG_ENABLE(debug,
             [  --enable-debug          turn on debugging [default=on]],,enable_debug=no)
if test "$enable_debug" = yes; then
  DFLAGS="-Wall -g -ansi -pedantic"
  AC_DEFINE(DEBUG, 1, [use debug code])
fi
AC_SUBST(DFLAGS)


dnl ============
dnl Check for OS
dnl ============

ignore_buffers=no
ignore_cached=no
ignore_wired=no

case ${host_os} in
linux*)
  OS=linux
  ignore_buffers=yes
  ignore_cached=yes
  ;;
freebsd*)
  OS=freebsd
  ignore_wired=yes
  ignore_cached=yes
dnl  LIBS="$LIBS -lkvm"
dnl  SETGID_FLAGS="-g wheel -m 0755 -o root"
  ;;
openbsd*)
  OS=openbsd
  ;;
solaris*)
  OS=solaris
  ;;
*)
  echo ""
  echo "Sorry, ${host_os} is not supported yet"
  echo ""
  exit 1
  ;;
esac

AC_SUBST(OS)
AC_SUBST(SETGID_FLAGS)
AC_SUBST(LIBS)

if test "$ignore_buffers" = yes; then
  AC_DEFINE(IGNORE_BUFFERS, 1, [use '--ignore-buffers' option])
fi
if test "$ignore_cached" = yes; then
  AC_DEFINE(IGNORE_CACHED, 1, [use '--ignore-cached' option])
fi
if test "$ignore_wired" = yes; then
  AC_DEFINE(IGNORE_WIRED, 1, [use '--ignore-wired' option])
fi


dnl =============================
dnl Checks for library functions.
dnl =============================

LIBRARY_SEARCH_PATH="$lib_search_path"
HEADER_SEARCH_PATH="$inc_search_path"

AC_SUBST(LIBRARY_SEARCH_PATH)
AC_SUBST(HEADER_SEARCH_PATH)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h memory.h stddef.h stdlib.h string.h strings.h sys/param.h sys/time.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS(select strtoul uname)

AC_CONFIG_FILES(Makefile \
		src/Makefile \
		doc/Makefile)
AC_OUTPUT
