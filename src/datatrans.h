// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef DATATRANS__H__
#define DATATRANS__H__


#include <boost/spirit/core.hpp>

#include "common.h"
using namespace boost::spirit;

struct Point;

bool transform_data(std::string const& str, 
                    std::vector<Point> const& old_points, 
                    std::vector<Point> &new_points);

bool validate_transformation(std::string const& str); 
fp get_transform_expression_value(std::string const &s);


struct DataTransformGrammar : public grammar<DataTransformGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(DataTransformGrammar const& /*self*/);

    rule<ScannerT> rprec1, rprec2, rprec3, rprec4, rprec5, rprec6,  
                   rbool_or, rbool_and, rbool_not, rbool,
                   real_constant, real_variable, parameterized_args,
                   index, assignment, statement, range, order;

    rule<ScannerT> const& start() const { return statement; }
  };
};

extern DataTransformGrammar DataTransformG;




#endif //DATATRANS__H__
