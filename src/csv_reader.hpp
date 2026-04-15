#pragma once
#include "table.hpp"
#include <string>

// Loads a CSV file into a Table.
// - First row is treated as the header (column names).
// - Each subsequent row is a data row; values that parse as a finite double
//   are stored as double, otherwise as std::string.
// Throws std::runtime_error on I/O or format errors.
Table load_csv(const std::string& path, const std::string& table_name);
