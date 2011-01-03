
AC_DEFUN([READLINE_STUFF], 
[
 dnl GNU readline and the required terminal library check
 AC_SUBST(READLINE_LIBS)
 AC_SUBST(NO_READLINE)
 if test "x$with_readline" != "xno" ; then
     dnl check for terminal library 
     dnl if libreadline is already linked with a terminal library, 
     dnl we don't need to link it the second time 
     gp_tcap=""
     for termlib in readline ncurses curses termcap terminfo termlib; do
         AC_CHECK_LIB(${termlib}, tputs, [gp_tcap="-l${termlib}"])
	 if test "x${gp_tcap}" != x; then
	     if test "${termlib}" = "readline"; then
	         gp_tcap=""
		 AC_MSG_RESULT([readline is linked with a terminal library])
	     else
		 AC_MSG_RESULT([using ${gp_tcap} with readline])
	     fi
             break
         fi
     done
 
     AC_CHECK_LIB([readline], [readline], 
	              [READLINE_LIBS="-lreadline $gp_tcap"],
                      [AC_MSG_ERROR([
      Can't find -lreadline in a standard path. 
      Install GNU readline library or, if it is already installed 
      in non-standard location, use CPPFLAGS and LDFLAGS. 
      You can also configure fityk with option --without-readline])], 
 		      [${gp_tcap}]) dnl readline
     AC_CHECK_HEADER([readline/readline.h], [], [AC_MSG_ERROR([
      You don't have headers of the readline library. 
      Perhaps you have installed run-time part of the readline library 
      from RPM or another binary package and have not installed development 
      package, which usually have appendix -dev or -devel.
      Either install it, or configure fityk with option --without-readline])])
     dnl readline < 4.2 doesn't have rl_completion_matches() function
     dnl some libreadline-compatibile libraries (like libedit) also
     dnl don't have it. We don't support them.
     AC_CHECK_DECLS([rl_completion_matches], [], [AC_MSG_ERROR([ 
     Although you seem to have a readline-compatible library, it is either 
     a very old GNU readline <= 4.1, or readline-compatible library, 
     like libedit, but it's not compatible enough with readline >= 4.2.
         Either install libreadline >= 4.2, 
         or configure fityk with option --without-readline])],
     [
      #include <stdio.h> 
      #include <readline/readline.h>
     ]) 
 else
     AC_DEFINE(NO_READLINE, 1,
        [Define if you do not want to use or do not have readline library.])
 fi
])
