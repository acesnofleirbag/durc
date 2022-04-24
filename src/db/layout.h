#ifndef DB_LAYOUT_H
#define DB_LAYOUT_H

#include <stdio.h>
#include <sys/types.h>

#define attr_size_identifier(Struct, Attr) sizeof(((Struct*) 0)->Attr)

#define NAME_SIZE 32
#define EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100

typedef struct {
    u_int32_t id;
    char name[NAME_SIZE];
    char email[EMAIL_SIZE];
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
    u_int32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
} Table;

void row_serialize(Row*, void*);
void row_deserialize(void*, Row*);
void* define_row_slot(Table*, u_int32_t);
Table* new_table();
void free_table(Table*);
void show_row(Row*);

#endif
