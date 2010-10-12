// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This lexer (scanner) is not used yet.
/// In the future it will replace the current lexer/parser.
/// Lexical analyser. Takes characters and yields tokens.

// TODO: sometimes include '*' in words, to allow "delete %pd*".

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
            return string(p, token.length);
        case kTokenString:
            return string(p+1, token.length - 2);
        case kTokenVarname:
            return string(p+1, token.length - 1);
        case kTokenFuncname:
            return string(p+1, token.length - 1);
        case kTokenShell:
            return string(p+1);
        default:
            assert(!"Unexpected token in get_string()");
            return "";
    }
}

inline
string get_quoted_string(const Token& token)
{
    return '"' + Lexer::get_string(token) + '"';
}

const char* tokentype2str(TokenType tt)
{
    switch (tt) {
        case kTokenName: return "NAME";
        case kTokenString: return "STRING";
        case kTokenVarname: return "VARNAME";
        case kTokenFuncname: return "FUNCNAME";
        case kTokenShell: return "SHELL";
        case kTokenNumber: return "NUMBER";
        case kTokenDataset: return "DATASET";

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

        case kTokenNop: return "Nop";

        default:
            assert(!"unexpected token in tokentype2str()");
            return NULL;
    }
}

string token2str(const Token& token)
{
    string s = tokentype2str(token.type);
    switch (token.type) {
        case kTokenName:
        case kTokenString:
        case kTokenVarname:
        case kTokenFuncname:
        case kTokenShell:
            return s + " " + get_quoted_string(token);
        case kTokenNumber:
            return s + " " + S(token.info.number);
        case kTokenDataset:
            return s + " " + S(token.info.dataset);
        default:
            return s;
    }
}

void Lexer::read_token()
{
    tok_.str = cur_;
    while (isspace(*tok_.str))
        ++tok_.str;
    const char* ptr = tok_.str;

    switch (*ptr) {
        case '\0':
        case '#':
            tok_.type = kTokenNop;
            break;
        case '\'': {
            tok_.type = kTokenString;
            const char* end = strchr(ptr + 1, '\'');
            if (end == NULL)
                throw SyntaxError("unfinished string");
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
            break;
        case '%':
            ++ptr;
            if (! (isalpha(*ptr) || *ptr == '_'))
                throw SyntaxError("unexpected character after '%'");
            tok_.type = kTokenFuncname;
            while (isalnum(*ptr) || *ptr == '_')
                ++ptr;
            break;
        case '!':
            ++ptr;
            if (*ptr == '=') {
                tok_.type = kTokenNE;
                ++ptr;
            }
            else {
                tok_.type = kTokenShell;
                while (*ptr != '\0')
                    ++ptr;
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
            else if (isalpha(*ptr) || *ptr == '_') {
                while (isalnum(*ptr) || *ptr == '_')
                    ++ptr;
                tok_.type = kTokenName;
            }
            else
                throw SyntaxError("unexpected character: " + string(ptr, 1));
    }
    tok_.length = ptr - tok_.str;
    cur_ = ptr;
}

Token Lexer::get_token()
{
    if (!peeked_)
        read_token();
    peeked_ = false;
    return tok_;
}

Token Lexer::get_expected_token(TokenType tt)
{
    if (peek_token().type != tt)
        throw_syntax_error(S("expected ") + tokentype2str(tt) + " instead of "
                           + tokentype2str(peek_token().type));
    return get_token();
}

Token Lexer::get_token_if(TokenType tt)
{
    if (peek_token().type == tt)
        return get_token();
    else {
        Token token;
        token.type = kTokenNop;
        token.str = cur_;
        token.length = 0;
        return token;
    }
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
}

void Lexer::throw_syntax_error(const string& msg)
{
    throw SyntaxError("Parsing error: " + msg);
}

