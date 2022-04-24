#include "main.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main() {
    Table* table = new_table();
    InputBuffer* input_buffer = new_input_buffer();

    while (true) {
        display_prompt();
        read_input(input_buffer);

        if (input_buffer->buffer[0] == '.') {
            switch (exec_meta_cmd(input_buffer)) {
                case (META_CMD_SUCCESS):
                    continue;
                case (META_CMD_UNRECOGNIZED_COMMAND):
                    printf("unrecognized command: '%s'\n", input_buffer->buffer);
                    continue;
            }
        }

        Statement statement;

        switch (prepare_statement(input_buffer, &statement)) {
            case (PREP_SUCCESS):
                break;
            case (PREP_UNRECOGNIZED_STATEMENT):
                printf("unrecognized keyword at start of '%s'\n", input_buffer->buffer);
                continue;
            case (PREP_SYNTAX_ERROR):
                printf("syntax error, could not parse statement\n");
                continue;
        }

        switch (exec_statement(&statement, table)) {
            case (EXEC_RES_SUCCESS):
                printf("executed\n");
                break;
            case (EXEC_RES_TABLE_FULL):
                printf("ERR: full table\n");
                break;
        }
    }
}

InputBuffer*
new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*) malloc(sizeof(InputBuffer));

    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

void
display_prompt() {
    printf("db: ");
}

void
read_input(InputBuffer* input_buffer) {
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0) {
        printf("error reading input\n");
        exit(EXIT_FAILURE);
    }

    input_buffer->buffer[bytes_read - 1] = 0;
    input_buffer->input_length = bytes_read - 1;
}

void
close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

MetaCmdResult
exec_meta_cmd(InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_input_buffer(input_buffer);
        exit(EXIT_SUCCESS);
    } else {
        return META_CMD_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult
prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        statement->type = STMT_INSERT;

        int args_assigned = sscanf(input_buffer->buffer,
                                   "insert %d %s %s",
                                   &(statement->row.id),
                                   statement->row.name,
                                   statement->row.email);

        if (args_assigned < 3) {
            return PREP_SYNTAX_ERROR;
        }

        return PREP_SUCCESS;
    } else if (strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STMT_SELECT;
        return PREP_SUCCESS;
    } else {
        return PREP_UNRECOGNIZED_STATEMENT;
    }
}

ExecuteResult
exec_stmt_insert(Statement* statement, Table* table) {
    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXEC_RES_TABLE_FULL;
    }

    Row* row = &(statement->row);
    void* slot = define_row_slot(table, table->num_rows);

    row_serialize(row, slot);
    table->num_rows++;

    return EXEC_RES_SUCCESS;
}

ExecuteResult
exec_stmt_select(Table* table) {
    Row row;

    for (u_int32_t i = 0; i < table->num_rows; i++) {
        row_deserialize(define_row_slot(table, i), &row);
        show_row(&row);
    }

    return EXEC_RES_SUCCESS;
}

ExecuteResult
exec_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case (STMT_INSERT):
            return exec_stmt_insert(statement, table);
        case (STMT_SELECT):
            return exec_stmt_select(table);
    }
}

void
row_serialize(Row* source, void* destination) {
    memcpy(destination + SCHEMA_ID_OFFSET, &(source->id), SCHEMA_ID_SIZE);
    memcpy(destination + SCHEMA_NAME_OFFSET, &(source->name), SCHEMA_NAME_SIZE);
    memcpy(destination + SCHEMA_EMAIL_OFFSET, &(source->email), SCHEMA_EMAIL_SIZE);
}

void
row_deserialize(void* source, Row* destination) {
    memcpy(&(destination->id), source + SCHEMA_ID_OFFSET, SCHEMA_ID_SIZE);
    memcpy(&(destination->name), source + SCHEMA_NAME_OFFSET, SCHEMA_NAME_SIZE);
    memcpy(&(destination->email), source + SCHEMA_EMAIL_OFFSET, SCHEMA_EMAIL_SIZE);
}

void*
define_row_slot(Table* table, u_int32_t row_number) {
    u_int32_t page_number = row_number / ROWS_PER_PAGE;
    void* page = table->pages[page_number];

    if (page == NULL) {
        page = table->pages[page_number] = malloc(PAGE_SIZE);
    }

    // number of pages already allocated
    u_int32_t row_offset = row_number % ROWS_PER_PAGE;

    // position available to alloc
    u_int32_t byte_offset = row_offset * ROW_SIZE;

    return page + byte_offset;
}

Table*
new_table() {
    Table* table = (Table*) malloc(sizeof(Table));
    table->num_rows = 0;

    for (u_int32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }

    return table;
}

void
free_table(Table* table) {
    for (u_int32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        free(table->pages[i]);
    }
    free(table);
}

void
show_row(Row* row) {
    printf("-- %d %s %s\n", row->id, row->name, row->email);
}
