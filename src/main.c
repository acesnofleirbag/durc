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
            case (EXEC_DUPLICATE_KEY):
                printf("ERR: duplicated key\n");
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
    } else if (strcmp(input_buffer->buffer, ".const") == 0) {
        display_constants();
        return META_CMD_SUCCESS;
    } else if (strcmp(input_buffer->buffer, ".btree") == 0) {
        void* node = get_page(table->pager, 0);
        uint32_t num_cells = *leaf_node_num_cells(node);

        printf("leaf (size %d)\n", num_cells);

        for (uint32_t i = 0; i < num_cells; i++) {
            uint32_t key = *leaf_node_key(node, i);
            printf("%d:\t%d\n", i, key);
        }

        return META_CMD_SUCCESS;
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
    void* node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = (*leaf_node_num_cells(node));

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        return EXEC_RES_TABLE_FULL;
    }

    Row* row = &(statement->row);
    uint32_t key = row->id;
    Cursor* cursor = table_find(table, key);

    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);

        if (key_at_index == key) {
            return EXEC_DUPLICATE_KEY;
        }
    }

    leaf_node_insert(cursor, key, row);

    free(cursor);

    return EXEC_RES_SUCCESS;
}

ExecuteResult
exec_stmt_select(Table* table) {
    Cursor* cursor = table_start(table);
    Row row;

    while (!(cursor->end_of_table)) {
        row_deserialize(cursor_value(cursor), &row);
        show_row(&row);
        cursor_advance(cursor);
    }

    free(cursor);

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
cursor_value(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* page = get_page(cursor->table->pager, page_num);

    return leaf_node_value(page, cursor->cell_num);
}

void
show_row(Row* row) {
    printf("-- %d %s %s\n", row->id, row->name, row->email);
}

void*
get_page(Pager* pager, uint32_t page_num) {
    if (page_num > TABLE_MAX_PAGES) {
        printf("tried to fetch page number out of bounds %d > %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        // cache miss. Allocate memory and load from file
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length / PAGE_SIZE;

        // we might save a partial page at the end of the file
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num <= num_pages) {
            lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->fd, page, PAGE_SIZE);

            if (bytes_read == -1) {
                printf("error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages += 1;
        }
    }

    return pager->pages[page_num];
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
    pager->num_pages = (file_length / PAGE_SIZE);

    if (file_length % PAGE_SIZE != 0) {
        printf("db file is not a whole number of pages. corrupted file\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

void
pager_flush(Pager* pager, uint32_t page_num) {
    if (pager->pages[page_num] == NULL) {
        printf("tried to flush a null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        printf("error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->fd, pager->pages[page_num], PAGE_SIZE);

    if (bytes_written == -1) {
        printf("error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

Table*
db_open(const char* filename) {
    Pager* pager = pager_open(filename);

    Table* table = malloc(sizeof(Table));
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        void* root_node = get_page(pager, 0);
        init_leaf_node(root_node);
    }

    return table;
}

void
db_close(Table* table) {
    Pager* pager = table->pager;

    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }

        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
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

Cursor*
table_start(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));

    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    void* root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

void
cursor_advance(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* node = get_page(cursor->table->pager, page_num);
    cursor->cell_num += 1;

    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        cursor->end_of_table = true;
    }
}

// NOTE: this functions is used as a pointer arithmetic operations to get the memory address that the value will be
// stored or accessed
uint32_t*
leaf_node_num_cells(void* node) {
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void*
leaf_node_cell(void* node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t*
leaf_node_key(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

void*
leaf_node_value(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void
init_leaf_node(void* node) {
    set_node_type(node, NODE_LEAF);
    *leaf_node_num_cells(node) = 0;
}

void
leaf_node_insert(Cursor* cursor, uint32_t key, Row* data) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        printf("need to implement splitting on leaf node\n");
        exit(EXIT_FAILURE);
    }

    // just fill up the bytes like "allocating" ???
    if (cursor->cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    row_serialize(data, leaf_node_value(node, cursor->cell_num));
}

void
display_constants() {
    printf("row size: %d\n"
           "common node header size: %d\n"
           "leaf node header size: %d\n"
           "leaf node cell size: %d\n"
           "leaf node space for cells: %d\n"
           "leaf node max cells: %d\n",
           ROW_SIZE,
           COMMON_NODE_HEADER_SIZE,
           LEAF_NODE_HEADER_SIZE,
           LEAF_NODE_CELL_SIZE,
           LEAF_NODE_SPACE_FOR_CELLS,
           LEAF_NODE_MAX_CELLS);
}

Cursor*
table_find(Table* table, uint32_t key) {
    void* root_node = get_page(table->pager, table->root_page_num);

    if (get_node_type(root_node) == NODE_LEAF) {
        return leaf_node_find(table, table->root_page_num, key);
    }

    printf("need to implement search on internal node\n");
    exit(EXIT_FAILURE);
}

Cursor*
leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    Cursor* cursor = malloc(sizeof(Cursor));

    cursor->table = table;
    cursor->page_num = page_num;

    // binary search tree implementation
    uint32_t start_idx = 0;
    uint32_t end_idx = num_cells;

    while (start_idx != end_idx) {
        uint32_t middle = (start_idx + end_idx) / 2;
        uint32_t key_at_index = *leaf_node_key(node, middle);

        if (key == key_at_index) {
            cursor->cell_num = middle;

            return cursor;
        } else if (key < key_at_index) {
            end_idx = middle;
        } else {
            start_idx = middle + 1;
        }
    }

    cursor->cell_num = start_idx;

    return cursor;
}

NodeType
get_node_type(void* node) {
    uint8_t type = *((uint8_t*) (node + NODE_TYPE_OFFSET));

    return (NodeType) type;
}

void
set_node_type(void* node, NodeType type) {
    *((uint8_t*) (node + NODE_TYPE_OFFSET)) = (uint8_t) type;
}
