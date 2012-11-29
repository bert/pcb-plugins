dnl Process this file with autoconf to produce a configure script.

AC_INIT(configure.ac)
AM_INIT_AUTOMAKE(pcb-plugins, 0.0.2)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

# Try to figure out if we are building from git sources.
# If we are then unless building of the documentation has been disabled
# then just require the user have all the right tools.
# Users building from a tarball won't need latex, makeinfo,
# etc. but if you're already downloading development sources, then
# it is not unreasonable to require development tools.
# What motivated this is that using maintainer mode proved to cause
# regular confusion.

pcb_plugins_sources="tarball"
AC_MSG_CHECKING([if you are building from a git checkout])
pcb_plugins_git_version=no
if test -f $srcdir/.gitignore ; then
	pcb_plugins_git_version=yes
	AC_MSG_RESULT([yes])
	pcb_plugins_sources="GIT"
else
	AC_MSG_RESULT([no])
fi
AM_CONDITIONAL(GIT_VERSION, test x$pcb_plugins_git_version = xyes)

AC_ISC_POSIX

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PROG_CPP

# AC_PROG_MKDIR_P
# is a backport of autoconf-2.60's AC_PROG_MKDIR_P.
# Remove this macro when we can assume autoconf >= 2.60.
m4_ifdef([AC_PROG_MKDIR_P], [], [
  AC_DEFUN([AC_PROG_MKDIR_P],
    [AC_REQUIRE([AM_PROG_MKDIR_P])dnl defined by automake
     MKDIR_P='$(mkdir_p)'
     AC_SUBST([MKDIR_P])])])
AC_PROG_MKDIR_P

pkg_modules="gtk+-2.0 >= 2.0.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

GETTEXT_PACKAGE=pcb-plugins
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

dnl Add the languages which your application supports here.
ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT

# Create makefiles for (sub)directories.
AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT

# Print some results.
expandedXDGDATADIR=`eval "echo $XDGDATADIR"`
expandedKDEDATADIR=`eval "echo $KDEDATADIR"`
AC_MSG_RESULT([
-----------------------------------------------
Configuration summary for $PACKAGE $VERSION:
Source tree distribution:    $pcb_plugins_sources
Build Doxygen documentation: $docs_yesno
xdg data directory:          $expandedXDGDATADIR
KDE data directory:          $expandedKDEDATADIR
CC:                          $CC
CXX:                         $CXX
CPPFLAGS:                    $CPPFLAGS
CFLAGS:                      $CFLAGS
CXXFLAGS:                    $CXXFLAGS
LIBS:                        $LIBS
-----------------------------------------------
])

# EOF

# FIXME: add Makefile.in files to get the following
#src/plugins/Makefile

