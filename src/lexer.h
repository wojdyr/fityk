// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

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
        : input_(input), cur_(input), peeked_(false),
          opened_paren_(0), opened_square_(0), opened_curly_(0) {}

    Token get_token();

    const Token& peek_token();

    // Filename is expected by parser. Reads any sequence of non-blank
    // characters (with exception of ' and #) as a file.
    // The filename can be inside 'single quotes'.
    Token get_filename_token();

    // works properly only if the last token is used as argument
    void go_back(const Token& token);

    int opened_paren() { return opened_paren_; }
    int opened_square() { return opened_square_; }
    int opened_curly() { return opened_curly_; }

    void throw_syntax_error(const std::string& msg="");

private:
    void read_token();

    const char* const input_; // used for diagnostic messages only
    const char* cur_;
    bool peeked_;
    Token tok_;
    int opened_paren_, opened_square_, opened_curly_;
};

std::string token2str(const Token& token);

#endif // FITYK__LEXER__H__
