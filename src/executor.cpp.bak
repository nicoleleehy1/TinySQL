#include "executor.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <cassert>

// ========================================================================
// Cost-based optimizer
// ========================================================================

// Returns true when the join key name looks like it could already be sorted
// (i.e. is exactly "id" or ends in "_id", case-insensitive).
static bool key_suggests_sorted(const ast::JoinCondition& cond) {
    auto check = [](const std::string& col) {
        std::string lo = col;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        if (lo == "id") return true;
        if (lo.size() >= 3 && lo.compare(lo.size() - 3, 3, "_id") == 0) return true;
        return false;
    };
    return check(cond.left.column) || check(cond.right.column);
}

JoinPlan choose_join_strategy(const Table& left, const Table& right,
                               const ast::JoinCondition& cond) {
    constexpr std::size_t HASH_THRESHOLD = 100;

    // Small tables: nested loop wins — no setup cost, cache-friendly.
    if (left.row_count() < HASH_THRESHOLD && right.row_count() < HASH_THRESHOLD) {
        return {JoinStrategy::NestedLoop,
                "both tables have < " + std::to_string(HASH_THRESHOLD) + " rows (" +
                std::to_string(left.row_count()) + " + " +
                std::to_string(right.row_count()) + ")"};
    }

    // Both large + key name suggests pre-sorted data: sort-merge avoids
    // building a hash table and may skip the sort entirely in practice.
    if (left.row_count() >= HASH_THRESHOLD && right.row_count() >= HASH_THRESHOLD
            && key_suggests_sorted(cond)) {
        // Report which key triggered the heuristic.
        std::string key_hint;
        auto lo_left  = cond.left.column;
        auto lo_right = cond.right.column;
        std::transform(lo_left.begin(),  lo_left.end(),  lo_left.begin(),  ::tolower);
        std::transform(lo_right.begin(), lo_right.end(), lo_right.begin(), ::tolower);
        if (lo_left == "id" || (lo_left.size() >= 3 &&
                lo_left.compare(lo_left.size() - 3, 3, "_id") == 0))
            key_hint = cond.left.column;
        else
            key_hint = cond.right.column;

        return {JoinStrategy::SortMerge,
                "both tables have >= " + std::to_string(HASH_THRESHOLD) + " rows (" +
                std::to_string(left.row_count()) + " + " +
                std::to_string(right.row_count()) + "); key '" + key_hint +
                "' suggests sorted data — sort-merge preferred over hash"};
    }

    // Default for large inputs: hash join.
    return {JoinStrategy::Hash,
            "at least one table has >= " + std::to_string(HASH_THRESHOLD) +
            " rows; hashing the smaller table (" +
            std::to_string(std::min(left.row_count(), right.row_count())) + " rows)"};
}

// ========================================================================
// ExecutionContext
// ========================================================================

int ExecutionContext::resolve(const ast::ColumnRef& ref) const {
    // Helper: check one table side.
    auto check = [&](const Table* tbl, const std::string& alias, int offset) -> int {
        if (!tbl) return -1;
        if (ref.table && *ref.table != alias && *ref.table != tbl->name)
            return -1;  // qualifier mismatch
        int idx = tbl->find_column(ref.column);
        return idx >= 0 ? offset + idx : -1;
    };

    int li = check(left_tbl, left_alias, 0);
    if (li >= 0) return li;

    if (is_join) {
        int offset = left_tbl ? static_cast<int>(left_tbl->col_count()) : 0;
        int ri = check(right_tbl, right_alias, offset);
        if (ri >= 0) return ri;
    }
    return -1;
}

Value ExecutionContext::get_value(const Row& row, const ast::ColumnRef& ref) const {
    int idx = resolve(ref);
    if (idx < 0)
        throw std::runtime_error("Unknown column '" + ref.to_string() + "'");
    return row[static_cast<std::size_t>(idx)];
}

// ========================================================================
// WHERE expression evaluators
// ========================================================================

// Single-table evaluator (unchanged from original, used by join code too).
bool eval_expr(const ast::ExprPtr& expr, const Row& row, const Table& schema) {
    switch (expr->kind) {
        case ast::Expr::Kind::And:
            return eval_expr(expr->lhs, row, schema) && eval_expr(expr->rhs, row, schema);
        case ast::Expr::Kind::Or:
            return eval_expr(expr->lhs, row, schema) || eval_expr(expr->rhs, row, schema);
        case ast::Expr::Kind::Compare: {
            auto resolve = [&](const ast::ExprPtr& e) -> Value {
                if (e->kind == ast::Expr::Kind::Literal)
                    return std::visit([](auto&& v) -> Value { return v; }, e->lit);
                int idx = schema.find_column(e->col.column);
                if (idx < 0) throw std::runtime_error("Unknown column '" + e->col.to_string() + "'");
                return row[idx];
            };
            Value lv = resolve(expr->lhs), rv = resolve(expr->rhs);
            int cmp = compare_values(lv, rv);
            switch (expr->op) {
                case ast::CompOp::EQ:  return cmp == 0;
                case ast::CompOp::NEQ: return cmp != 0;
                case ast::CompOp::LT:  return cmp <  0;
                case ast::CompOp::GT:  return cmp >  0;
                case ast::CompOp::LEQ: return cmp <= 0;
                case ast::CompOp::GEQ: return cmp >= 0;
            }
        }
        default: throw std::logic_error("eval_expr: unexpected kind");
    }
}

// Context-aware evaluator — works for both single-table and join rows.
bool eval_expr_ctx(const ast::ExprPtr& expr, const Row& row, const ExecutionContext& ctx) {
    switch (expr->kind) {
        case ast::Expr::Kind::And:
            return eval_expr_ctx(expr->lhs, row, ctx) && eval_expr_ctx(expr->rhs, row, ctx);
        case ast::Expr::Kind::Or:
            return eval_expr_ctx(expr->lhs, row, ctx) || eval_expr_ctx(expr->rhs, row, ctx);
        case ast::Expr::Kind::Compare: {
            auto resolve_val = [&](const ast::ExprPtr& e) -> Value {
                if (e->kind == ast::Expr::Kind::Literal)
                    return std::visit([](auto&& v) -> Value { return v; }, e->lit);
                return ctx.get_value(row, e->col);
            };
            Value lv = resolve_val(expr->lhs), rv = resolve_val(expr->rhs);
            int cmp = compare_values(lv, rv);
            switch (expr->op) {
                case ast::CompOp::EQ:  return cmp == 0;
                case ast::CompOp::NEQ: return cmp != 0;
                case ast::CompOp::LT:  return cmp <  0;
                case ast::CompOp::GT:  return cmp >  0;
                case ast::CompOp::LEQ: return cmp <= 0;
                case ast::CompOp::GEQ: return cmp >= 0;
            }
        }
        default: throw std::logic_error("eval_expr_ctx: unexpected kind");
    }
}

// ========================================================================
// Projection
// ========================================================================

// Build result column names — handles Star (expands), Column, Aggregate.
std::vector<std::string> output_column_names(
        const ExecutionContext& ctx,
        const std::vector<ast::OutputColumn>& cols) {
    std::vector<std::string> names;
    for (const auto& oc : cols) {
        if (oc.kind == ast::OutputColumn::Kind::Star) {
            if (ctx.left_tbl)
                for (const auto& c : ctx.left_tbl->columns) names.push_back(c);
            if (ctx.is_join && ctx.right_tbl)
                for (const auto& c : ctx.right_tbl->columns) names.push_back(c);
        } else {
            names.push_back(oc.label());
        }
    }
    return names;
}

// Project one row using the execution context.
Row project_row_ctx(const Row& row, const ExecutionContext& ctx,
                    const std::vector<ast::OutputColumn>& cols) {
    Row out;
    for (const auto& oc : cols) {
        if (oc.kind == ast::OutputColumn::Kind::Star) {
            for (const auto& v : row) out.push_back(v);
        } else if (oc.kind == ast::OutputColumn::Kind::Column) {
            out.push_back(ctx.get_value(row, oc.ref));
        } else {
            throw std::runtime_error(
                "Aggregate '" + oc.agg.label() +
                "' in SELECT without GROUP BY or aggregate context");
        }
    }
    return out;
}

// Original single-table projection (kept for compile-compatibility; delegates).
Row project_row(const Row& row, const Table& schema,
                const std::vector<ast::OutputColumn>& cols) {
    ExecutionContext ctx;
    ctx.left_tbl   = &schema;
    ctx.left_alias = schema.name;
    return project_row_ctx(row, ctx, cols);
}

// ========================================================================
// Aggregation
// ========================================================================

static Value compute_agg(const ast::AggCall& agg,
                          const std::vector<const Row*>& rows,
                          const ExecutionContext& ctx) {
    switch (agg.func) {
        case ast::AggFunc::COUNT_STAR:
            return static_cast<double>(rows.size());

        case ast::AggFunc::COUNT:
            // In our model every value is non-null, so COUNT(col) == COUNT(*).
            return static_cast<double>(rows.size());

        case ast::AggFunc::SUM: {
            double sum = 0.0;
            for (const auto* r : rows) {
                Value v = ctx.get_value(*r, *agg.col);
                if (!std::holds_alternative<double>(v))
                    throw std::runtime_error(
                        "SUM requires a numeric column, got string in '" + agg.col->to_string() + "'");
                sum += std::get<double>(v);
            }
            return sum;
        }

        case ast::AggFunc::AVG: {
            if (rows.empty()) return std::string("NULL");
            double sum = 0.0;
            for (const auto* r : rows) {
                Value v = ctx.get_value(*r, *agg.col);
                if (!std::holds_alternative<double>(v))
                    throw std::runtime_error(
                        "AVG requires a numeric column, got string in '" + agg.col->to_string() + "'");
                sum += std::get<double>(v);
            }
            return sum / static_cast<double>(rows.size());
        }

        case ast::AggFunc::MIN: {
            if (rows.empty()) return std::string("NULL");
            Value best = ctx.get_value(*rows[0], *agg.col);
            for (std::size_t i = 1; i < rows.size(); ++i) {
                Value v = ctx.get_value(*rows[i], *agg.col);
                if (compare_values(v, best) < 0) best = v;
            }
            return best;
        }

        case ast::AggFunc::MAX: {
            if (rows.empty()) return std::string("NULL");
            Value best = ctx.get_value(*rows[0], *agg.col);
            for (std::size_t i = 1; i < rows.size(); ++i) {
                Value v = ctx.get_value(*rows[i], *agg.col);
                if (compare_values(v, best) > 0) best = v;
            }
            return best;
        }
    }
    return std::string("NULL");
}

Table apply_aggregation(const std::vector<Row>& rows,
                        const ExecutionContext& ctx,
                        const std::vector<ast::OutputColumn>& out_cols,
                        const std::optional<ast::ColumnRef>& group_by) {
    // Validate: star not allowed with aggregation.
    for (const auto& oc : out_cols)
        if (oc.kind == ast::OutputColumn::Kind::Star)
            throw std::runtime_error("SELECT * cannot be combined with aggregate functions");

    Table result;
    result.name = "result";
    for (const auto& oc : out_cols)
        result.columns.push_back(oc.label());

    // Helper: compute one output row from a group of row pointers.
    auto emit_row = [&](const std::vector<const Row*>& grp) {
        Row out;
        out.reserve(out_cols.size());
        for (const auto& oc : out_cols) {
            if (oc.kind == ast::OutputColumn::Kind::Aggregate) {
                out.push_back(compute_agg(oc.agg, grp, ctx));
            } else {
                // Plain column: take value from first row in group.
                if (grp.empty())
                    out.push_back(std::string("NULL"));
                else
                    out.push_back(ctx.get_value(*grp[0], oc.ref));
            }
        }
        result.rows.push_back(std::move(out));
    };

    if (group_by) {
        // Bucket rows, preserving first-seen order of group keys.
        std::vector<std::string>                             key_order;
        std::unordered_map<std::string, std::vector<const Row*>> groups;
        groups.reserve(rows.size());

        for (const auto& row : rows) {
            std::string key = value_to_string(ctx.get_value(row, *group_by));
            auto [it, inserted] = groups.emplace(key, std::vector<const Row*>{});
            if (inserted) key_order.push_back(key);
            it->second.push_back(&row);
        }

        for (const auto& key : key_order)
            emit_row(groups.at(key));
    } else {
        // No GROUP BY — collapse all rows into one.
        std::vector<const Row*> all;
        all.reserve(rows.size());
        for (const auto& r : rows) all.push_back(&r);
        emit_row(all);
    }

    return result;
}

// ========================================================================
// ORDER BY
// ========================================================================

void apply_order_by(Table& result, const ast::OutputColumn& order_col, bool asc) {
    if (order_col.kind == ast::OutputColumn::Kind::Star)
        throw std::runtime_error("Cannot ORDER BY *");

    std::string col_name = order_col.label();
    int idx = result.find_column(col_name);
    if (idx < 0)
        throw std::runtime_error(
            "ORDER BY column '" + col_name + "' not found in result set");

    std::stable_sort(result.rows.begin(), result.rows.end(),
        [&, idx](const Row& a, const Row& b) {
            int cmp = compare_values(a[idx], b[idx]);
            return asc ? cmp < 0 : cmp > 0;
        });
}

// ========================================================================
// LIMIT
// ========================================================================

void apply_limit(Table& result, std::size_t n) {
    if (result.rows.size() > n)
        result.rows.resize(n);
}

// ========================================================================
// JOIN helpers
// ========================================================================

static int resolve_join_key(const ast::ColumnRef& ref,
                             const Table& tbl, const std::string& alias) {
    if (ref.table && *ref.table != alias && *ref.table != tbl.name)
        return -1;
    return tbl.find_column(ref.column);
}

static std::vector<Row> nested_loop_join(const Table& left, const Table& right,
                                          const ast::JoinCondition& cond,
                                          const std::string& la, const std::string& ra) {
    int li = resolve_join_key(cond.left, left, la);
    int ri = resolve_join_key(cond.right, right, ra);
    if (li < 0 || ri < 0) {
        // Try swapped orientation
        li = resolve_join_key(cond.right, left, la);
        ri = resolve_join_key(cond.left,  right, ra);
    }
    if (li < 0 || ri < 0)
        throw std::runtime_error("JOIN ON columns not found");

    std::vector<Row> result;
    for (const auto& lr : left.rows)
        for (const auto& rr : right.rows)
            if (compare_values(lr[li], rr[ri]) == 0) {
                Row combined = lr;
                combined.insert(combined.end(), rr.begin(), rr.end());
                result.push_back(std::move(combined));
            }
    return result;
}

static std::vector<Row> hash_join(const Table& left, const Table& right,
                                   const ast::JoinCondition& cond,
                                   const std::string& la, const std::string& ra) {
    int li = resolve_join_key(cond.left, left, la);
    int ri = resolve_join_key(cond.right, right, ra);
    if (li < 0 || ri < 0) {
        li = resolve_join_key(cond.right, left, la);
        ri = resolve_join_key(cond.left,  right, ra);
    }
    if (li < 0 || ri < 0)
        throw std::runtime_error("JOIN ON columns not found");

    bool build_right = right.row_count() <= left.row_count();
    const Table& build_tbl = build_right ? right : left;
    const Table& probe_tbl = build_right ? left  : right;
    int          bk        = build_right ? ri : li;
    int          pk        = build_right ? li : ri;

    std::unordered_multimap<std::string, const Row*> ht;
    ht.reserve(build_tbl.row_count() * 2);
    for (const auto& row : build_tbl.rows)
        ht.emplace(value_to_string(row[bk]), &row);

    std::vector<Row> result;
    for (const auto& prow : probe_tbl.rows) {
        auto range = ht.equal_range(value_to_string(prow[pk]));
        for (auto it = range.first; it != range.second; ++it) {
            Row combined;
            if (build_right) {
                combined = prow;
                combined.insert(combined.end(), it->second->begin(), it->second->end());
            } else {
                combined = *it->second;
                combined.insert(combined.end(), prow.begin(), prow.end());
            }
            result.push_back(std::move(combined));
        }
    }
    return result;
}

static std::vector<Row> sort_merge_join(const Table& left, const Table& right,
                                         const ast::JoinCondition& cond,
                                         const std::string& la, const std::string& ra) {
    int li = resolve_join_key(cond.left, left, la);
    int ri = resolve_join_key(cond.right, right, ra);
    if (li < 0 || ri < 0) {
        li = resolve_join_key(cond.right, left, la);
        ri = resolve_join_key(cond.left,  right, ra);
    }
    if (li < 0 || ri < 0)
        throw std::runtime_error("JOIN ON columns not found");

    // Build sorted index arrays — avoids copying rows during sort.
    std::vector<std::size_t> l_ord(left.row_count()), r_ord(right.row_count());
    std::iota(l_ord.begin(), l_ord.end(), 0);
    std::iota(r_ord.begin(), r_ord.end(), 0);
    std::sort(l_ord.begin(), l_ord.end(), [&](std::size_t a, std::size_t b) {
        return compare_values(left.rows[a][li], left.rows[b][li]) < 0;
    });
    std::sort(r_ord.begin(), r_ord.end(), [&](std::size_t a, std::size_t b) {
        return compare_values(right.rows[a][ri], right.rows[b][ri]) < 0;
    });

    // Two-pointer sweep.
    std::vector<Row> result;
    std::size_t i = 0, j = 0;
    while (i < l_ord.size() && j < r_ord.size()) {
        const Value& lv = left.rows[l_ord[i]][li];
        const Value& rv = right.rows[r_ord[j]][ri];
        int cmp = compare_values(lv, rv);

        if (cmp < 0) { ++i; continue; }
        if (cmp > 0) { ++j; continue; }

        // Keys are equal — find the full equal-key run on each side.
        std::size_t i_end = i + 1;
        while (i_end < l_ord.size() &&
               compare_values(left.rows[l_ord[i_end]][li], lv) == 0)
            ++i_end;

        std::size_t j_end = j + 1;
        while (j_end < r_ord.size() &&
               compare_values(right.rows[r_ord[j_end]][ri], rv) == 0)
            ++j_end;

        // Emit the cross product of the two runs.
        for (std::size_t ii = i; ii < i_end; ++ii) {
            const Row& lr = left.rows[l_ord[ii]];
            for (std::size_t jj = j; jj < j_end; ++jj) {
                Row combined = lr;
                const Row& rr = right.rows[r_ord[jj]];
                combined.insert(combined.end(), rr.begin(), rr.end());
                result.push_back(std::move(combined));
            }
        }

        i = i_end;
        j = j_end;
    }

    return result;
}

// ========================================================================
// Main execute function
// ========================================================================

Table execute(const ast::SelectStmt& stmt,
              const std::unordered_map<std::string, Table>& db) {
    // ---- Step 1: locate tables, build context --------------------------
    auto it = db.find(stmt.from_table);
    if (it == db.end())
        throw std::runtime_error("Unknown table '" + stmt.from_table + "'");
    const Table& from_tbl   = it->second;
    std::string  from_alias = stmt.from_alias.value_or(stmt.from_table);

    ExecutionContext ctx;
    ctx.left_tbl   = &from_tbl;
    ctx.left_alias = from_alias;

    // ---- Step 2: produce filtered rows (full-width) --------------------
    std::vector<Row> filtered_rows;

    if (!stmt.join_table) {
        for (const auto& row : from_tbl.rows)
            if (!stmt.where_expr || eval_expr_ctx(stmt.where_expr, row, ctx))
                filtered_rows.push_back(row);
    } else {
        auto jt = db.find(*stmt.join_table);
        if (jt == db.end())
            throw std::runtime_error("Unknown table '" + *stmt.join_table + "'");
        const Table& join_tbl   = jt->second;
        std::string  join_alias = stmt.join_alias.value_or(*stmt.join_table);

        ctx.is_join    = true;
        ctx.right_tbl  = &join_tbl;
        ctx.right_alias = join_alias;

        JoinPlan plan = choose_join_strategy(from_tbl, join_tbl, *stmt.join_on);
        const char* strategy_name =
            plan.strategy == JoinStrategy::NestedLoop ? "NESTED LOOP JOIN" :
            plan.strategy == JoinStrategy::Hash       ? "HASH JOIN"        :
                                                        "SORT-MERGE JOIN";
        std::cout << "[optimizer] Join strategy: "
                  << strategy_name << " — " << plan.reason << "\n";

        std::vector<Row> joined_rows;
        switch (plan.strategy) {
            case JoinStrategy::NestedLoop:
                joined_rows = nested_loop_join(from_tbl, join_tbl, *stmt.join_on, from_alias, join_alias);
                break;
            case JoinStrategy::Hash:
                joined_rows = hash_join(from_tbl, join_tbl, *stmt.join_on, from_alias, join_alias);
                break;
            case JoinStrategy::SortMerge:
                joined_rows = sort_merge_join(from_tbl, join_tbl, *stmt.join_on, from_alias, join_alias);
                break;
        }

        for (const auto& row : joined_rows)
            if (!stmt.where_expr || eval_expr_ctx(stmt.where_expr, row, ctx))
                filtered_rows.push_back(row);
    }

    // ---- Step 3: aggregate or project ----------------------------------
    bool has_agg = false;
    for (const auto& oc : stmt.select_cols)
        if (oc.kind == ast::OutputColumn::Kind::Aggregate) { has_agg = true; break; }

    Table result;
    result.name = "result";

    if (has_agg || stmt.group_by) {
        result = apply_aggregation(filtered_rows, ctx, stmt.select_cols, stmt.group_by);
    } else {
        result.columns = output_column_names(ctx, stmt.select_cols);
        for (const auto& row : filtered_rows)
            result.rows.push_back(project_row_ctx(row, ctx, stmt.select_cols));
    }

    // ---- Step 4: ORDER BY ----------------------------------------------
    if (stmt.order_by)
        apply_order_by(result, *stmt.order_by, stmt.order_by_asc);

    // ---- Step 5: LIMIT -------------------------------------------------
    if (stmt.limit)
        apply_limit(result, *stmt.limit);

    return result;
}
