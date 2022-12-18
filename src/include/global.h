#pragma once

#include "array.h"

#define GLOBAL_TABLE_SIZE 1024

enum
{
    S_VARIABLE,
    S_FUNCTION
};

typedef struct SYMBOL_TABLE_STRUCT
{
    char *name;
    int type;
    int s_type;
    int label;
} symbol_table_T;

static symbol_table_T *global_symbol[GLOBAL_TABLE_SIZE];
static int global_symbol_init[GLOBAL_TABLE_SIZE] = { 0 };
static int global_symbol_pos = 0;
static int current_function_inside = -1;
static int current_function_call = -1;

void create_symbol_table(char *name, int type, int s_type, int label);

int init_symbol(char *name);
int symbol_get(char *name);
int symbol_get_type(char *name);
int symbol_get_s_type(char *name);
int symbol_get_label(char *name);

symbol_table_T *symbol_table_find(int index);

void print_symbol_table();
