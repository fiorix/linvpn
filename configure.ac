dnl init
AC_INIT(lib/linvpn.h)
AM_INIT_AUTOMAKE(linvpn, 3.0)
AM_CONFIG_HEADER(autoconf.h)

dnl check hostname
AC_CANONICAL_HOST

dnl check programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PATH_PROG(AR, ar, none)
AC_PATH_PROG(RANLIB, ranlib, none)
AC_PATH_PROG(PPPD, pppd, none)
if test "$ac_cv_path_PPPD" != "/usr/sbin/pppd"; then
    echo "pppd is required to run linvpn"
    echo "please install /usr/sbin/pppd and re-run configure"
    exit 1
fi

dnl set CFLAGS and LDFLAGS
CFLAGS="-Wall -O2 $CFLAGS"
LDFLAGS="$LDFLAGS"

dnl check OS
case "$host" in
    *openbsd*)
        LDFLAGS="$LDFLAGS -lintl -liconv -lgpg-error"
    ;;
esac

dnl check headers
AC_STDC_HEADERS
AC_CHECK_HEADERS(gcrypt.h)
if test "$ac_cv_header_gcrypt_h" = "no"; then
    echo "libgcrypt is required to compile linvpn"
    echo "please install libgcrypt and re-run configure"
    exit 1
fi

dnl check libgcrypt and libutil
AC_CHECK_LIB(gcrypt, main)
if test "$ac_cv_lib_gcrypt_main" = "no"; then
    echo "libgcrypt is required to compile linvpn"
    echo "please install libgcrypt and re-run configure"
    exit 1
fi
AC_CHECK_LIB(util, main)
if test "$ac_cv_lib_util_main" = "no"; then
    echo "libutil is required to compile linvpn"
    echo "please install libutil and re-run configure"
    exit 1
fi

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_OUTPUT([Makefile doc/Makefile lib/Makefile client/Makefile server/Makefile])
