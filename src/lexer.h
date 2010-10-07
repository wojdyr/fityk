// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This lexer (scanner) is not used yet.
/// In the future it will replace the current lexer/parser.
/// Lexical analyser. Takes characters and yields tokens.

#ifndef FITYK__LEXER__H__
#define FITYK__LEXER__H__

#include <string>

enum TokenType
{
    // tokens with additional info, see TokenInfo
    kTokenName, // name (info.length)
    kTokenString, // 'string' (info.length)
    kTokenNumber, // number (info.number: double)
    kTokenDataset, // @n or @* TODO: @+ (info.dataset)
    kTokenVarname, // $variable (info.length)
    kTokenFuncname, // %function (info.length)
    kTokenShell, // ! command args (info.length)

    // multi-char tokens
    kTokenLE, kTokenGE, // <= >=
    kTokenNE, // != or <>
    kTokenEQ, // ==
    kTokenAppend, // >>
    kTokenDots, // ..
    kTokenPlusMinus, // +-

    // single-char tokens: ( ) [ ] { } + - * / ^ < > = ; , : . ~ ?
    kTokenOpen, kTokenClose, // ( )
    kTokenLSquare, kTokenRSquare, // [ ]
    kTokenLCurly, kTokenRCurly, // { }
    kTokenPlus, kTokenMinus, // + -
    kTokenMult, kTokenDiv, // * /
    kTokenPower, // ^
    kTokenLT, kTokenGT, // < >
    kTokenAssign, // =
    kTokenComma, kTokenSemicolon, // , ;
    kTokenDot, // .
    kTokenColon, // :
    kTokenTilde, // ~
    kTokenQMark, // ?

    kTokenEOL
};


struct Token
{
    TokenType type;

    const char* str; // position in the string

    union
    {
        short length; // raw string length (e.g. 3 for 'a')
        int dataset;
        double number;
    } info;
};

class Lexer
{
public:

    // get string associated with the token. Works only with:
    // kTokenName, kTokenString, kTokenVarname, kTokenFuncname, kTokenShell.
    static std::string get_string(const Token& token);

    // get string associated with the token.
    static std::string get_raw(const Token& token)
        { return std::string(token.str, token.info.length); }

    Lexer(const char* input)
        : input_(input), cur_(input), peeked_(false) {}

    Token get_token();

    const Token& peek_token();

    Token get_expected_token(TokenType tt);

    // Filename is expected by parser. Reads any sequence of non-blank
    // characters (with exception of ' and #) as a file.
    // The filename can be inside 'single quotes'.
    Token get_filename_token();

    void go_back(const Token& token);

    void throw_syntax_error(const std::string& msg="");

    const char* pchar() const { return cur_; }
    int scanned_chars() const { return  cur_ - input_; }

private:
    void read_token();

    const char* const input_; // used for diagnostic messages only
    const char* cur_;
    bool peeked_;
    Token tok_;
};

std::string token2str(const Token& token);

#endif // FITYK__LEXER__H__
