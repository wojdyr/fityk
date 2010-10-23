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
    // textual tokens
    kTokenLname, // (Lowercase | '_') {Lowercase | Digit | '_'}
    kTokenCname, // Uppercase (Alpha | Digit) {Alpha | Digit}
    kTokenUletter, // single uppercase letter
    kTokenString, // 'string'
    kTokenVarname, // $variable
    kTokenFuncname, // %function
    kTokenShell, // ! command args

    // special value, never returned by get_token();
    // it can be returned by get_filename_token() or read_expr()
    kTokenRaw,

    // tokens with `value' set
    kTokenNumber, // number (value.d)
    kTokenDataset, // @n, @*, @+ (value.i)

    // 2-char tokens
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

    kTokenNop, // end of line (returned by Lexer) or placeholder (in Parser)
};

// with 64-bit alignment sizeof(Token) is 24
struct Token
{
    const char* str; // position in the string
    TokenType type;
    short length; // raw string length (e.g. 3 for 'a')

    union
    {
        int i;
        double d;
    } value;

    // get string associated with the token.
    std::string as_string() const { return std::string(str, length); }
};

class Lexer
{
public:
    // dataset special values
    enum { kAll = -1, kNew = -2 };

    // get string associated with the token. Works only with:
    // kTokenLname, kTokenCname, kTokenUletter,
    // kTokenString, kTokenVarname, kTokenFuncname, kTokenShell.
    static std::string get_string(const Token& token);

    Lexer(const char* input)
        : input_(input), cur_(input), peeked_(false) {}

    Token get_token();

    const Token& peek_token();

    Token get_expected_token(TokenType tt);
    Token get_expected_token(const std::string& raw);
    Token get_expected_token(TokenType tt1, TokenType tt2);
    Token get_expected_token(TokenType tt, const std::string& raw);

    Token get_token_if(TokenType tt);

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
