#pragma once
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>
#include <algorithm>

// A cell value is either a UTF-8 string or a double.
using Value = std::variant<std::string, double>;
using Row   = std::vector<Value>;

struct Table {
    std::string              name;
    std::vector<std::string> columns;  // ordered column names
    std::vector<Row>         rows;

    std::size_t row_count() const { return rows.size(); }
    std::size_t col_count() const { return columns.size(); }

    // Returns -1 when not found.
    int find_column(const std::string& col) const {
        for (int i = 0; i < static_cast<int>(columns.size()); ++i)
            if (columns[i] == col) return i;
        return -1;
    }

    int require_column(const std::string& col) const {
        int idx = find_column(col);
        if (idx == -1)
            throw std::runtime_error("Unknown column '" + col + "' in table '" + name + "'");
        return idx;
    }
};

// Helpers ----------------------------------------------------------------

inline std::string value_to_string(const Value& v) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) return arg;
        else {
            // Trim trailing zeros for integers stored as double.
            if (arg == static_cast<double>(static_cast<long long>(arg)))
                return std::to_string(static_cast<long long>(arg));
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.6g", arg);
            return buf;
        }
    }, v);
}

inline bool value_is_double(const Value& v) {
    return std::holds_alternative<double>(v);
}

// Compare two values. Strings compare lexicographically; doubles numerically.
// Mixed comparisons convert both sides to string.
inline int compare_values(const Value& a, const Value& b) {
    if (value_is_double(a) && value_is_double(b)) {
        double da = std::get<double>(a), db = std::get<double>(b);
        if (da < db) return -1;
        if (da > db) return  1;
        return 0;
    }
    std::string sa = value_to_string(a), sb = value_to_string(b);
    if (sa < sb) return -1;
    if (sa > sb) return  1;
    return 0;
}
