#include "include/global.h"
#include "include/macro.h"

void create_symbol_table(char *name, int type, int s_type, int label)
{
    symbol_table_T *new_symbol_table = calloc(1, sizeof(struct SYMBOL_TABLE_STRUCT));

    new_symbol_table->name = name;
    new_symbol_table->type = type;
    new_symbol_table->s_type = s_type;
    new_symbol_table->label = label;

    if (global_symbol_pos < GLOBAL_TABLE_SIZE)
    {
        if (symbol_get(name) >= 0)
        {
            if (symbol_get_s_type(name) == s_type) 
            {
                log(3, "GLOBAL_SYMBOL_TABLE (err)\n\tfound `%s`, cannot redefine", name);
            }
        }
        global_symbol[global_symbol_pos] = new_symbol_table;
        global_symbol_pos++;
    }
    else
        log(3, "GLOBAL_SYMBOL_TABLE (err)\n\toverflow symbol, cannot assign `%s`", name);
}

int init_symbol(char *name)
{
    for (int i = 0; i < global_symbol_pos; i++)
    {
        if (!strcmp(name, global_symbol[i]->name))
        {
            if (global_symbol_init[i] == 0)
            {
                global_symbol_init[i] = 1;
                return 0;
            }
        }
    }

    return 1;
}

int symbol_get(char *name)
{
    for (int i = 0; i < global_symbol_pos; i++)
    {
        if (!strcmp(name, global_symbol[i]->name))
            return i;
    }

    return -1;
}

int symbol_get_type(char *name)
{
    return symbol_table_find(symbol_get(name))->type;
}

int symbol_get_s_type(char *name)
{
    return symbol_table_find(symbol_get(name))->s_type;
}

int symbol_get_label(char *name)
{
    return symbol_table_find(symbol_get(name))->label;
}

symbol_table_T *symbol_table_find(int index)
{
    if (index < global_symbol_pos) return global_symbol[index];
    return NULL;
}

void print_symbol_table()
{
    for (int i = 0; i < global_symbol_pos; i++)
    {
        symbol_table_T *symb = global_symbol[i];
        log(1, "name: %s, type: %d, s_type: %d, label: %d", symb->name, symb->type, symb->s_type, symb->label);
    }
}

void create_error(char *information, lexer_T *lexer, int level)
{
    error_T *new_error = calloc(1, sizeof(struct ERROR_STRUCT));

    new_error->information = information;
    new_error->lexer = lexer;
    new_error->level = level;


    if (global_error_pos < GLOBAL_ERROR_SIZE)
    {
        global_error[global_error_pos] = new_error;
        global_error_pos++;
    }
    else log(3, "%s", "cannot create error, overflow.");
}

int check_error()
{
    return global_error_pos;
}

char *err_get_line(char *src, int ln)
{
    int i = 0;
    char c = src[i];
    while (ln != 1 || c != '\0')
    {
        i++; c = src[i];
        if (c == '\n')
        {
            ln--;
        }
    }

    char *value = calloc(1, sizeof(char));
    while (0)
    {
        char *to_append = calloc(1, sizeof(char));
        to_append[0] = c;
        to_append[1] = '\0';

        value = realloc(value, (strlen(to_append) + strlen(value) + 8) * sizeof(char));
        strcat(value, to_append);

        i++; c = src[i];
        if (c == '\n') break;
    }

    return value;
}

char *err_get_level(int level)
{
    switch (level)
    {
        case warn: return "[warn]";
        case error: return "[error]";
        default: return "[info]";
    }
}

char *err_generate_template(error_T *element)
{
    char *temp = ">> %d:%d :: %s > %s\n"
                 "%s\n"
                 "%s\n";

    char *clm_cross = calloc(1, sizeof(char));
    for (int i = 0; i < element->lexer->clm; i++)
    {
        char *c = calloc(1, sizeof(char));
        c[0] = i < element->lexer->clm - 1 ? '-' : '^'; c[1] = '\0';

        clm_cross = realloc(clm_cross, (strlen(clm_cross) + strlen(c) + 8) * sizeof(char));
        strcat(clm_cross, c);
    }

    int total_size = strlen(err_get_level(element->level))
        + strlen(element->information)
        + strlen(err_get_line(element->lexer->src, element->lexer->ln))
        + strlen(clm_cross);

    int total_offset_byte = total_size + 256;
    char *return_value = calloc(total_offset_byte, sizeof(char));
    sprintf(
            return_value,
            temp,
            element->lexer->ln, element->lexer->clm - 1,
            err_get_level(element->level),
            element->information,
            err_get_line(element->lexer->src, element->lexer->ln),
            clm_cross
           );

    return return_value;
}

void stdout_error()
{
    for (int i = 0; i < global_error_pos; i++)
    {
        /*
         * > ln:line:clm :: level > information
         *      _________________
         *                       ^
        */

        fprintf(stdout, "%s", err_generate_template(global_error[i]));
    }

    if (global_error_pos != 0) exit(1);
}
