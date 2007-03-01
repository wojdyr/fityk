// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK_CMD__H__
#define FITYK_CMD__H__


#include "common.h"
#include "ui.h"

bool check_command_syntax(std::string const& str);
Commands::Status parse_and_execute(std::string const& str);


#endif 
