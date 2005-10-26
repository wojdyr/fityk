// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// $Id$

#include "cmd.h"
#include "ui.h"
#include "data.h"
#include "datatrans.h"
#include "var.h"
#include <boost/spirit/core.hpp>

using namespace std;
using namespace boost::spirit;

namespace {

int extended_print = 0;
string t, t2;

void set_data_title(char const*, char const*)  { my_data->title = t; }

void do_transform(char const* a, char const* b)  
                                         { my_data->transform(string(a,b)); }

void do_assign_var(char const* a, char const* b) 
                                   { assign_variable(t, string(a,b)); }

void do_print(char const* a, char const* b)
{
    //mesg("extended_print= " + S(extended_print));
    //TODO why it doesn't work ???
    string s = string(a,b);
    string m;
    if (s == "variables") {
        m = "Defined variables: ";
        for (vector<Variable*>::const_iterator i = variables.begin(); 
                i != variables.end(); ++i)
            if (extended_print)
                m += "\n" + (*i)->get_info(false);
            else
                m += "$" + (*i)->get_name() + " ";
    }
    else if (s[0] == '$') {
        int f = find_variable(string(s, 1));
        if (f == -1)
            m = "Undefined variable: " + s;
        else
            m = variables[f]->get_info(extended_print);
    }
    mesg(m);

}

} //namespace

struct CmdGrammar : public grammar<CmdGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(CmdGrammar const& /*self*/)
    {
        transform_arg 
            = "title">>ch_p('=') >> lexeme_d['"' >> (+~ch_p('"'))[assign_a(t)] 
                                             >> '"']  [&set_data_title]
            | no_actions_d[DataTransformG][&do_transform] 
            ;

        assign_var 
            = lexeme_d['$' >> (+(alnum_p | '_'))[assign_a(t)]
                      ] 
                      >> '=' >> no_actions_d[VariableRhsG] [&do_assign_var]
            ;

        print_arg
            = (str_p("variables") 
              | lexeme_d["$" >> +(alnum_p | '_')]
              )[&do_print]
            ;

        statement 
            = str_p("d.transform") >> (transform_arg % ',')
            | assign_var % ','
            | str_p("print") 
                >> (str_p("ext") [assign_a(extended_print, 1)] 
                   | eps_p[assign_a(extended_print, 0)] 
                   )
                >> (print_arg % ',')
            ;

        multi 
            = statement % ';';
    }

    rule<ScannerT> transform_arg, assign_var, print_arg, 
                   statement, multi;  

    rule<ScannerT> const& start() const { return multi; }
  };
} cmdG;


bool spirit_parser(string const& str)
{
    parse_info<> result = parse(str.c_str(), cmdG, space_p);
    if (result.full) {
    }
    return (bool) result.full;
}

