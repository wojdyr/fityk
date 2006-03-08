
AC_DEFUN([READLINE_STUFF], 
[
 dnl GNU readline and the required terminal library check
 AC_SUBST(READLINE_LIBS)
 AC_SUBST(NO_READLINE)
 if test "x$with_readline" != "xno" ; then
     dnl check for terminal library
     dnl this is a very cool solution from octave's configure.in
     gp_tcap=""
     for termlib in ncurses curses termcap terminfo termlib; do
         AC_CHECK_LIB(${termlib}, tputs, [gp_tcap="${gp_tcap} -l${termlib}"])
         case "${gp_tcap}" in
             *-l${termlib}*)
             AC_MSG_RESULT([using ${gp_tcap} with readline])
             break
             ;;
         esac
     done
 
     if test "x$with_readline" = "xyes"; then
         AC_CHECK_LIB([readline], [readline], 
	              [READLINE_LIBS="-lreadline $gp_tcap"],
                      [AC_MSG_ERROR([
      Can't find -lreadline in a standard path. 
      Install GNU readline library or, if it is installed, 
      specify its location using --with-readline=/path/to/libreadline.a, 
      or configure fityk with option --without-readline])], 
 		      [${gp_tcap}]) dnl readline
         AC_CHECK_HEADER([readline/readline.h], [], [AC_MSG_ERROR([
      You don't have headers of the readline library. 
      Perhaps you have installed run-time part of the readline library 
      from RPM or another binary package and have not installed development 
      package, which usually have appendix -dev or -devel.
      Either install it, or configure fityk with option --without-readline])])
     else
       if test ! -f $with_readline ; then
         if test ! -f ${with_readline}/lib/libreadline.a ; then
           if test ! -f ${with_readline}/libreadline.a ; then
             AC_MSG_ERROR([GNU readline library not found])
           else
             AC_MSG_RESULT([using ${with_readline}/libreadline.a])
             READLINE_LIBS="${with_readline}/libreadline.a $gp_tcap"
           fi
         else
           AC_MSG_RESULT([using ${with_readline}/lib/libreadline.a])
           READLINE_LIBS="${with_readline}/lib/libreadline.a $gp_tcap"
         fi
       else
         AC_MSG_RESULT([using ${with_readline}])
         READLINE_LIBS="$with_readline $gp_tcap"
       fi
     fi
 else
     AC_DEFINE(NO_READLINE, 1,
        [Define if you do not want to use or do not have readline library.])
 fi
]) 
