# TinySQL

A lightweight C++ SQL engine with a cost-based optimizer that runs SELECT, JOIN, and WHERE queries directly against CSV files, built from scratch.

---

## Features

- **SQL parsing** — recursive descent parser supporting SELECT, FROM, WHERE, JOIN, ORDER BY, LIMIT, and GROUP BY
- **CSV-native** — loads any CSV with a header row, no schema file needed, types inferred automatically
- **Cost-based optimizer** — chooses between nested loop join, hash join, and sort-merge join based on table sizes
- **Aggregates** — COUNT, SUM, AVG, MIN, MAX with optional GROUP BY
- **REPL + CLI** — interactive shell for exploration or single-query mode for scripting
- **No dependencies** — pure C++17, stdlib only

---

## Build

**Prerequisites:** clang++ (or g++) and CMake 3.x+

```bash
# macOS
xcode-select --install
brew install cmake
```

```bash
git clone https://github.com/nicoleleehy1/TinySQL.git
cd TinySQL
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Usage

### Interactive REPL

```bash
./build/mini_sql data/employees.csv data/departments.csv
```

```
mini-sql> .tables
mini-sql> .schema employees
mini-sql> SELECT name, salary FROM employees WHERE salary > 70000 ORDER BY salary DESC
mini-sql> .quit
```

### Single query (CLI)

```bash
./build/mini_sql data/employees.csv data/departments.csv -- \
  "SELECT e.name, e.salary, d.name FROM employees e JOIN departments d ON e.department_id = d.id WHERE e.salary > 70000"
```

### REPL commands

| Command | Description |
|---|---|
| `.tables` | List loaded tables with row and column counts |
| `.schema <table>` | Show column names and inferred types |
| `.quit` | Exit the REPL |

---

## SQL support

```sql
-- Projection and filtering
SELECT name, salary FROM employees WHERE salary > 70000

-- Joins
SELECT e.name, d.name FROM employees e
  JOIN departments d ON e.department_id = d.id

-- Aggregates
SELECT department_id, COUNT(*), AVG(salary)
  FROM employees GROUP BY department_id

-- Sorting and limiting
SELECT name, salary FROM employees ORDER BY salary DESC LIMIT 5
```

Supported operators in WHERE: `=`, `!=`, `<`, `>`, `<=`, `>=`, `AND`, `OR`

---

## Project structure

```
TinySQL/
├── CMakeLists.txt
├── queries.sql              — 10 example queries
├── data/
│   ├── employees.csv
│   ├── departments.csv
│   └── orders.csv
├── src/
│   ├── main.cpp             — CLI, REPL, pretty-printer
│   ├── table.hpp            — Value, Row, Table types
│   ├── tokenizer.hpp/.cpp   — Lexer
│   ├── ast.hpp              — AST nodes
│   ├── parser.hpp/.cpp      — Recursive descent parser
│   ├── csv_reader.hpp/.cpp  — CSV loader
│   └── executor.hpp/.cpp    — Query execution and optimizer
└── docs/                    — React docs site
```

---

## How the optimizer works

The optimizer estimates cost using row counts at load time and selects a join strategy before execution:

- **Nested loop join** — used when both tables are small (< 100 rows). Simple and low overhead.
- **Hash join** — used for larger tables. Builds a hash map on the smaller table's join key, then probes with the larger table. O(n + m) average case.
- **Sort-merge join** — used when join columns are likely sorted (e.g. columns named `id` or ending in `_id`). Sorts both sides then merges linearly.

The chosen strategy and reasoning are printed at query time.

---

## License

MIT
