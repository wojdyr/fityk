// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// Lexical analyser. Takes characters and yields tokens.

#ifndef FITYK__LEXER__H__
#define FITYK__LEXER__H__

#include <string>

namespace fityk {

class VMData;

#ifdef _WIN32
// On Windows TokenType is a constant in _TOKEN_INFORMATION_CLASS in Winnt.h
#define TokenType FitykTokenType
#endif

enum TokenType
{
    // textual tokens
    kTokenLname, // (Lowercase | '_') {Lowercase | Digit | '_'}
    kTokenCname, // Uppercase (Alpha | Digit) {Alpha | Digit}
    kTokenUletter, // single uppercase letter
    kTokenString, // 'string'
    kTokenVarname, // $variable
    kTokenFuncname, // %function

    // special tokens:

    // returned only by get_filename_token(), not by get_token()
    kTokenFilename,
    // expression as string, never returned by Lexer, used only in Parser
    kTokenExpr,
    // variable or expression meant to create a new variable,
    // never returned by Lexer, used only in Parser
    kTokenEVar,
    // it can be returned by get_rest_of_line()
    kTokenRest,

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
    kTokenAddAssign, kTokenSubAssign, // += -=

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
    kTokenBang, // !

    kTokenNop, // end of line (returned by Lexer) or placeholder (in Parser)
};

const char* tokentype2str(TokenType tt);

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
    // kTokenString, kTokenVarname, kTokenFuncname.
    static std::string get_string(const Token& token);

    Lexer(const char* input)
        : input_(input), cur_(input), peeked_(false) {}

    Token get_token();

    // look ahead and read the next token without removing it
    const Token& peek_token();

    Token get_expected_token(TokenType tt);
    Token get_expected_token(const std::string& raw);
    Token get_expected_token(TokenType tt1, TokenType tt2);
    Token get_expected_token(TokenType tt, const std::string& raw);
    Token get_expected_token(const std::string& raw1, const std::string& raw2);

    // if the next token is of type `tt' get it, otherwise return kTokenNop
    Token get_token_if(TokenType tt);

    bool discard_token_if(TokenType tt)
        { return get_token_if(tt).type != kTokenNop; }

    // Unlike get_token(), allow '*' in $variables and %functions. 
    Token get_glob_token();

    // Filename is expected by parser. Reads any sequence of non-blank
    // characters (with exception of ' and #) as a file.
    // The filename can be inside 'single quotes'.
    Token get_filename_token();

    // Reads rest of the line and returns kTokenRest
    Token get_rest_of_line();

    void go_back(const Token& token);

    void throw_syntax_error(const std::string& msg="");

    const char* pchar() const { return peeked_ ? tok_.str : cur_; }
    int scanned_chars() const { return  pchar() - input_; }

private:
    void read_token(bool allow_glob=false);

    const char* const input_; // used for diagnostic messages only
    const char* cur_;
    bool peeked_;
    Token tok_;
};

std::string token2str(const Token& token);

} // namespace fityk
#endif // FITYK__LEXER__H__
