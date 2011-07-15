// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_INFO_H_
#define FITYK_INFO_H_

#include <string>
#include <vector>
#include "lexer.h" // Token
#include "cparser.h" // CommandType

class Ftk;

/// appends output of the "info" command to the result
int eval_info_args(const Ftk* F, int ds, const std::vector<Token>& args,
                   int len, std::string& result);

/// handles commands info and print
void command_redirectable(Ftk const* F, int ds,
                          CommandType cmd, const std::vector<Token>& args);

void command_debug(const Ftk* F, int ds, const Token& key, const Token&rest);

void parse_and_eval_info(Ftk *F, const std::string& s, int dataset,
                         std::string& result);

#endif // FITYK_INFO_H_
