// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This lexer (scanner) is not used yet.
/// In the future it will replace the current lexer/parser.
/// Lexical analyser. Takes characters and yields tokens.

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
    switch (token.type) {
        case kTokenString:
            return string(token.str+1, token.length - 2);
        case kTokenVarname:
            return string(token.str+1, token.length - 1);
        case kTokenFuncname:
            return string(token.str+1, token.length - 1);
        default:
            assert(!"Unexpected token in get_string()");
            return "";
    }
}

const char* tokentype2str(TokenType tt)
{
    switch (tt) {
        case kTokenLname: return "lower_case_name";
        case kTokenCname: return "CamelCaseName";
        case kTokenUletter: return "Upper-case-letter";
        case kTokenString: return "'quoted-string'";
        case kTokenVarname: return "$variable_name";
        case kTokenFuncname: return "%func_name";
        case kTokenNumber: return "number";
        case kTokenDataset: return "@dataset";
        case kTokenFilename: return "filename";
        case kTokenExpr: return "expr";
        case kTokenRest: return "rest-of-line";

        case kTokenLE: return "<=";
        case kTokenGE: return ">=";
        case kTokenNE: return "!=";
        case kTokenEQ: return "==";
        case kTokenAppend: return ">>";
        case kTokenDots: return "..";
        case kTokenPlusMinus: return "+-";
        case kTokenAddAssign: return "+=";
        case kTokenSubAssign: return "-=";

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
        case kTokenBang: return "!";

        case kTokenNop: return "Nop";
    }
    return NULL; // avoid compiler warning
}

string token2str(const Token& token)
{
    string s = tokentype2str(token.type);
    switch (token.type) {
        case kTokenString:
        case kTokenVarname:
        case kTokenFuncname:
        case kTokenLname:
        case kTokenCname:
        case kTokenUletter:
        case kTokenFilename:
        case kTokenRest:
            return s + " \"" + token.as_string() + "\"";
        case kTokenExpr:
            return s + " \"" + token.as_string() + "\" ("+S(token.value.d)+")";
        case kTokenNumber:
            return s + " " + S(token.value.d);
        case kTokenDataset:
            if (token.value.i == Lexer::kAll)
                return s + " '*'";
            else if (token.value.i == Lexer::kNew)
                return s + " '+'";
            else
                return s + " " + S(token.value.i);
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
            else if (*ptr == '=') {
                tok_.type = kTokenAddAssign;
                ++ptr;
            }
            else
                tok_.type = kTokenPlus;
            break;
        case '-':
            ++ptr;
            if (*ptr == '=') {
                tok_.type = kTokenSubAssign;
                ++ptr;
            }
            else
                tok_.type = kTokenMinus;
            break;

        case '!':
            ++ptr;
            if (*ptr == '=') {
                tok_.type = kTokenNE;
                ++ptr;
            }
            else
                tok_.type = kTokenBang;
            break;

        case '.':
            ++ptr;
            if (isdigit(*ptr)) {
                char* endptr;
                tok_.value.d = strtod(ptr-1, &endptr);
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
            if (*ptr == '*') {
                tok_.value.i = kAll;
                ++ptr;
            }
            else if (*ptr == '+') {
                tok_.value.i = kNew;
                ++ptr;
            }
            else if (isdigit(*ptr)) {
                char *endptr;
                tok_.value.i = strtol(ptr, &endptr, 10);
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
                tok_.value.d = strtod(ptr, &endptr);
                ptr = endptr;
                tok_.type = kTokenNumber;
            }
            else if (isupper(*ptr)) {
                ++ptr;
                if (isalnum(*ptr)) {
                    while (isalnum(*ptr))
                        ++ptr;
                    tok_.type = kTokenCname;
                }
                else
                    tok_.type = kTokenUletter;
            }
            else if (isalpha(*ptr) || *ptr == '_') {
                while (isalnum(*ptr) || *ptr == '_')
                    ++ptr;
                tok_.type = kTokenLname;
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

Token Lexer::get_filename_token()
{
    Token t = get_token();
    if (t.type == kTokenString || t.type == kTokenNop)
        return get_token();
    while (*cur_ != '\0' && !isspace(*cur_) && *cur_ != ';' && *cur_ != '#')
        ++cur_;
    t.type = kTokenFilename;
    t.length = cur_ - t.str;
    return t;
}

Token Lexer::get_rest_of_line()
{
    Token t = get_token();
    while (t.str + t.length != '\0')
        ++t.length;
    if (t.type == kTokenString || t.type == kTokenNop)
        return get_token();
    t.type = kTokenRest;
    return t;
}

Token Lexer::get_expected_token(const string& raw)
{
    TokenType p = peek_token().type;
    string s = peek_token().as_string();
    if (s != raw) {
        string msg = "expected `" + raw + "'";
        throw_syntax_error(p == kTokenNop ? msg
                                          : msg + " instead of `" + s + "'");
    }
    return get_token();
}

Token Lexer::get_expected_token(TokenType tt)
{
    TokenType p = peek_token().type;
    if (p != tt) {
        string msg = S("expected ") + tokentype2str(tt);
        throw_syntax_error(p == kTokenNop ? msg
                                    : msg + " instead of " + tokentype2str(p));
    }
    return get_token();
}

Token Lexer::get_expected_token(TokenType tt1, TokenType tt2)
{
    TokenType p = peek_token().type;
    if (p != tt1 && p != tt2) {
        string msg = S("expected ") + tokentype2str(tt1)
                     + " or " + tokentype2str(tt2);
        throw_syntax_error(p == kTokenNop ? msg
                                    : msg + " instead of " + tokentype2str(p));
    }
    return get_token();
}

Token Lexer::get_expected_token(TokenType tt, const string& raw)
{
    TokenType p = peek_token().type;
    string s = peek_token().as_string();
    if (p != tt && s != raw) {
        string msg = S("expected ") + tokentype2str(tt) + " or `" + raw + "'";
        throw_syntax_error(p == kTokenNop ? msg
                                          : msg + " instead of `" + s + "'");
    }
    return get_token();
}

Token Lexer::get_expected_token(const string& raw1, const string& raw2)
{
    TokenType p = peek_token().type;
    string s = peek_token().as_string();
    if (s != raw1 && s != raw2) {
        string msg = "expected `" + raw1 + "' or `" + raw2 + "'";
        throw_syntax_error(p == kTokenNop ? msg
                                          : msg + " instead of `" + s + "'");
    }
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

void Lexer::throw_syntax_error(const string& msg)
{
    int pos = cur_ - input_;
    string s = S(pos);
    if (pos >= 10)
        s += ", near `" + string(cur_ - 10, cur_) + "'";
    throw SyntaxError("at " + s + ": " + msg);
}

