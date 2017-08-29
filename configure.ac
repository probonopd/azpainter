
AC_PREREQ([2.69])
AC_INIT([azpainter], [2.1.0], [azelpg@gmail.com])
AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

AC_PROG_INSTALL
AC_PROG_RANLIB

# C compiler

AC_PROG_CC
AC_PROG_CC_STDC
AC_C_BIGENDIAN([AC_DEFINE([MLIB_BIGENDIAN], [1], [BigEndian])])

# with

AC_ARG_WITH([freetype-dir],
  AS_HELP_STRING([--with-freetype-dir], [set to freetype header directory]),
  [AC_SUBST([freetype_dir], [$withval])],
  [AC_SUBST([freetype_dir], [/usr/include/freetype2])] )

# need header

AC_CHECK_HEADER([pthread.h],
  [AC_DEFINE([HAVE_PTHREAD_H], [1], [pthread.h])],
  AC_MSG_ERROR(pthread.h not found))

AC_CHECK_FILE("$freetype_dir/ft2build.h",,AC_MSG_ERROR([(freetype) ft2build.h not found]))
AC_CHECK_HEADER([fontconfig/fontconfig.h],,AC_MSG_ERROR(fontconfig.h not found))
AC_CHECK_HEADER([zlib.h],,AC_MSG_ERROR(zlib.h not found))
AC_CHECK_HEADER([png.h],,AC_MSG_ERROR(png.h not found))
AC_CHECK_HEADER([jpeglib.h],,AC_MSG_ERROR(jpeglib.h not found))

# X11 ext

AC_CHECK_LIB([Xext], [XShmCreateImage], [AC_DEFINE([HAVE_XEXT_XSHM], [1], [XShm])])
AC_CHECK_LIB([Xi], [XIQueryDevice], [have_xi2="yes"])

if test "$have_xi2" == "yes";then
  AC_DEFINE([HAVE_XEXT_XINPUT2], [1], [XInput2])
  LIBS="$LIBS -lXi"
fi

#

AC_PATH_X

#

AC_CONFIG_FILES([Makefile mlib/Makefile src/Makefile data/Makefile data/texture/Makefile desktop/Makefile])
AC_OUTPUT
