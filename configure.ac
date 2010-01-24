dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT(gpdftext, 0.1.1, http://sourceforge.net/projects/gpdftext/)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

GNOME_COMMON_INIT
GNOME_DOC_INIT

AM_INIT_AUTOMAKE([dist-bzip2])
# Checks for programs.
AC_PROG_CXX
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AM_PROG_CC_STDC
AC_PROG_LN_S
AC_PROG_MAKE_SET
IT_PROG_INTLTOOL([0.40.0])
AC_HEADER_STDC

# Checks for header files.
AC_CHECK_HEADERS([libintl.h stddef.h stdlib.h string.h unistd.h])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE

AC_DEFINE_UNQUOTED(DATADIR, "${prefix}/share", "data dir")

GLADEFILE="${prefix}/share/gpdftext/glade/gpdftext.glade"
AC_DEFINE_UNQUOTED(PACKAGE_GLADE_FILE, "$GLADEFILE", "location of Glade file")

AM_PROG_LIBTOOL

POPPLER_REQUIRED=0.8.7
GCONF_REQUIRED=2.0.0
GLIB_REQUIRED=2.16.6
# need 2.16 for GtkBuilder
GTK_REQUIRED=2.16.5
CAIRO_REQUIRED=1.6.4
PANGO_REQUIRED=1.20.5

AC_SUBST(POPPLER_REQUIRED)
AC_SUBST(GCONF_REQUIRED)
AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GTK_REQUIRED)
AC_SUBST(CAIRO_REQUIRED)
AC_SUBST(PANGO_REQUIRED)

MODULES="
	glib-2.0 >= $GLIB_REQUIRED
	gmodule-2.0 >= $GLIB_REQUIRED
	gtk+-2.0 >= $GTK_REQUIRED
	gconf-2.0 >= $GCONF_REQUIRED
	poppler-glib >= $POPPLER_REQUIRED
	gdk-pixbuf-2.0 >= $GTK_REQUIRED
	cairo-pdf >= $CAIRO_REQUIRED
	pango >= $PANGO_REQUIRED
"

# Make sure we have pkg-config installed
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

# Append GtkSpell to the pkg-config module list if desired
HAVE_GTKSPELL=no
AC_ARG_WITH(gtkspell, AS_HELP_STRING([--with-gtkspell],
		      [Compile with GtkSpell support (default=autodetect)]), 
		      [with_gtkspell=yes], [with_gtkspell=yes])
if test "$with_gtkspell" = "yes"; then
  AC_MSG_CHECKING(for gtkspell-2.0)
  if $PKG_CONFIG --exists "gtkspell-2.0" ; then
	AC_MSG_RESULT(yes)
	HAVE_GTKSPELL=yes
	AC_DEFINE(HAVE_GTKSPELL,1,[do we have gtkspell?])
	MODULES="$MODULES	gtkspell-2.0
"
  else
	AC_MSG_RESULT(not found)
  fi
fi

PKG_CHECK_MODULES(GPDFTEXT, $MODULES) 
AC_SUBST(GPDFTEXT_CFLAGS)
AC_SUBST(GPDFTEXT_LIBS)

dnl #
dnl # GConf related settings
dnl #
AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi
AM_GCONF_SOURCE_2

#
# Check for Scrollkeeper
#
AC_PATH_PROG(SCROLLKEEPER_CONFIG, scrollkeeper-config, no)
if test x$SCROLLKEEPER_CONFIG = xno; then
  AC_MSG_ERROR(Could not find scrollkeeper-config, please install the scrollkeeper package)
fi

dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
GETTEXT_PACKAGE=gpdftext
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", "definition of GETTEXT_PACKAGE")
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR([GNOMELOCALEDIR])

##################################################
# Check for gtk-doc.
##################################################
AC_ARG_WITH(html-dir, [  --with-html-dir=PATH path to installed docs ])
if test "x$with_html_dir" = "x" ; then
  HTML_DIR='${datadir}/gtk-doc/html'
else
  HTML_DIR=$with_html_dir
fi
AC_SUBST(HTML_DIR)

gtk_doc_min_version=1.0
AC_MSG_CHECKING([gtk-doc version >= $gtk_doc_min_version])
if pkg-config --atleast-version=$gtk_doc_min_version gtk-doc; then
  AC_MSG_RESULT(yes)
  GTKDOC=true
else
  AC_MSG_RESULT(no)
  GTKDOC=false
fi
dnl Let people disable the gtk-doc stuff.
AC_ARG_ENABLE(gtk-doc,
              [  --enable-gtk-doc  Use gtk-doc to build documentation [default=auto]],
	      enable_gtk_doc="$enableval", enable_gtk_doc=auto)
if test x$enable_gtk_doc = xauto ; then
  if test x$GTKDOC = xtrue ; then
    enable_gtk_doc=yes
  else
    enable_gtk_doc=no
  fi
fi
AM_CONDITIONAL(ENABLE_GTK_DOC, test x$enable_gtk_doc = xyes)

#
# allow the user to disable the desktop database update
#
AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database, no)
HAVE_NEW_DESKTOP=no
AC_ARG_ENABLE(desktop-update,
	AS_HELP_STRING([--disable-desktop-update],
	[Disable the desktop database update]),
	[disable_desktop=yes],
	[disable_desktop=no])
if test "x$disable_desktop" = "xno"; then
	if test "x$UPDATE_DESKTOP_DATABASE" != "xno"; then
		HAVE_NEW_DESKTOP=yes
		AC_DEFINE(HAVE_NEW_DESKTOP,1,[update desktop database?])
	fi
fi
AM_CONDITIONAL(HAVE_NEW_DESKTOP, test x$HAVE_NEW_DESKTOP = xyes)

dnl # ***********************************************
dnl # Extra warnings - even more than GNOME (maximum)
dnl # ***********************************************
AC_MSG_CHECKING(what extra warning flags to pass to the C compiler)
if test ${GCC}x = yesx; then
  WARN_CFLAGS=
  CFLAGS="${CFLAGS} -g2 -Wall"
  AC_ARG_ENABLE(error-on-warning,
        [  --disable-error-on-warning
                          disable treating compile warnings as errors],
        [case "${enableval}" in
        yes) WARN_CFLAGS="${WARN_CFLAGS} -Werror" ;;
        no)  ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-error-on-warning) ;;
        esac],
        [  WARN_CFLAGS="${WARN_CFLAGS} -Werror" ])
  GCC_VERSION=`${CC} -dumpversion`
  if test `echo ${GCC_VERSION} | cut -d. -f1` -ge 3; then
     # This is gcc >= 3.x.x
     if test `echo ${GCC_VERSION} | cut -d. -f2` -ge 4; then
        # This is gcc >= 3.4.x
        WARN_CFLAGS="${WARN_CFLAGS} -Wdeclaration-after-statement"
     else if test `echo ${GCC_VERSION} | cut -d. -f1` -ge 4; then
        # This is gcc == 4.x.x
        WARN_CFLAGS="${WARN_CFLAGS} -Wdeclaration-after-statement -Wno-pointer-sign"
        fi
     fi
  fi
dnl  CFLAGS="${CFLAGS} ${WARN_CFLAGS}"
else
  WARN_CFLAGS=
fi
AC_MSG_RESULT($WARN_CFLAGS)

set_compile_warnings=no

AC_ARG_ENABLE(compile-warnings,
              [  --enable-compile-warnings
                          Enable verbose compiler warnings],
              set_compile_warnings=yes)

if test "$GCC" = "yes" -a "$set_compile_warnings" != "no"; then
        WARN_CFLAGS="$WARN_CFLAGS -Wcast-align -Wsign-compare \
 -Wmissing-prototypes -Wmissing-declarations -Werror -Wmissing-include-dirs \
 -Winline -Wnested-externs -Wpointer-arith \
 -Wmissing-field-initializers"
	LDFLAGS="$LDFLAGS -Wl,--no-add-needed"
fi
AC_SUBST(WARN_CFLAGS)

#
# Pass user-specified flags on to our Makefiles
#
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)


AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
help/Makefile
po/Makefile.in
])
