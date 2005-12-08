/* This file is part of fityk program. Copyright (C) Marcin Wojdyr */

/* file for bison. */

%{
// $Id$
#include <string>
#include <stdlib.h>
#include <vector>
#include <utility>
#include "common.h"
#include "fit.h"
#include "sum.h"
#include "data.h"
#include "ui.h"
#include "manipul.h"
#include "logic.h"
#include "cmd.h"

using namespace std;
int iperror (char *s);
int iplex ();
struct yy_buffer_state;
yy_buffer_state *yy_scan_string(const char *str);
void ip_delete_buffer (yy_buffer_state *b);
vector<int> ivec, ivec2;
vector<pair<int,int> > vlines;
vector <double> fvec;

bool new_line = false;

struct Pre_string //used in parser.y and parser.l
{ 
char *c; int l; 
std::string str() { return std::string(c, l); }
};

void replot()
{
    getUI()->drawPlot(2);
}

#define YYERROR_VERBOSE 1

%}


%union 
{
    bool bol;
    int i;
    char c;
    double f;
    Pre_string s;
    struct { double l, r; } range;
}

%token <c> SET
%token F_RUN F_CONTINUE F_METHOD 
%token O_INCLUDE O_DUMP
%token M_FINDPEAK
%token QUIT 
%token PLUS_MINUS TWO_COLONS
%token SEP
%token <s> FILENAME DASH_STRING EQ_STRING DT_STRING
%token <c> LOWERCASE 
%token <i> UINt UI_DASH UI_SLASH INt 
%token <f> FLOAt 
%token <s> LEX_ERROR
%type <i> inr opt_uint opt_uint_1 
%type <f> flt opt_flt
%type <c> opt_lcase opt_proc
%type <range> range sim_range bracket_range
%type <bol> opt_asterix opt_plus

%%

input:	 /*empty*/
    | input SEP { replot(); }
    | input exp { replot(); }
    | input LEX_ERROR error SEP {
    	warn ("Syntax error at the beginning of command."); 
	yyerrok; yyclearin; }
    | input error LEX_ERROR error SEP { 
    	warn ("Syntax error near unknown token: `" + $3.str() + "'"); 
	yyerrok; yyclearin; }
    | input error SEP { warn("Parse error."); 
      yyerrok; yyclearin;}
    ;

exp:  SET DASH_STRING EQ_STRING SEP { 
	set_class_p($1)->setp ($2.str(), $3.str());
      }
    | SET DASH_STRING SEP          { set_class_p($1)->getp ($2.str()); }
    | SET SEP                      { mesg (set_class_p($1)->print_usage($1)); }
    | F_METHOD SEP     { mesg (fitMethodsContainer->print_current_method ()); }
    | F_METHOD LOWERCASE SEP       { fitMethodsContainer->change_method ($2); }
    ;


opt_proc: /*empty*/         { $$ = '='; }  
    |     '%'               { $$ = '%'; }  
    ;

sim_range:  ':'     { $$.l = -INF; $$.r = +INF; }
    | flt ':'      { $$.l = $1; $$.r = +INF; }
    | ':' flt      { $$.l = -INF; $$.r = $2; }
    ;

bracket_range: '[' ']'      { $$.l = -INF; $$.r = +INF; }
    | '[' sim_range ']'     { $$ = $2 }
    | flt ':' flt         { $$.l = $1; $$.r = $3; }
    | '[' flt ':' flt ']' { $$.l = $2; $$.r = $4; }
    ;

range: sim_range      { $$ = $1; }
     | bracket_range  { $$ = $1; }
     ;



inr: INt  
    | UINt { $$ = $1; }
    ;

opt_uint: /*empty*/   { $$ = 0; }
    |  UINt           { $$ = $1; }
    ;

opt_uint_1: /*empty*/ { $$ = -1; }
    |  UINt           { $$ = $1; }
    ;

uints: UINt          { ivec = vector1($1); }
    | uints UINt     { ivec.push_back($2); }
    ;

flt: FLOAt  
    | inr { $$ = (double) $1; }
    ;

opt_flt: /*empty*/      { $$ = 0; }
    |          flt      { $$ = $1 }
    ;

opt_lcase: /*empty*/     { $$ = 0; }
    |      LOWERCASE     { $$ = $1; }
    ;

opt_asterix: /*empty*/   { $$ = false; }
    |       '*'          { $$ = true;  }
    ;
opt_plus: /*empty*/      { $$ = false; }
    |       '+'          { $$ = true;  }
    ;


columns: /*empty*/      { ivec.clear(); }
   |  UINt ':' UINt   { ivec = vector2 ($1, $3); }
   |  UINt ':' UINt ':' UINt  { ivec = vector3 ($1, $3, $5); }
   ;

lines: /*empty*/          { vlines.clear(); }
   |  lines UI_DASH UINt  { vlines.push_back(make_pair($2,$3)); }
   |  lines UINt          { vlines.push_back(make_pair($2,$2)); }
   ;

uint_slashes_: UI_SLASH               { ivec = vector1 ($1); }
   |           uint_slashes UI_SLASH  { ivec.push_back ($2); }
   ;

uint_slashes: uint_slashes_           { /*nothing*/;         }
   |          uint_slashes_ UINt     { ivec.push_back ($2); }
   |          '/'                     { ivec.clear();        }
   ;

%%

int iperror (char * /*s*/) { return 0; }

void start_of_string_parsing(const char *s);
void end_of_string_parsing();

bool bison_parser (const std::string &cmd)
{
    start_of_string_parsing ((" " + cmd + "\n").c_str());
    int result = ipparse();
    end_of_string_parsing();
    return result == 0 ? true : false;
}

void cmd_parser (std::string cmd)
{
    try {
        bool r = spirit_parser(cmd);
        if (!r)
            bison_parser(cmd);
    } catch (ExecuteError &e) {
        warn(string("Error: ") + e.what());
    }
}

