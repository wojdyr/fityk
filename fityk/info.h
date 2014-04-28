// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_INFO_H_
#define FITYK_INFO_H_

#include <string>
#include <vector>
#include "lexer.h" // Token
#include "cparser.h" // CommandType

namespace fityk {

std::string build_info();

/// appends output of the "info" command to the result
int eval_info_args(const Full* F, int ds, const std::vector<Token>& args,
                   int len, std::string& result);

/// handles commands info and print
void command_redirectable(Full const* F, int ds,
                          CommandType cmd, const std::vector<Token>& args);

void command_debug(const Full* F, int ds, const Token& key, const Token&rest);

FITYK_API void parse_and_eval_info(Full *F, const std::string& s, int dataset,
                                   std::string& result);
FITYK_API std::string& gnuplotize_formula(std::string& formula);
FITYK_API void models_as_script(const Full* F, std::string& r,
                                bool commented_defines);

} // namespace fityk
#endif // FITYK_INFO_H_
