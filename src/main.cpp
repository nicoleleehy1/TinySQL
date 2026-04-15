#include "tokenizer.hpp"
#include "parser.hpp"
#include "csv_reader.hpp"
#include "executor.hpp"
#include "table.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <stdexcept>

// ========================================================================
// Pretty printer
// ========================================================================

static void print_table(const Table& t) {
    if (t.columns.empty()) { std::cout << "(no columns)\n"; return; }

    std::vector<std::size_t> widths;
    for (const auto& c : t.columns) widths.push_back(c.size());

    for (const auto& row : t.rows)
        for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i)
            widths[i] = std::max(widths[i], value_to_string(row[i]).size());

    auto separator = [&]() {
        std::cout << '+';
        for (auto w : widths) std::cout << std::string(w + 2, '-') << '+';
        std::cout << '\n';
    };

    separator();
    std::cout << '|';
    for (std::size_t i = 0; i < t.columns.size(); ++i)
        std::cout << ' ' << std::left << std::setw(widths[i]) << t.columns[i] << " |";
    std::cout << '\n';
    separator();

    for (const auto& row : t.rows) {
        std::cout << '|';
        for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i)
            std::cout << ' ' << std::left << std::setw(widths[i]) << value_to_string(row[i]) << " |";
        std::cout << '\n';
    }
    separator();
    std::cout << t.row_count() << " row(s)\n";
}

// ========================================================================
// Query runner — shared by both CLI and REPL modes
// ========================================================================

static void run_query(const std::string& sql,
                      const std::unordered_map<std::string, Table>& db) {
    Tokenizer tok(sql);
    auto tokens = tok.tokenize();
    Parser parser(std::move(tokens));
    auto stmt = parser.parse();
    Table result = execute(stmt, db);
    std::cout << '\n';
    print_table(result);
}

// ========================================================================
// REPL helpers
// ========================================================================

static std::string trim(const std::string& s) {
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Infer the display type of a column by scanning all its values.
// Returns "DOUBLE" if every present value is numeric, otherwise "TEXT".
static std::string infer_column_type(const Table& t, std::size_t col_idx) {
    if (t.rows.empty()) return "UNKNOWN";
    for (const auto& row : t.rows)
        if (!value_is_double(row[col_idx])) return "TEXT";
    return "DOUBLE";
}

// Handle dot commands: return true if the line was a dot command.
static bool handle_dot_command(const std::string& line,
                                const std::unordered_map<std::string, Table>& db,
                                bool& should_quit) {
    if (line.empty() || line[0] != '.') return false;

    // Tokenise the dot command by whitespace.
    std::istringstream iss(line);
    std::vector<std::string> parts;
    std::string tok;
    while (iss >> tok) parts.push_back(tok);

    if (parts[0] == ".quit" || parts[0] == ".exit") {
        should_quit = true;
        return true;
    }

    if (parts[0] == ".tables") {
        if (db.empty()) {
            std::cout << "(no tables loaded)\n";
        } else {
            // Collect and sort names for stable output.
            std::vector<std::string> names;
            names.reserve(db.size());
            for (const auto& [k, _] : db) names.push_back(k);
            std::sort(names.begin(), names.end());

            std::size_t max_name = 5; // minimum header width "table"
            for (const auto& n : names) max_name = std::max(max_name, n.size());

            std::cout << std::left
                      << std::setw(max_name + 2) << "table"
                      << std::setw(8)  << "rows"
                      << "columns\n";
            std::cout << std::string(max_name + 2 + 8 + 7, '-') << '\n';
            for (const auto& n : names) {
                const Table& t = db.at(n);
                std::cout << std::setw(max_name + 2) << n
                          << std::setw(8)  << t.row_count()
                          << t.col_count() << '\n';
            }
        }
        return true;
    }

    if (parts[0] == ".schema") {
        if (parts.size() < 2) {
            std::cerr << "Usage: .schema <table_name>\n";
            return true;
        }
        auto it = db.find(parts[1]);
        if (it == db.end()) {
            std::cerr << "Unknown table '" << parts[1] << "'\n";
            return true;
        }
        const Table& t = it->second;
        std::cout << "Table: " << t.name << "  ("
                  << t.row_count() << " rows, "
                  << t.col_count() << " columns)\n";

        // Width of the widest column name.
        std::size_t max_col = 6; // "column"
        for (const auto& c : t.columns) max_col = std::max(max_col, c.size());

        std::cout << std::left
                  << std::setw(max_col + 2) << "column"
                  << "type\n";
        std::cout << std::string(max_col + 2 + 8, '-') << '\n';
        for (std::size_t i = 0; i < t.columns.size(); ++i) {
            std::cout << std::setw(max_col + 2) << t.columns[i]
                      << infer_column_type(t, i) << '\n';
        }
        return true;
    }

    // Unknown dot command.
    std::cerr << "Unknown command '" << parts[0]
              << "'. Type .quit to exit.\n";
    return true;
}

// ========================================================================
// REPL
// ========================================================================

static void run_repl(const std::unordered_map<std::string, Table>& db) {
    std::cout << "mini-sql — " << db.size() << " table(s) loaded. "
              << "Type .tables, .schema <t>, .quit, or a SQL query.\n";

    bool should_quit = false;
    std::string line;

    while (!should_quit) {
        std::cout << "mini-sql> " << std::flush;

        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl-D / piped input exhausted)
            std::cout << '\n';
            break;
        }

        std::string input = trim(line);
        if (input.empty()) continue;

        if (handle_dot_command(input, db, should_quit)) continue;

        // SQL query
        try {
            run_query(input, db);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
        }
    }
}

// ========================================================================
// Usage
// ========================================================================

static void print_usage(const char* prog) {
    std::cerr <<
        "Usage:\n"
        "  " << prog << " <csv>[:<alias>] ...                       — interactive REPL\n"
        "  " << prog << " <csv>[:<alias>] ... -- <SQL query>        — single query\n"
        "  " << prog << " <csv>[:<alias>] ... -f <query_file>       — query from file\n\n"
        "Examples:\n"
        "  " << prog << " data/employees.csv data/departments.csv\n"
        "  " << prog << " data/employees.csv -- "
             "\"SELECT name, salary FROM employees WHERE salary > 50000\"\n";
}

// ========================================================================
// Entry point
// ========================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) { print_usage(argv[0]); return 1; }

    std::unordered_map<std::string, Table> db;
    std::string query;
    bool found_sep  = false;
    bool from_file  = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--") { found_sep = true; continue; }

        if (arg == "-f") {
            from_file = true;
            if (i + 1 >= argc) { std::cerr << "Error: -f requires a filename\n"; return 1; }
            ++i;
            std::ifstream qf(argv[i]);
            if (!qf) { std::cerr << "Cannot open query file: " << argv[i] << "\n"; return 1; }
            std::ostringstream ss;
            ss << qf.rdbuf();
            query = ss.str();
            continue;
        }

        if (found_sep || from_file) {
            if (!query.empty()) query += ' ';
            query += arg;
            continue;
        }

        // CSV file argument, optional :alias suffix.
        std::string path  = arg;
        std::string alias = arg;

        auto colon = arg.rfind(':');
        if (colon != std::string::npos && colon > 1) {
            path  = arg.substr(0, colon);
            alias = arg.substr(colon + 1);
        } else {
            auto slash = alias.find_last_of("/\\");
            if (slash != std::string::npos) alias = alias.substr(slash + 1);
            auto dot = alias.rfind('.');
            if (dot != std::string::npos) alias = alias.substr(0, dot);
        }

        try {
            Table t = load_csv(path, alias);
            std::cout << "[loader] Loaded '" << path << "' as table '" << alias
                      << "' (" << t.row_count() << " rows, "
                      << t.col_count() << " columns)\n";
            db[alias] = std::move(t);
        } catch (const std::exception& e) {
            std::cerr << "Error loading CSV: " << e.what() << "\n";
            return 1;
        }
    }

    if (db.empty()) { print_usage(argv[0]); return 1; }

    // ---- Single-query mode (-- or -f provided) ----
    if (found_sep || from_file) {
        query = trim(query);
        if (query.empty()) { std::cerr << "Error: empty query\n"; return 1; }
        try {
            run_query(query, db);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    // ---- REPL mode (no query on the command line) ----
    run_repl(db);
    return 0;
}
