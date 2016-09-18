dnl $Id$
dnl config.m4 for extension beehive

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(beehive, for beehive support,
dnl Make sure that the comment is aligned:
dnl [  --with-beehive             Include beehive support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(beehive, whether to enable beehive support,
dnl Make sure that the comment is aligned:
dnl [  --enable-beehive           Enable beehive support])

if test "$PHP_BEEHIVE" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-beehive -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/beehive.h"  # you most likely want to change this
  dnl if test -r $PHP_BEEHIVE/$SEARCH_FOR; then # path given as parameter
  dnl   BEEHIVE_DIR=$PHP_BEEHIVE
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for beehive files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       BEEHIVE_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$BEEHIVE_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the beehive distribution])
  dnl fi

  dnl # --with-beehive -> add include path
  dnl PHP_ADD_INCLUDE($BEEHIVE_DIR/include)

  dnl # --with-beehive -> check for lib and symbol presence
  dnl LIBNAME=beehive # you may want to change this
  dnl LIBSYMBOL=beehive # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $BEEHIVE_DIR/$PHP_LIBDIR, BEEHIVE_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_BEEHIVELIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong beehive lib version or lib not found])
  dnl ],[
  dnl   -L$BEEHIVE_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(BEEHIVE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(beehive, beehive.c, $ext_shared)
fi
