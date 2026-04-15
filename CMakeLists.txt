cmake_minimum_required(VERSION 3.16)
project(mini_sql CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(mini_sql
    src/main.cpp
    src/tokenizer.cpp
    src/parser.cpp
    src/csv_reader.cpp
    src/executor.cpp
)

target_include_directories(mini_sql PRIVATE src)
