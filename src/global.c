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
