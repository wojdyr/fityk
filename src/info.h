// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

#ifndef FITYK_INFO_H_
#define FITYK_INFO_H_

#include <string>
#include <vector>
#include "lexer.h" // Token

class Ftk;

/// returns output of the "info" command
std::string get_info_string(Ftk const* F, std::string const& args);

void do_command_info(Ftk const* F, int ds, const std::vector<Token>& args);

void do_command_debug(const Ftk* F, const std::string& args);

// utils used in both runner.cpp and info.cpp
class Data;
bool get_data_range(const Data* data, const std::vector<Token>& args, int n,
                    int* lb, int* rb);


#endif // FITYK_INFO_H_
