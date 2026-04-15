#pragma once
#include "ast.hpp"
#include "tokenizer.hpp"
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    ast::SelectStmt parse();   // throws std::runtime_error

private:
    std::vector<Token> toks_;
    std::size_t        pos_ = 0;

    const Token& cur()  const;
    const Token& peek(int offset = 1) const;
    Token        consume();
    Token        expect(TokenType t, const std::string& ctx);
    bool         match(TokenType t);

    ast::SelectStmt  parse_select();
    ast::OutputColumn parse_output_column();  // handles agg functions + plain cols
    ast::ColumnRef   parse_col_ref();
    ast::ExprPtr     parse_expr();       // OR level
    ast::ExprPtr     parse_and_expr();   // AND level
    ast::ExprPtr     parse_comparison(); // leaf / grouped
    ast::ExprPtr     parse_primary();
};
