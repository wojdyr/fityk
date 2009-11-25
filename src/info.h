// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

#ifndef FITYK_INFO_H_
#define FITYK_INFO_H_

#include <string>

class Ftk;

/// put output of the "info" command to result, return the postion after
/// the last parsed character. Parsing starts at position start.
size_t get_info_string(Ftk const* F, std::string const& args, bool full,
                       std::string& result,
                       size_t start=0);

bool get_info_string_safe(Ftk const* F, std::string const& args, bool full,
                          std::string& result);

void output_info(Ftk const* F, std::string const& args, bool full);

#endif // FITYK_INFO_H_
