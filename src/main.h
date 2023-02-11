#ifndef MAIN_H
#define MAIN_H

#include "db/layout.h"
#include <stdio.h>
#include <sys/types.h>

typedef struct {
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum {
    META_CMD_SUCCESS,
    META_CMD_UNRECOGNIZED_COMMAND,
} MetaCmdResult;

typedef enum {
    STMT_INSERT,
    STMT_SELECT,
} StatementType;

typedef struct {
    StatementType type;
    Row row;
} Statement;

typedef enum {
    PREP_SUCCESS,
    PREP_UNRECOGNIZED_STATEMENT,
    PREP_SYNTAX_ERROR,
    PREP_NEGATIVE_ROW_ID,
    PREP_STR_TOO_LONG,
} PrepareResult;

typedef enum {
    EXEC_RES_SUCCESS,
    EXEC_RES_TABLE_FULL,
} ExecuteResult;

InputBuffer *new_input_buffer();
void display_prompt();
void read_input(InputBuffer *buffer);
MetaCmdResult exec_meta_cmd(InputBuffer *buffer, Table *table);
PrepareResult prepare_statement(InputBuffer *buffer, Statement *statement);
ExecuteResult exec_stmt_insert(Statement *statement, Table *table);
ExecuteResult exec_stmt_select(Table *table);
ExecuteResult exec_statement(Statement *statement, Table *table);
void close_input_buffer();
void show_row(Row *row);

#endif
