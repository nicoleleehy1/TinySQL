#include "csv_reader.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

// Parse a single CSV line respecting double-quote escaping.
static std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                // "" inside quotes is an escaped quote
                if (i + 1 < line.size() && line[i+1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields.push_back(field); // last field (even if empty)
    return fields;
}

// Trim leading/trailing ASCII whitespace.
static std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Try to interpret a string as a double. Returns nullopt on failure.
static std::optional<double> try_parse_double(const std::string& s) {
    if (s.empty()) return std::nullopt;
    std::size_t idx = 0;
    try {
        double d = std::stod(s, &idx);
        if (idx == s.size()) return d;   // whole string was consumed
    } catch (...) {}
    return std::nullopt;
}

Table load_csv(const std::string& path, const std::string& table_name) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);

    Table table;
    table.name = table_name;

    std::string line;
    bool header_done = false;
    int  line_num    = 0;

    while (std::getline(file, line)) {
        ++line_num;
        // Skip empty lines and comment lines starting with '#'
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        auto fields = parse_csv_line(line);

        if (!header_done) {
            for (auto& f : fields)
                table.columns.push_back(trim(f));
            header_done = true;
            continue;
        }

        if (fields.size() != table.columns.size())
            throw std::runtime_error(
                path + ":" + std::to_string(line_num) +
                ": expected " + std::to_string(table.columns.size()) +
                " columns, got " + std::to_string(fields.size()));

        Row row;
        row.reserve(fields.size());
        for (auto& f : fields) {
            std::string val = trim(f);
            if (auto d = try_parse_double(val))
                row.emplace_back(*d);
            else
                row.emplace_back(val);
        }
        table.rows.push_back(std::move(row));
    }

    if (!header_done)
        throw std::runtime_error(path + ": file is empty");

    return table;
}
