AC_INIT([hk], [0.3.1])

AC_LANG(C)

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AC_SEARCH_LIBS([strerror], [cposix])
AC_PROG_GCC_TRADITIONAL

CFLAGS="$CFLAGS -std=c99 -Wall -pedantic -Wextra"
LDFLAGS="$LDFLAGS -Wl,-z,relro,-z,now"

case `uname -s` in
  Linux*|GNU* )
   CFLAGS="$CFLAGS -D_POSIX_C_SOURCE=199309L"
   ;;
esac

AC_PATH_XTRA
AS_IF([test "X$no_x" = "Xyes"], [AC_MSG_ERROR([Could not find X11])])
if test -n "$X_CFLAGS"; then
    CFLAGS="$CFLAGS $X_CFLAGS"
fi
if test -n "$X_LIBS"; then
    LDFLAGS="$LDFLAGS $X_LIBS"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
