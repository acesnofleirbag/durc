#ifndef DB_LAYOUT_H
#define DB_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#define attr_size_identifier(Struct, Attr) sizeof(((Struct*) 0)->Attr)

#define NAME_SIZE 32
#define EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100

typedef struct {
    uint32_t id;
    char name[NAME_SIZE + 1];  // +1 for the '\0' char
    char email[EMAIL_SIZE + 1];  // +1 for the '\0' char
} Row;

const uint32_t SCHEMA_ID_SIZE = attr_size_identifier(Row, id);
const uint32_t SCHEMA_NAME_SIZE = attr_size_identifier(Row, name);
const uint32_t SCHEMA_EMAIL_SIZE = attr_size_identifier(Row, email);
const uint32_t SCHEMA_ID_OFFSET = 0;
const uint32_t SCHEMA_NAME_OFFSET = SCHEMA_ID_OFFSET + SCHEMA_ID_SIZE;
const uint32_t SCHEMA_EMAIL_OFFSET = SCHEMA_NAME_OFFSET + SCHEMA_NAME_SIZE;
const uint32_t ROW_SIZE = SCHEMA_ID_SIZE + SCHEMA_NAME_SIZE + SCHEMA_EMAIL_SIZE;
const uint32_t PAGE_SIZE = 4096;

// common node header layout
// NOTE: the type just need an 1 bit for the representation until we've only 2 node types, but it's represented inside
// an entirely byte to make more easely the implementation
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
// NOTE: the is root flag just need an 1 bit for the representation until we've only 2 node types, but it's represented
// inside an entirely byte to make more easely the implementation
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_SIZE + IS_ROOT_OFFSET;
const uint8_t COMMON_NODE_HEADER_SIZE = IS_ROOT_SIZE + NODE_TYPE_SIZE + PARENT_POINTER_SIZE;

// leaf node header layout
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

// leaf node body layout
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;  // +1 'cause new node
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT =
    (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;  // +1 'cause new node

// internal node header format
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

// internal node body layout
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

typedef struct {
    int fd;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    uint32_t root_page_num;
    Pager* pager;
} Table;

typedef struct {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
} Cursor;

typedef enum { NODE_INTERNAL, NODE_LEAF } NodeType;

void row_serialize(Row* source, void* desctination);
void row_deserialize(void* source, Row* destination);
void* cursor_value(Cursor* cursor);
Table* new_table();
void show_row(Row* row);
Pager* pager_open(const char* filename);
Table* db_open(const char* filename);
void db_close(Table* table);

#endif
