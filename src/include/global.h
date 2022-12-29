#pragma once

#include "array.h"
#include "lexer.h"

#define GLOBAL_TABLE_SIZE 1024
#define GLOBAL_ERROR_SIZE 1024

enum
{
    S_VARIABLE,
    S_FUNCTION
};

enum
{
    warn,
    error
};

typedef struct SYMBOL_TABLE_STRUCT
{
    char *name;
    int type;
    int s_type;
    int label;
} symbol_table_T;

typedef struct ERROR_STRUCT
{
    char *information;
    lexer_T *lexer;
    int level;
} error_T;

static symbol_table_T *global_symbol[GLOBAL_TABLE_SIZE];
static int global_symbol_init[GLOBAL_TABLE_SIZE] = { 0 };
static int global_symbol_pos = 0;
static int current_function_inside = -1;
static int current_function_call = -1;
static struct token *last_variable = NULL;

static error_T *global_error[GLOBAL_ERROR_SIZE];
static int global_error_pos = 0;

void create_symbol_table(char *name, int type, int s_type, int label);

int init_symbol(char *name);
int symbol_get(char *name);
int symbol_get_type(char *name);
int symbol_get_s_type(char *name);
int symbol_get_label(char *name);

symbol_table_T *symbol_table_find(int index);

void print_symbol_table();

void create_error(char *information, lexer_T *lexer, int level);
int check_error();
void stdout_error();
