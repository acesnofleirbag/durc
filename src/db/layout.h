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
    u_int32_t id;
    char name[NAME_SIZE + 1];  // +1 for the '\0' char
    char email[EMAIL_SIZE + 1];  // +1 for the '\0' char
} Row;

const u_int32_t SCHEMA_ID_SIZE = attr_size_identifier(Row, id);
const u_int32_t SCHEMA_NAME_SIZE = attr_size_identifier(Row, name);
const u_int32_t SCHEMA_EMAIL_SIZE = attr_size_identifier(Row, email);

const u_int32_t SCHEMA_ID_OFFSET = 0;
const u_int32_t SCHEMA_NAME_OFFSET = SCHEMA_ID_OFFSET + SCHEMA_ID_SIZE;
const u_int32_t SCHEMA_EMAIL_OFFSET = SCHEMA_NAME_OFFSET + SCHEMA_NAME_SIZE;

const u_int32_t ROW_SIZE = SCHEMA_ID_SIZE + SCHEMA_NAME_SIZE + SCHEMA_EMAIL_SIZE;

const u_int32_t PAGE_SIZE = 4096;
const u_int32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const u_int32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
    int fd;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    u_int32_t num_rows;
    Pager* pager;
} Table;

typedef struct {
    Table* table;
    uint32_t row_num;
    bool end_of_table;
} Cursor;

void row_serialize(Row* source, void* desctination);
void row_deserialize(void* source, Row* destination);
void* cursor_value(Cursor* cursor);
Table* new_table();
void show_row(Row* row);
Pager* pager_open(const char* filename);
Table* db_open(const char* filename);
void db_close(Table* table);

#endif
