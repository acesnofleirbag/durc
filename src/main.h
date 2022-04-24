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
} PrepareResult;

typedef enum {
    EXEC_RES_SUCCESS,
    EXEC_RES_TABLE_FULL,
} ExecuteResult;

InputBuffer *new_input_buffer();
void display_prompt();
void read_input(InputBuffer *);
MetaCmdResult exec_meta_cmd(InputBuffer *);
PrepareResult prepare_statement(InputBuffer *, Statement *);
ExecuteResult exec_stmt_insert(Statement *, Table *);
ExecuteResult exec_stmt_select(Table *);
ExecuteResult exec_statement(Statement *, Table *);
void close_input_buffer();
void show_row(Row *);

#endif
