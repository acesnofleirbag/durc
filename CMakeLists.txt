cmake_minimum_required(VERSION 3.10)

set(PROJECT "database")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_C_STANDARD 11)

project(
    $(PROJECT)    
    VERSION 1.0.0
    DESCRIPTION "database structure from scratch tutorial"
    LANGUAGES C
)

add_executable(
    ${PROJECT}
    src/main.c
)

# REF: <https://caiorss.github.io/C-Cpp-Notes/compiler-flags-options.html>
target_compile_options(
    ${PROJECT}
    PUBLIC
    -Wall
    -Werror
    -Wextra
    # -pedantic # necessary to remove because pointer arithmetic on schema
)
