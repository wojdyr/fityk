// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK_CMD__H__
#define FITYK_CMD__H__


#include "common.h"
#include "ui.h"

/// return true if the syntax is correct
bool check_command_syntax(std::string const& str);

/// parse and execute command; does not throw exceptions, returns status 
Commands::Status parse_and_execute(std::string const& str);

/// like parse_and_execute(), but returns false on syntax error, throw exception on execute error
bool parse_and_execute_e(std::string const& str);

/// return output of info command
std::string get_info_string(std::string const& s, bool full);


#endif 
