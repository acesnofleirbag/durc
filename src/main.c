#include "main.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char** argv) {
    if (argc < 2) {
        printf("must supply a database filename\n");
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    Table* table = db_open(filename);
    InputBuffer* input_buffer = new_input_buffer();

    while (true) {
        display_prompt();
        read_input(input_buffer);

        if (input_buffer->buffer[0] == '.') {
            switch (exec_meta_cmd(input_buffer, table)) {
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
            case (PREP_STR_TOO_LONG):
                printf("string is too long to persistency\n");
                continue;
            case (PREP_NEGATIVE_ROW_ID):
                printf("row id must be a positive value\n");
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
        // printf("error reading input\n");
        exit(EXIT_FAILURE);
    }

    input_buffer->buffer[bytes_read - 1] = 0;
    input_buffer->input_length = bytes_read - 1;
}

MetaCmdResult
exec_meta_cmd(InputBuffer* input_buffer, Table* table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        db_close(table);
        exit(EXIT_SUCCESS);
    } else {
        return META_CMD_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult
prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        statement->type = STMT_INSERT;

        __attribute__((unused)) char* keyword = strtok(input_buffer->buffer, " ");
        char* row_id_str = strtok(NULL, " ");
        char* username = strtok(NULL, " ");
        char* email = strtok(NULL, " ");

        if (row_id_str == NULL || username == NULL || email == NULL) {
            return PREP_SYNTAX_ERROR;
        }

        int row_id = atoi(row_id_str);

        if (row_id < 0) {
            return PREP_NEGATIVE_ROW_ID;
        }

        if (strlen(username) > EMAIL_SIZE || strlen(email) > NAME_SIZE) {
            return PREP_STR_TOO_LONG;
        }

        statement->row.id = row_id;
        strcpy(statement->row.name, username);
        strcpy(statement->row.email, email);

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
    void* page = get_page(table->pager, page_number);

    // number of pages already allocated
    u_int32_t row_offset = row_number % ROWS_PER_PAGE;

    // position available to alloc
    u_int32_t byte_offset = row_offset * ROW_SIZE;

    return page + byte_offset;
}

void
show_row(Row* row) {
    printf("-- %d %s %s\n", row->id, row->name, row->email);
}

void*
get_page(Pager* pager, uint32_t page_number) {
    if (page_number > TABLE_MAX_PAGES) {
        printf("tried to fetch page number out of bounds %d > %d\n", page_number, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_number] == NULL) {
        // cache miss. Allocate memory and load from file
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length / PAGE_SIZE;

        // we might save a partial page at the end of the file
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_number <= num_pages) {
            lseek(pager->fd, page_number * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->fd, page, PAGE_SIZE);

            if (bytes_read == -1) {
                printf("error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[page_number] = page;
    }

    return pager->pages[page_number];
}

Pager*
pager_open(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

    if (fd == -1) {
        printf("unable to open file\n");
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);

    Pager* pager = malloc(sizeof(Pager));
    pager->fd = fd;
    pager->file_length = file_length;

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

void
pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
    if (pager->pages[page_num] == NULL) {
        printf("tried to flush a null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        printf("error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->fd, pager->pages[page_num], size);

    if (bytes_written == -1) {
        printf("error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

Table*
db_open(const char* filename) {
    Pager* pager = pager_open(filename);
    uint32_t num_rows = pager->file_length / ROW_SIZE;

    Table* table = malloc(sizeof(Table));
    table->pager = pager;
    table->num_rows = num_rows;

    return table;
}

void
db_close(Table* table) {
    Pager* pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (uint32_t i = 0; i < num_full_pages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }

        pager_flush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // there may be a partial page to write to the end of the file. This should not be needed after we switch to a btree
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;

    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;

        if (pager->pages[page_num] != NULL) {
            pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = NULL;
        }
    }

    int result = close(pager->fd);

    if (result == -1) {
        printf("error closing db file\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void* page = pager->pages[i];

        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }

    free(pager);
    free(table);
}
