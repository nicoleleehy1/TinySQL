#include "tokenizer.hpp"
#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"SELECT", TokenType::KW_SELECT},
    {"FROM",   TokenType::KW_FROM  },
    {"WHERE",  TokenType::KW_WHERE },
    {"JOIN",   TokenType::KW_JOIN  },
    {"INNER",  TokenType::KW_INNER },
    {"ON",     TokenType::KW_ON    },
    {"AND",    TokenType::KW_AND   },
    {"OR",     TokenType::KW_OR    },
    {"GROUP",  TokenType::KW_GROUP },
    {"ORDER",  TokenType::KW_ORDER },
    {"BY",     TokenType::KW_BY    },
    {"ASC",    TokenType::KW_ASC   },
    {"DESC",   TokenType::KW_DESC  },
    {"LIMIT",  TokenType::KW_LIMIT },
    {"COUNT",  TokenType::KW_COUNT },
    {"SUM",    TokenType::KW_SUM   },
    {"AVG",    TokenType::KW_AVG   },
    {"MIN",    TokenType::KW_MIN   },
    {"MAX",    TokenType::KW_MAX   },
};

Tokenizer::Tokenizer(std::string src) : src_(std::move(src)) {}

char Tokenizer::cur()  const { return pos_ < src_.size() ? src_[pos_]     : '\0'; }
char Tokenizer::peek() const { return pos_+1 < src_.size() ? src_[pos_+1] : '\0'; }

void Tokenizer::advance() {
    if (pos_ < src_.size() && src_[pos_] == '\n') ++line_;
    ++pos_;
}

void Tokenizer::skip_ws() {
    while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(cur())))
        advance();
    // line comments
    if (cur() == '-' && peek() == '-') {
        while (pos_ < src_.size() && cur() != '\n') advance();
        skip_ws();
    }
}

Token Tokenizer::read_word() {
    std::size_t start = pos_;
    while (std::isalnum(static_cast<unsigned char>(cur())) || cur() == '_') advance();
    std::string raw = src_.substr(start, pos_ - start);
    std::string upper = raw;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    auto it = KEYWORDS.find(upper);
    if (it != KEYWORDS.end())
        return {it->second, raw, line_};
    return {TokenType::IDENTIFIER, raw, line_};
}

Token Tokenizer::read_string() {
    advance(); // consume opening quote
    std::string val;
    while (pos_ < src_.size() && cur() != '\'') {
        if (cur() == '\\' && peek() == '\'') { advance(); } // escaped quote
        val += cur();
        advance();
    }
    if (cur() != '\'')
        throw std::runtime_error("Unterminated string literal at line " + std::to_string(line_));
    advance(); // consume closing quote
    return {TokenType::STRING_LIT, val, line_};
}

Token Tokenizer::read_number() {
    std::size_t start = pos_;
    while (std::isdigit(static_cast<unsigned char>(cur()))) advance();
    if (cur() == '.' && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(cur()))) advance();
    }
    return {TokenType::NUMBER, src_.substr(start, pos_ - start), line_};
}

Token Tokenizer::read_op() {
    char c = cur(); advance();
    switch (c) {
        case ',': return {TokenType::COMMA,  ",", line_};
        case '*': return {TokenType::STAR,   "*", line_};
        case '.': return {TokenType::DOT,    ".", line_};
        case '(': return {TokenType::LPAREN, "(", line_};
        case ')': return {TokenType::RPAREN, ")", line_};
        case '=': return {TokenType::EQ,     "=", line_};
        case '<':
            if (cur() == '=') { advance(); return {TokenType::LEQ, "<=", line_}; }
            return {TokenType::LT, "<", line_};
        case '>':
            if (cur() == '=') { advance(); return {TokenType::GEQ, ">=", line_}; }
            return {TokenType::GT, ">", line_};
        case '!':
            if (cur() == '=') { advance(); return {TokenType::NEQ, "!=", line_}; }
            throw std::runtime_error(
                std::string("Unexpected character '!") + cur() +
                "' at line " + std::to_string(line_));
        default:
            throw std::runtime_error(
                std::string("Unexpected character '") + c +
                "' at line " + std::to_string(line_));
    }
}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skip_ws();
        if (pos_ >= src_.size()) { tokens.push_back({TokenType::END, "", line_}); break; }
        char c = cur();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
            tokens.push_back(read_word());
        else if (c == '\'')
            tokens.push_back(read_string());
        else if (std::isdigit(static_cast<unsigned char>(c)))
            tokens.push_back(read_number());
        else
            tokens.push_back(read_op());
    }
    return tokens;
}
