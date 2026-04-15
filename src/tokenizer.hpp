#pragma once
#include <string>
#include <vector>

enum class TokenType {
    // Keywords — clauses
    KW_SELECT, KW_FROM, KW_WHERE, KW_JOIN, KW_INNER, KW_ON, KW_AND, KW_OR,
    KW_GROUP, KW_ORDER, KW_BY, KW_ASC, KW_DESC, KW_LIMIT,
    // Keywords — aggregate functions
    KW_COUNT, KW_SUM, KW_AVG, KW_MIN, KW_MAX,
    // Identifiers & literals
    IDENTIFIER, STRING_LIT, NUMBER,
    // Operators
    EQ, NEQ, LT, GT, LEQ, GEQ,
    // Punctuation
    COMMA, STAR, DOT, LPAREN, RPAREN,
    // End of input
    END
};

struct Token {
    TokenType   type;
    std::string value;   // raw text
    int         line = 1;
};

class Tokenizer {
public:
    explicit Tokenizer(std::string src);
    std::vector<Token> tokenize();   // throws std::runtime_error on bad input

private:
    std::string src_;
    std::size_t pos_ = 0;
    int         line_ = 1;

    char cur()  const;
    char peek() const;
    void advance();
    void skip_ws();

    Token read_word();
    Token read_string();
    Token read_number();
    Token read_op();
};
