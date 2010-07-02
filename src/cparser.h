// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

#ifndef FITYK_CPARSER_H_
#define FITYK_CPARSER_H_

#include <string>
#include "ui.h"

class StatementList;

class Parser
{
public:
    Parser();
    ~Parser();

    // Parses the string. Throws SyntaxError.
    void parse(const std::string& str);

    // Execute the last parsed string.
    // Throws ExecuteError, ExitRequestedException.
    void execute();

    // Calls parse() and execute(), catches exceptions and returns status.
    Commands::Status parse_and_execute(const std::string& str);

    // The same as parse(), but it doesn't throw. Returns true on success.
    bool check_command_syntax(const std::string& str);

    // Parses the string. Throws SyntaxError.
    void parse(const std::string& str);

    // Execute the last parsed string.
    // Throws ExecuteError, ExitRequestedException.
    void execute();

    // Calls parse() and execute(), catches exceptions and returns status.
    Commands::Status parse_and_execute(const std::string& str);

    // The same as parse(), but it doesn't throw. Returns true on success.
    bool check_command_syntax(const std::string& str);

private:
    string str_;
    bool ok_;
    StatementList *sts_;
};

#endif //FITYK_CPARSER_H_
