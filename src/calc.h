// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef CALC__H__
#define CALC__H__

using namespace boost::spirit;


///  function grammar
struct FuncGrammar : public grammar<FuncGrammar>
{
    static const int real_constID = 1;
    static const int variableID = 2;
    static const int exptokenID = 3;
    static const int factorID = 4;
    static const int termID = 5;
    static const int expressionID = 6;

    template <typename ScannerT>
    struct definition
    {
        definition(FuncGrammar const& /*self*/)
        {
            //  Start grammar definition
            real_const  =  leaf_node_d[ real_p |  as_lower_d[str_p("pi")] ];

            variable    =  leaf_node_d[lexeme_d[+alpha_p]];

            exptoken    =  real_const
                        |  inner_node_d[ch_p('(') >> expression >> ')']
                        |  root_node_d[ as_lower_d[ str_p("sqrt") 
                                                  | "exp" | "log10" | "ln" 
                                                  | "sin" | "cos" | "tan" 
                                                  | "atan" | "asin" | "acos"
                                                  ] ]
                           >>  inner_node_d[ch_p('(') >> expression >> ')']
                        |  (root_node_d[ch_p('-')] >> exptoken)
                        |  variable;

            factor      =  exptoken >>
                           *(  (root_node_d[ch_p('^')] >> exptoken)
                            );

            term        =  factor >>
                           *(  (root_node_d[ch_p('*')] >> factor)
                             | (root_node_d[ch_p('/')] >> factor)
                           );

            expression  =  term >>
                           *(  (root_node_d[ch_p('+')] >> term)
                             | (root_node_d[ch_p('-')] >> term)
                           );
        }

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > expression;
        rule<ScannerT, parser_context<>, parser_tag<termID> >       term;
        rule<ScannerT, parser_context<>, parser_tag<factorID> >     factor;
        rule<ScannerT, parser_context<>, parser_tag<exptokenID> >   exptoken;
        rule<ScannerT, parser_context<>, parser_tag<variableID> >   variable;
        rule<ScannerT, parser_context<>, parser_tag<real_constID> > real_const;

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > const&
        start() const { return expression; }
    };
};

#endif

