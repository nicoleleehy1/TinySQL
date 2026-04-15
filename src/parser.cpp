#include "parser.hpp"
#include <stdexcept>

// ---- Helpers -----------------------------------------------------------

Parser::Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

const Token& Parser::cur()  const { return toks_[pos_]; }
const Token& Parser::peek(int off) const {
    std::size_t p = pos_ + off;
    return p < toks_.size() ? toks_[p] : toks_.back();
}

Token Parser::consume() {
    Token t = toks_[pos_];
    if (pos_ + 1 < toks_.size()) ++pos_;
    return t;
}

Token Parser::expect(TokenType t, const std::string& ctx) {
    if (cur().type != t)
        throw std::runtime_error(
            "Parse error near '" + cur().value + "' (line " +
            std::to_string(cur().line) + "): expected " + ctx);
    return consume();
}

bool Parser::match(TokenType t) {
    if (cur().type == t) { consume(); return true; }
    return false;
}

// ---- Public entry point ------------------------------------------------

ast::SelectStmt Parser::parse() {
    auto stmt = parse_select();
    if (cur().type != TokenType::END)
        throw std::runtime_error(
            "Unexpected token '" + cur().value + "' after statement end");
    return stmt;
}

// ---- Aggregate function helper -----------------------------------------

static bool is_agg_keyword(TokenType t) {
    return t == TokenType::KW_COUNT || t == TokenType::KW_SUM ||
           t == TokenType::KW_AVG   || t == TokenType::KW_MIN ||
           t == TokenType::KW_MAX;
}

static ast::AggFunc token_to_agg(TokenType t) {
    switch (t) {
        case TokenType::KW_COUNT: return ast::AggFunc::COUNT;
        case TokenType::KW_SUM:   return ast::AggFunc::SUM;
        case TokenType::KW_AVG:   return ast::AggFunc::AVG;
        case TokenType::KW_MIN:   return ast::AggFunc::MIN;
        case TokenType::KW_MAX:   return ast::AggFunc::MAX;
        default: throw std::logic_error("token_to_agg: not an aggregate keyword");
    }
}

// ---- Output column (SELECT list item) ----------------------------------
// Handles: *, plain col ref, table.col ref, AGG(*), AGG(col)

ast::OutputColumn Parser::parse_output_column() {
    if (cur().type == TokenType::STAR) {
        consume();
        return ast::OutputColumn::make_star();
    }

    if (is_agg_keyword(cur().type)) {
        ast::AggFunc func = token_to_agg(cur().type);
        consume();   // eat COUNT / SUM / …
        expect(TokenType::LPAREN, "(");

        if (func == ast::AggFunc::COUNT && cur().type == TokenType::STAR) {
            consume();   // eat *
            expect(TokenType::RPAREN, ")");
            return ast::OutputColumn::make_agg(ast::AggCall{ast::AggFunc::COUNT_STAR, std::nullopt});
        }

        ast::ColumnRef col = parse_col_ref();
        expect(TokenType::RPAREN, ")");
        return ast::OutputColumn::make_agg(ast::AggCall{func, col});
    }

    return ast::OutputColumn::make_col(parse_col_ref());
}

// ---- SELECT statement --------------------------------------------------

ast::SelectStmt Parser::parse_select() {
    expect(TokenType::KW_SELECT, "SELECT");
    ast::SelectStmt stmt;

    // Column list
    stmt.select_cols.push_back(parse_output_column());
    while (cur().type == TokenType::COMMA) {
        consume();
        stmt.select_cols.push_back(parse_output_column());
    }

    // FROM
    expect(TokenType::KW_FROM, "FROM");
    stmt.from_table = expect(TokenType::IDENTIFIER, "table name").value;
    // optional table alias (only if the next token is a plain identifier)
    if (cur().type == TokenType::IDENTIFIER)
        stmt.from_alias = consume().value;

    // Optional INNER? JOIN
    if (cur().type == TokenType::KW_INNER) consume();
    if (cur().type == TokenType::KW_JOIN) {
        consume();
        stmt.join_table = expect(TokenType::IDENTIFIER, "join table name").value;
        if (cur().type == TokenType::IDENTIFIER)
            stmt.join_alias = consume().value;
        expect(TokenType::KW_ON, "ON");
        ast::ColumnRef left  = parse_col_ref();
        expect(TokenType::EQ, "=");
        ast::ColumnRef right = parse_col_ref();
        stmt.join_on = ast::JoinCondition{std::move(left), std::move(right)};
    }

    // Optional WHERE
    if (cur().type == TokenType::KW_WHERE) {
        consume();
        stmt.where_expr = parse_expr();
    }

    // Optional GROUP BY col
    if (cur().type == TokenType::KW_GROUP) {
        consume();
        expect(TokenType::KW_BY, "BY");
        stmt.group_by = parse_col_ref();
    }

    // Optional ORDER BY col|agg [ASC|DESC]
    if (cur().type == TokenType::KW_ORDER) {
        consume();
        expect(TokenType::KW_BY, "BY");
        stmt.order_by = parse_output_column();
        if (cur().type == TokenType::KW_DESC) {
            consume();
            stmt.order_by_asc = false;
        } else if (cur().type == TokenType::KW_ASC) {
            consume();
            stmt.order_by_asc = true;
        }
    }

    // Optional LIMIT n
    if (cur().type == TokenType::KW_LIMIT) {
        consume();
        Token num = expect(TokenType::NUMBER, "integer row count");
        double d = std::stod(num.value);
        if (d < 0 || d != static_cast<double>(static_cast<std::size_t>(d)))
            throw std::runtime_error("LIMIT requires a non-negative integer");
        stmt.limit = static_cast<std::size_t>(d);
    }

    return stmt;
}

// ---- Column reference --------------------------------------------------

ast::ColumnRef Parser::parse_col_ref() {
    std::string first = expect(TokenType::IDENTIFIER, "column name").value;
    if (cur().type == TokenType::DOT) {
        consume();
        std::string col = expect(TokenType::IDENTIFIER, "column name after '.'").value;
        return {first, col};
    }
    return {std::nullopt, first};
}

// ---- Expression (recursive descent) -----------------------------------

ast::ExprPtr Parser::parse_expr() {
    auto left = parse_and_expr();
    while (cur().type == TokenType::KW_OR) {
        consume();
        auto right = parse_and_expr();
        left = ast::Expr::make_or(std::move(left), std::move(right));
    }
    return left;
}

ast::ExprPtr Parser::parse_and_expr() {
    auto left = parse_comparison();
    while (cur().type == TokenType::KW_AND) {
        consume();
        auto right = parse_comparison();
        left = ast::Expr::make_and(std::move(left), std::move(right));
    }
    return left;
}

ast::ExprPtr Parser::parse_comparison() {
    if (cur().type == TokenType::LPAREN) {
        consume();
        auto inner = parse_expr();
        expect(TokenType::RPAREN, ")");
        return inner;
    }
    auto left = parse_primary();

    ast::CompOp op;
    switch (cur().type) {
        case TokenType::EQ:  op = ast::CompOp::EQ;  break;
        case TokenType::NEQ: op = ast::CompOp::NEQ; break;
        case TokenType::LT:  op = ast::CompOp::LT;  break;
        case TokenType::GT:  op = ast::CompOp::GT;  break;
        case TokenType::LEQ: op = ast::CompOp::LEQ; break;
        case TokenType::GEQ: op = ast::CompOp::GEQ; break;
        default:
            throw std::runtime_error(
                "Expected comparison operator near '" + cur().value +
                "' (line " + std::to_string(cur().line) + ")");
    }
    consume();
    auto right = parse_primary();
    return ast::Expr::make_compare(op, std::move(left), std::move(right));
}

ast::ExprPtr Parser::parse_primary() {
    if (cur().type == TokenType::STRING_LIT) {
        std::string s = consume().value;
        return ast::Expr::make_literal(ast::LiteralValue{s});
    }
    if (cur().type == TokenType::NUMBER) {
        double d = std::stod(consume().value);
        return ast::Expr::make_literal(ast::LiteralValue{d});
    }
    if (cur().type == TokenType::IDENTIFIER) {
        return ast::Expr::make_column(parse_col_ref());
    }
    throw std::runtime_error(
        "Expected value or column near '" + cur().value +
        "' (line " + std::to_string(cur().line) + ")");
}
