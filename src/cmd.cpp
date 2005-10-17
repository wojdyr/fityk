// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// $Id$

#include "cmd.h"
#include "data.h"
#include "datatrans.h"
#include <boost/spirit/core.hpp>

using namespace std;
using namespace boost::spirit;

string t;

void set_data_title(char const*, char const*)  { my_data->title = t; }
void do_transform(char const* a, char const* b)  
                                         { my_data->transform(string(a,b)); }

struct CmdGrammar : public grammar<CmdGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(CmdGrammar const& /*self*/)
    {
        transform_arg 
            = "title" >> lexeme_d['"' >> (+~ch_p('"'))[assign_a(t)] >> '"']
                                                             [&set_data_title]
            | no_actions_d[DataTransformG][&do_transform] 
            ;

        statement 
            = str_p("d.transform") >> (transform_arg % ',')
            ;

        multi 
            = statement % ';';
    }

    rule<ScannerT> transform_arg, statement, multi;  

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

