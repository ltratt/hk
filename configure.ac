AC_INIT([hk], [0.1])

AC_LANG(C)

AC_PROG_CC
AC_ISC_POSIX
AC_PROG_GCC_TRADITIONAL
AC_GNU_SOURCE

CFLAGS="$CFLAGS -std=c99 -Wall -pedantic -Wextra"
LDFLAGS="-Wl,-z,relro,-z,now"

case `uname -s` in
  Linux*|GNU* )
   CFLAGS="$CFLAGS -D_POSIX_C_SOURCE=199309L"
   ;;
esac

AC_PATH_X
AC_PATH_XTRA
if test -n "$X_CFLAGS"; then
    CFLAGS="$CFLAGS $X_CFLAGS"
fi
if test -n "$X_LIBS"; then
    LDFLAGS="$LDFLAGS $X_LIBS"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
