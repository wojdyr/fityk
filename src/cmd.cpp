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

bool extended_print;
string t, t2;

void set_data_title(char const*, char const*)  { my_data->title = t; }

void do_transform(char const* a, char const* b)  
                                         { my_data->transform(string(a,b)); }

void do_assign_var(char const* a, char const* b) 
               { assign_variable(string(t, 1), string(a,b)); }

void do_print(char const* a, char const* b)
{
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
        const Variable* v = find_variable(string(s, 1));
        m = v ? v->get_info(extended_print) : "Undefined variable: " + s;
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
        //these static constants for assign_a are workaround for assign_a
        //problems, as proposed by Joao Abecasis at Spirit-general ML
        //Message-ID: <435FB3DD.8030205@gmail.com>
        //Subject: [Spirit-general] Re: weird assign_a(x,y) problem
        static const bool true_ = true;
        static const bool false_ = false;

        transform_arg 
            = "title">>ch_p('=') >> lexeme_d['"' >> (+~ch_p('"'))[assign_a(t)] 
                                             >> '"']  [&set_data_title]
            | no_actions_d[DataTransformG][&do_transform] 
            ;

        assign_var 
            = VariableLhsG [assign_a(t)]
                      >> '=' >> no_actions_d[VariableRhsG] [&do_assign_var]
            ;

        print_arg
            = (str_p("variables") 
              | VariableLhsG
              )[&do_print]
            ;

        statement 
            = str_p("d.transform") >> (transform_arg % ',')
            | assign_var % ','
            | str_p("print") 
                >> (str_p("ext") [assign_a(extended_print, true_)] 
                   | eps_p[assign_a(extended_print, false_)] 
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

