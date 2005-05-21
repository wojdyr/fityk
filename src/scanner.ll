/* This file is part of fityk program. Copyright (C) Marcin Wojdyr */

%{
// $Id$
#include <string>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include "common.h"
#include "pag.h"
#include "parser.tab.hh"
char str_tb[1024+1];
 // table for saving strings (yytext(iptext) can be changed - %array)
 // it has to be ensured that one string is not overwritten
 // by another one before parsing expression with this string
std::string last_cmd;
void r_cmd ()
{
  last_cmd = std::string (iptext, 0, ipleng);
}
  
void start_of_string_parsing(const char *s)
{
    yy_scan_string (s);
}

void end_of_string_parsing()
{
    ip_delete_buffer(YY_CURRENT_BUFFER);
}

extern bool new_line;

%}

%option never-interactive
%option noyywrap
%option prefix="ip"
%array
%x SET_COND
%s D_UI_SLASH
%s FILE_COND
%s D_LOAD_COND

UINT [[:digit:]]+
INT [+|-]?[[:digit:]]+
FLOAT {INT}("."[[:digit:]]*)?([e|E]{INT})?
ws [ \t\v\f\r]* 
ws_ [ \t\v\f\r]+ 

%%

<*>{ws_} 	   /* ignore */
<*>(#[^\n]*)?[\n]+  BEGIN(0); last_cmd=""; new_line = true; return SEP;   
  /*<*>#[^\n]*[\n]   BEGIN(0); last_cmd.clear(); new_line = true; return SEP; */
								/*comments*/
<*>[;]+          BEGIN(0); last_cmd=""; return SEP;
<*>","           { unput (' '); /*FIXME how to do it without unput() ? */
                   for (int i = last_cmd.size() - 1; i >= 0; i--) {
                       unput(last_cmd[i]);
		   }
                   return SEP;
                 }
[dfsmoc]\.s(et)?   r_cmd(); BEGIN(SET_COND); iplval.c = iptext[0]; return SET;
d\.l(oad)?	   r_cmd(); BEGIN(D_LOAD_COND); return D_LOAD;
d\.a(ctivate)?     r_cmd(); return D_ACTIVATE;
d\.r(ange)?	   r_cmd(); return D_RANGE;
d\.d(eviation)?    r_cmd(); return D_DEVIATION;
d\.i(nfo)?	   r_cmd(); return D_INFO;
d\.e(xport)?       r_cmd(); BEGIN(FILE_COND); return D_EXPORT;
f\.r(un)?	   r_cmd(); return F_RUN;
f\.c(ontinue)?	   r_cmd(); return F_CONTINUE;
f\.m(ethod)?	   r_cmd(); return F_METHOD;
f\.i(nfo)?	   r_cmd(); return F_INFO;
s\.a(dd)?	   r_cmd(); return S_ADD; 
s\.h(istory)?      r_cmd(); BEGIN(D_UI_SLASH); return S_HISTORY;
s\.i(nfo)?	   r_cmd(); return S_INFO;
s\.g(uess)?        r_cmd(); return S_GUESS;
s\.r(emove)?	   r_cmd(); return S_REMOVE;
s\.c(hange)?	   r_cmd(); return S_CHANGE; 
s\.f(reeze)?	   r_cmd(); return S_FREEZE;
s\.v(alue)?        r_cmd(); return S_VALUE;
s\.e(xport)?       r_cmd(); BEGIN(FILE_COND); return S_EXPORT;
m\.f(indpeak)?     r_cmd(); return M_FINDPEAK;
o\.p(lot)?	   r_cmd(); return O_PLOT; 
o\.l(og)?	   r_cmd(); BEGIN(FILE_COND); return O_LOG; 
o\.i(nclude)?	   r_cmd(); BEGIN(FILE_COND); return O_INCLUDE; 
o\.w(ait)?	   r_cmd(); return O_WAIT;
o\.d(ump)?	   r_cmd(); BEGIN(FILE_COND); return O_DUMP;
(q(uit)?)|(exit)   r_cmd(); return QUIT;

c\.w(avelength)?   r_cmd(); return C_WAVELENGTH; 
c\.a(dd)?	   r_cmd(); return C_ADD; 
c\.i(nfo)?	   r_cmd(); return C_INFO; 
c\.r(emove)?	   r_cmd(); return C_REMOVE; 
c\.e(stimate)?     r_cmd(); return C_FIND;

  
{UINT}     iplval.i = atoi (iptext); return UINt; 
{INT}      iplval.i = atoi (iptext); return INt; 
{FLOAT}    iplval.f = atof (iptext); return FLOAt; 
({UINT}"-")|({UINT}{ws_}"-"{ws_})   iplval.i = atoi (iptext); return UI_DASH; 
<D_UI_SLASH,D_LOAD_COND>{UINT}{ws}"/"  iplval.i = atoi(iptext); return UI_SLASH;
":"        return ':'; 
"/"        return '/'; 
"!"        return '!'; 
"%"        return '%'; 
"["        return '['; 
"]"        return ']'; 
"*"        return '*'; 
"."        return '.'; 
"@"        return '@'; 
"$"        return '$'; 
"^"        return '^'; 
"<"        return '<'; 
"+-"       return PLUS_MINUS; 
"}"        return '}'; 
"+"        return '+'; 
"-"        return '-'; 
"("        return '(';
")"        return ')';
"::"       return TWO_COLONS;
{ws_}[[:lower:]]/({ws_}|\n|\0)  { int i=0; while (isspace (iptext[i])) ++i; 
                             iplval.c = iptext[i]; return LOWERCASE; 
		           }
"_"{FLOAT} iplval.f = atof (iptext + 1); return P_NUM; 
"~"{FLOAT} iplval.f = atof (iptext + 1); return NEW_A; 
"@"{UINT}  iplval.i = atoi (iptext + 1); return A_NUM;
"@*"       iplval.i = -1; return A_NUM; 
"$"{UINT}  iplval.i = atoi (iptext + 1); return G_NUM; 
"$*"       iplval.i = -1; return G_NUM; 
"^"{UINT}  iplval.i = atoi (iptext + 1); return F_NUM; 
"^*"       iplval.i = -1; return F_NUM; 
"<"{UINT}  iplval.i = atoi (iptext + 1); return Z_NUM; 
"<*"       iplval.i = -1; return Z_NUM; 
"%"{UINT}  iplval.i = atoi (iptext + 1); return PH_NUM; 
"%*"       iplval.i = -1; return PH_NUM; 
"$"[[:alpha:]]  iplval.c = *(iptext + 1); return G_TYPE; 
"^"[[:alpha:]]  iplval.c = *(iptext + 1); return F_TYPE; 
"<"[[:alpha:]]  iplval.c = *(iptext + 1); return Z_TYPE; 
"%"[[:alpha:]]  iplval.c = *(iptext + 1); return PH_TYPE; 
'[^']+'    { /* 'filename' */
           strncpy (str_tb, iptext + 1, 256);
           iplval.s.c = str_tb; 
	   iplval.s.l = ipleng - 2;
	   return FILENAME;
	   }
<FILE_COND,D_LOAD_COND>[[:alpha:]./\\_][[:alnum:]./\\_-]{2,255} { /* filename */
           strncpy (str_tb, iptext, 256);
           iplval.s.c = str_tb; 
	   iplval.s.l = ipleng;
	   return FILENAME;
	   }
<SET_COND>[[:alpha:]_-]+  {
		strncpy (str_tb, iptext, 256);
	        iplval.s.c = str_tb; 
		iplval.s.l = ipleng;
		return DASH_STRING;
		}
<SET_COND>={ws}[[:alnum:].\"+-]+  {
	        strncpy (str_tb + 512, iptext + 1, 256);
	        iplval.s.c = str_tb + 512; 
		iplval.s.l = ipleng - 1;
		while (isspace(*iplval.s.c)) {
		    iplval.s.c++;
		    iplval.s.l--;
		}
		return EQ_STRING;
	       }


<*>([[:alnum:]]+|.)   { /* last rule - error */
	        strncpy (str_tb, iptext, 256);
	        iplval.s.c = str_tb; 
		iplval.s.l = ipleng;
		return LEX_ERROR;
		}

