AC_INIT([hk], [0.1])

CFLAGS="$CFLAGS -std=c99 -Wall -pedantic -Wextra"

AC_PATH_X
if test -n "$x_includes"; then
    CFLAGS="$CFLAGS -I $x_includes"
fi
if test -n "$x_libraries"; then
    LDFLAGS="$LDFLAGS -L $x_libraries"
fi



AC_CONFIG_FILES([Makefile])
AC_OUTPUT
