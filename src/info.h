// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

#ifndef FITYK_INFO_H_
#define FITYK_INFO_H_

#include <string>
#include <vector>
#include "lexer.h" // Token
#include "cparser.h" // CommandType

class Ftk;

/// returns output of the "info" command
std::string get_info_string(Ftk const* F, std::string const& args);

void run_info(Ftk const* F, int ds,
              CommandType cmd, const std::vector<Token>& args);

void run_debug(const Ftk* F, int ds, const Token& key, const Token&rest);

// utils used in both runner.cpp and info.cpp
class Data;
bool get_data_range(const Data* data, const std::vector<Token>& args, int n,
                    int* lb, int* rb);


#endif // FITYK_INFO_H_
