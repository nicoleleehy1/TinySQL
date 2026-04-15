#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>

namespace ast {

// ---- Column reference --------------------------------------------------
struct ColumnRef {
    std::optional<std::string> table;
    std::string                column;

    std::string to_string() const {
        return table ? (*table + "." + column) : column;
    }
};

// ---- Literal value -----------------------------------------------------
using LiteralValue = std::variant<std::string, double>;

// ---- Comparison operators ----------------------------------------------
enum class CompOp { EQ, NEQ, LT, GT, LEQ, GEQ };

inline std::string comp_op_str(CompOp op) {
    switch (op) {
        case CompOp::EQ:  return "=";
        case CompOp::NEQ: return "!=";
        case CompOp::LT:  return "<";
        case CompOp::GT:  return ">";
        case CompOp::LEQ: return "<=";
        case CompOp::GEQ: return ">=";
    }
    return "?";
}

// ---- Expression --------------------------------------------------------
struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Expr {
    enum class Kind { Column, Literal, Compare, And, Or } kind;

    ColumnRef    col;
    LiteralValue lit;
    CompOp       op{};
    ExprPtr      lhs, rhs;

    static ExprPtr make_column(ColumnRef c) {
        auto e = std::make_shared<Expr>(); e->kind = Kind::Column; e->col = std::move(c); return e;
    }
    static ExprPtr make_literal(LiteralValue v) {
        auto e = std::make_shared<Expr>(); e->kind = Kind::Literal; e->lit = std::move(v); return e;
    }
    static ExprPtr make_compare(CompOp op, ExprPtr l, ExprPtr r) {
        auto e = std::make_shared<Expr>(); e->kind = Kind::Compare;
        e->op = op; e->lhs = std::move(l); e->rhs = std::move(r); return e;
    }
    static ExprPtr make_and(ExprPtr l, ExprPtr r) {
        auto e = std::make_shared<Expr>(); e->kind = Kind::And;
        e->lhs = std::move(l); e->rhs = std::move(r); return e;
    }
    static ExprPtr make_or(ExprPtr l, ExprPtr r) {
        auto e = std::make_shared<Expr>(); e->kind = Kind::Or;
        e->lhs = std::move(l); e->rhs = std::move(r); return e;
    }
};

// ---- JOIN condition ----------------------------------------------------
struct JoinCondition {
    ColumnRef left;
    ColumnRef right;
};

// ---- Aggregate function ------------------------------------------------
enum class AggFunc { COUNT_STAR, COUNT, SUM, AVG, MIN, MAX };

struct AggCall {
    AggFunc                    func;
    std::optional<ColumnRef>   col;   // nullopt for COUNT(*)

    // Canonical display label — used as the output column name.
    std::string label() const {
        switch (func) {
            case AggFunc::COUNT_STAR: return "COUNT(*)";
            case AggFunc::COUNT:      return "COUNT(" + col->to_string() + ")";
            case AggFunc::SUM:        return "SUM("   + col->to_string() + ")";
            case AggFunc::AVG:        return "AVG("   + col->to_string() + ")";
            case AggFunc::MIN:        return "MIN("   + col->to_string() + ")";
            case AggFunc::MAX:        return "MAX("   + col->to_string() + ")";
        }
        return "?";
    }
};

// ---- Output column -----------------------------------------------------
// Three kinds: bare star (*), column reference, or aggregate call.
struct OutputColumn {
    enum class Kind { Star, Column, Aggregate } kind = Kind::Column;
    ColumnRef ref;   // Kind::Column
    AggCall   agg;   // Kind::Aggregate

    static OutputColumn make_star() {
        OutputColumn oc; oc.kind = Kind::Star; return oc;
    }
    static OutputColumn make_col(ColumnRef r) {
        OutputColumn oc; oc.kind = Kind::Column; oc.ref = std::move(r); return oc;
    }
    static OutputColumn make_agg(AggCall a) {
        OutputColumn oc; oc.kind = Kind::Aggregate; oc.agg = std::move(a); return oc;
    }

    // Label used as the result column name.
    std::string label() const {
        switch (kind) {
            case Kind::Star:      return "*";
            case Kind::Column:    return ref.column;
            case Kind::Aggregate: return agg.label();
        }
        return "?";
    }
};

// ---- SELECT statement --------------------------------------------------
struct SelectStmt {
    std::vector<OutputColumn>     select_cols;
    std::string                   from_table;
    std::optional<std::string>    from_alias;

    std::optional<std::string>    join_table;
    std::optional<std::string>    join_alias;
    std::optional<JoinCondition>  join_on;

    ExprPtr where_expr;   // nullptr = no WHERE

    // GROUP BY (single column)
    std::optional<ColumnRef>  group_by;

    // ORDER BY
    std::optional<OutputColumn> order_by;      // Column or Aggregate
    bool                        order_by_asc = true;

    // LIMIT
    std::optional<std::size_t> limit;
};

} // namespace ast
