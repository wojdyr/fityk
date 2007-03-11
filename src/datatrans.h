// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__DATATRANS__H__
#define FITYK__DATATRANS__H__


#include <boost/spirit/core.hpp>

#include "common.h"
#include "data.h" //Point

using namespace boost::spirit;

class Data;

std::vector<Point> transform_data(std::string const& str, 
                                  std::vector<Point> const& old_points);

bool validate_transformation(std::string const& str); 
bool validate_data_expression(std::string const& str);
bool is_data_dependent_expression(std::string const& s);
fp get_transform_expression_value(std::string const &s, Data const* data);
fp get_transform_expr_value(std::vector<int>& code_, 
                            std::vector<Point> const& points);
std::vector<fp> get_all_point_expressions(std::string const &s, 
                                          Data const* data,
                                          bool only_active=true);
bool get_dt_code(std::string const& s, 
                 std::vector<int>& code_, std::vector<fp>& numbers_);
fp get_value_for_point(std::vector<int> const& code_, 
                       std::vector<fp> const& numbers_, fp x, fp y);


/// a part of data expression grammar
struct DataExpressionGrammar : public grammar<DataExpressionGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(DataExpressionGrammar const& /*self*/);

    rule<ScannerT> rprec1, rbool_or, rbool_and, rbool_not, rbool,
                   rprec2, rprec3, rprec4, rprec5; 

    rule<ScannerT> const& start() const { return rprec1; }
  };
};

extern DataExpressionGrammar DataExpressionG;

/// data transformation grammar
struct DataTransformGrammar : public grammar<DataTransformGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(DataTransformGrammar const& /*self*/);

    rule<ScannerT> assignment, statement, range, order;

    rule<ScannerT> const& start() const { return statement; }
  };
};

extern DataTransformGrammar DataTransformG;




#endif 
