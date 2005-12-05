/* This file is part of fityk program. Copyright (C) Marcin Wojdyr */

%{
// $Id$
#include <string>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include "common.h"
struct Pre_string //used in parser.y and parser.l
{ 
char *c; int l; 
std::string str() { return std::string(c, l); }
};

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
[fsmoc]\.s(et)?   r_cmd(); BEGIN(SET_COND); iplval.c = iptext[0]; return SET;
f\.r(un)?	   r_cmd(); return F_RUN;
f\.c(ontinue)?	   r_cmd(); return F_CONTINUE;
f\.m(ethod)?	   r_cmd(); return F_METHOD;
m\.f(indpeak)?     r_cmd(); return M_FINDPEAK;
o\.i(nclude)?	   r_cmd(); BEGIN(FILE_COND); return O_INCLUDE; 
o\.d(ump)?	   r_cmd(); BEGIN(FILE_COND); return O_DUMP;
(q(uit)?)|(exit)   r_cmd(); return QUIT;

  
{UINT}     iplval.i = atoi (iptext); return UINt; 
{INT}      iplval.i = atoi (iptext); return INt; 
{FLOAT}    iplval.f = atof (iptext); return FLOAt; 
({UINT}"-")|({UINT}{ws_}"-"{ws_})   iplval.i = atoi (iptext); return UI_DASH; 
<D_UI_SLASH>{UINT}{ws}"/"  iplval.i = atoi(iptext); return UI_SLASH;
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
'[^']+'    { /* 'filename' */
           strncpy (str_tb, iptext + 1, 256);
           iplval.s.c = str_tb; 
	   iplval.s.l = ipleng - 2;
	   return FILENAME;
	   }
<FILE_COND>[[:alpha:]./\\_][[:alnum:]./\\_-]{2,255} { /* filename */
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

