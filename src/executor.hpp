#pragma once
#include "ast.hpp"
#include "table.hpp"
#include <unordered_map>
#include <string>
#include <vector>

// ---- Join strategy -----------------------------------------------------
enum class JoinStrategy { NestedLoop, Hash, SortMerge };

struct JoinPlan {
    JoinStrategy strategy;
    std::string  reason;
};

// cond is needed so the optimizer can inspect the key column name.
JoinPlan choose_join_strategy(const Table& left, const Table& right,
                               const ast::JoinCondition& cond);

// ---- Execution context -------------------------------------------------
// Holds all table/alias metadata needed to resolve a ColumnRef to a value
// inside a row (which may be a plain row or a combined join row).
struct ExecutionContext {
    const Table* left_tbl   = nullptr;
    std::string  left_alias;
    bool         is_join    = false;
    const Table* right_tbl  = nullptr;
    std::string  right_alias;

    // Resolve ref → index into a (possibly combined) row.
    // Returns -1 when not found (caller decides whether to throw).
    int resolve(const ast::ColumnRef& ref) const;

    // Get value from row, throws on unknown column.
    Value get_value(const Row& row, const ast::ColumnRef& ref) const;
};

// ---- WHERE expression evaluators ---------------------------------------

bool eval_expr(const ast::ExprPtr& expr, const Row& row, const Table& schema);

bool eval_expr_ctx(const ast::ExprPtr& expr,
                   const Row& row,
                   const ExecutionContext& ctx);

// ---- Projection --------------------------------------------------------

Row project_row(const Row& row, const Table& schema,
                const std::vector<ast::OutputColumn>& cols);

Row project_row_ctx(const Row& row, const ExecutionContext& ctx,
                    const std::vector<ast::OutputColumn>& cols);

// Build result column name list for a non-aggregate query.
std::vector<std::string> output_column_names(
    const ExecutionContext& ctx,
    const std::vector<ast::OutputColumn>& cols);

// ---- Post-processing pipeline ------------------------------------------

// Apply GROUP BY + aggregation; returns a new Table.
Table apply_aggregation(const std::vector<Row>& rows,
                        const ExecutionContext& ctx,
                        const std::vector<ast::OutputColumn>& out_cols,
                        const std::optional<ast::ColumnRef>& group_by);

// Sort result rows in-place by the given output column.
void apply_order_by(Table& result,
                    const ast::OutputColumn& order_col,
                    bool asc);

// Truncate result rows to at most n rows.
void apply_limit(Table& result, std::size_t n);

// ---- Main entry point --------------------------------------------------

Table execute(const ast::SelectStmt& stmt,
              const std::unordered_map<std::string, Table>& db);
