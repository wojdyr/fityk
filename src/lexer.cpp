// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

#include "lexer.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "fityk.h" // SyntaxError
#include "common.h" // S()

using namespace std;
using fityk::SyntaxError;

string Lexer::get_string(const Token& token)
{
    const char* p = token.str;
    switch (token.type) {
        case kTokenName:
            return string(p, p + token.info.length);
        case kTokenString:
            return string(p+1, p + token.info.length - 1);
        case kTokenVarname:
            return string(p+1, p + token.info.length);
        case kTokenFuncname:
            return string(p+1, p + token.info.length);
        case kTokenShell:
            return string(p+1);
        default:
            assert(!"Unexpected token in get_string()");
    }
}

namespace {

string get_quoted_string(const Token& token)
{
    return '"' + Lexer::get_string(token) + '"';
}

string token2str(const Token& token)
{
    switch (token.type) {
        case kTokenName:
            return "NAME " + get_quoted_string(token);
        case kTokenString:
            return "STRING " + get_quoted_string(token);
        case kTokenVarname:
            return "VARNAME " + get_quoted_string(token);
        case kTokenFuncname:
            return "FUNCNAME " + get_quoted_string(token);
        case kTokenShell:
            return "SHELL " + get_quoted_string(token);
        case kTokenNumber:
            return "NUMBER " + S(token.info.number);
        case kTokenDataset:
            return "DATASET " + S(token.info.dataset);
        case kTokenLE: return "<=";
        case kTokenGE: return ">=";
        case kTokenNE: return "!=";
        case kTokenEQ: return "==";
        case kTokenAppend: return ">>";
        case kTokenDots: return "..";
        case kTokenPlusMinus: return "+-";

        case kTokenOpen: return "(";
        case kTokenClose: return ")";
        case kTokenLSquare: return "[";
        case kTokenRSquare: return "]";
        case kTokenLCurly: return "{";
        case kTokenRCurly: return "}";
        case kTokenPlus: return "+";
        case kTokenMinus: return "-";
        case kTokenMult: return "*";
        case kTokenDiv: return "/";
        case kTokenPower: return "^";
        case kTokenLT: return "<";
        case kTokenGT: return ">";
        case kTokenAssign: return "=";
        case kTokenComma: return ",";
        case kTokenSemicolon: return ";";
        case kTokenDot: return ".";
        case kTokenColon: return ":";
        case kTokenTilde: return "~";
        case kTokenQMark: return "?";

        case kTokenEOL: return "EOL";

        default:
            assert(!"unexpected token in token2str()");
    }
}

} // anonymous namespace

void Lexer::read_token()
{
    tok_.str = cur_;
    while (isspace(*tok_.str))
        ++tok_.str;
    const char* ptr = tok_.str;

    switch (*ptr) {
        case '\0':
        case '#':
            tok_.type = kTokenEOL;
            break;
        case '\'': {
            tok_.type = kTokenString;
            const char* end = strchr(ptr + 1, '\'');
            if (end == NULL)
                throw SyntaxError("unfinished string");
            tok_.info.length = end - ptr + 1;
            ptr = end + 1;
            break;
        }
        case '>':
            ++ptr;
            if (*ptr == '=') {
                tok_.type = kTokenGE;
                ++ptr;
            }
            else if (*ptr == '>') {
                tok_.type = kTokenAppend;
                ++ptr;
            }
            else
                tok_.type = kTokenGT;
            break;
        case '<':
            ++ptr;
            if (*ptr == '=') {
                tok_.type = kTokenLE;
                ++ptr;
            }
            else if (*ptr == '>') {
                tok_.type = kTokenNE;
                ++ptr;
            }
            else
                tok_.type = kTokenLT;
            break;
        case '=':
            ++ptr;
            if (*ptr == '=') {
                tok_.type = kTokenEQ;
                ++ptr;
            }
            else
                tok_.type = kTokenAssign;
            break;
        case '+':
            ++ptr;
            if (*ptr == '-') {
                tok_.type = kTokenPlusMinus;
                ++ptr;
            }
            else
                tok_.type = kTokenPlus;
            break;
        case '-':
            ++ptr;
            tok_.type = kTokenMinus;
            break;
        case '.':
            ++ptr;
            if (isdigit(*ptr)) {
                char* endptr;
                tok_.info.number = strtod(ptr-1, &endptr);
                ptr = endptr;
                tok_.type = kTokenNumber;
            }
            else if (*ptr == '.') {
                ++ptr;
                if (*ptr == '.') // 3rd dot
                    ++ptr;
                tok_.type = kTokenDots;
            }
            else
                tok_.type = kTokenDot;
            break;
        case '@':
            ++ptr;
            tok_.type = kTokenDataset;
            if (*ptr == '*')
                tok_.info.dataset = -1;
            else if (isdigit(*ptr)) {
                char *endptr;
                tok_.info.dataset = strtol(ptr, &endptr, 10);
                ptr = endptr;
            }
            else
                throw SyntaxError("unexpected character after '@'");
            break;
        case '$':
            ++ptr;
            if (! (isalpha(*ptr) || *ptr == '_'))
                throw SyntaxError("unexpected character after '$'");
            tok_.type = kTokenVarname;
            while (isalnum(*ptr) || *ptr == '_')
                ++ptr;
            tok_.info.length = ptr - tok_.str;
            break;
        case '%':
            ++ptr;
            if (! (isalpha(*ptr) || *ptr == '_'))
                throw SyntaxError("unexpected character after '%'");
            tok_.type = kTokenFuncname;
            while (isalnum(*ptr) || *ptr == '_')
                ++ptr;
            tok_.info.length = ptr - tok_.str;
            break;
        case '!':
            ++ptr;
            if (*ptr == '=') {
                tok_.type = kTokenNE;
                ++ptr;
            }
            else {
                tok_.type = kTokenShell;
                int len = strlen(ptr);
                tok_.info.length = len + 1;
                ptr += len;
                assert(*ptr == '\0');
            }
            break;

        case '(': tok_.type = kTokenOpen;      ++ptr; break;
        case ')': tok_.type = kTokenClose;     ++ptr; break;
        case '[': tok_.type = kTokenLSquare;   ++ptr; break;
        case ']': tok_.type = kTokenRSquare;   ++ptr; break;
        case '{': tok_.type = kTokenLCurly;    ++ptr; break;
        case '}': tok_.type = kTokenRCurly;    ++ptr; break;
        case '*': tok_.type = kTokenMult;      ++ptr; break;
        case '/': tok_.type = kTokenDiv;       ++ptr; break;
        case '^': tok_.type = kTokenPower;     ++ptr; break;
        case ',': tok_.type = kTokenComma;     ++ptr; break;
        case ';': tok_.type = kTokenSemicolon; ++ptr; break;
        case ':': tok_.type = kTokenColon;     ++ptr; break;
        case '~': tok_.type = kTokenTilde;     ++ptr; break;
        case '?': tok_.type = kTokenQMark;     ++ptr; break;

        default:
            if (isdigit(*ptr)) {
                char* endptr;
                tok_.info.number = strtod(ptr, &endptr);
                ptr = endptr;
                tok_.type = kTokenNumber;
            }
            else if (isalnum(*ptr) || *ptr == '_') {
                if (mode_ == kMath) {
                    while (isalnum(*ptr) || *ptr == '_')
                        ++ptr;
                }
                else { // kWord
                    while (isalnum(*ptr) || *ptr == '_' || *ptr == '-')
                        ++ptr;
                }
                tok_.info.length = ptr - tok_.str;
                tok_.type = kTokenName;
            }
            else
                throw SyntaxError("unexpected character: " + string(ptr, 1));
    }
    cur_ = ptr;
}

Token Lexer::get_token()
{
    if (!peeked_)
        read_token();
    peeked_ = false;
    if (tok_.type == kTokenOpen)
        ++opened_paren_;
    else if (tok_.type == kTokenLSquare)
        ++opened_square_;
    else if (tok_.type == kTokenLCurly)
        ++opened_curly_;
    else if (tok_.type == kTokenClose)
        --opened_paren_;
    else if (tok_.type == kTokenRSquare)
        --opened_square_;
    else if (tok_.type == kTokenRCurly)
        --opened_curly_;
    return tok_;
}

const Token& Lexer::peek_token()
{
    if (!peeked_)
        read_token();
    peeked_ = true;
    return tok_;
}

void Lexer::go_back(const Token& token)
{
    cur_ = token.str;
    peeked_ = false;
    if (token.type == kTokenOpen)
        --opened_paren_;
    else if (token.type == kTokenLSquare)
        --opened_square_;
    else if (token.type == kTokenLCurly)
        --opened_curly_;
    else if (token.type == kTokenClose)
        ++opened_paren_;
    else if (token.type == kTokenRSquare)
        ++opened_square_;
    else if (token.type == kTokenRCurly)
        ++opened_curly_;
}

void Lexer::throw_syntax_error(const string& msg)
{
    throw SyntaxError("Parsing error: " + msg);
}

#ifdef TEST_FITYK_LEXER

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: lexer mode string      where mode is 'w' or 'm'");
        return 1;
    }
    const char* str = argv[2];
    bool math = (argv[1][0] == 'm');

    Lexer lex(str);
    if (math)
        lex.set_mode(Lexer::kMath);
    printf("scanning (%s mode): %s\n", (math ? "math" : "word"), str);
    fflush(stdout);
    for (;;) {
        Token token = lex.get_token();
        string s = token2str(token);
        printf("%s\n", s.c_str());
        if (token.type == kTokenEOL)
            break;
    }
    return 0;
}
#endif // TEST_FITYK_LEXER
