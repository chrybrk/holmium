#pragma once

#include "libho.h"
#include "lexer.h"

enum
{
    AST_ADD,
    AST_SUB,
    AST_DIV,
    AST_MUL,
    AST_MOD,
    AST_EQU,
    AST_NEQ,
    AST_GT,
    AST_LT,
    AST_GEQ,
    AST_LEQ,
    AST_INTLIT,
    AST_IDENT,
    AST_LVAL,
    AST_ASSIGN,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_FUNCTION,
    AST_FUNCTION_CALL,
    AST_RETURN,
    AST_GLUE,
    AST_NOOP,
    AST_WIDE,
    AST_ADDR,
    AST_DEREF
};

enum
{
    P_nil, P_void, P_char, P_i16, P_i32, P_i64,
    P_voidptr, P_charptr, P_i16ptr, P_i32ptr, P_i64ptr
};

typedef struct PARSER_STRUCT
{
    lexer_T *lexer;
    struct token *token;
} parser_T;

parser_T *init_parser(lexer_T *lexer);

struct ASTnode
{
    int op;
    int type;
    int intvalue;
    struct ASTnode *left;
    struct ASTnode *mid;
    struct ASTnode *right;
};

struct ASTnode *init_ASTnode(int op, int type, int intvalue, struct ASTnode *left, struct ASTnode *mid, struct ASTnode *right);
struct ASTnode *ASTnode_leaf(int op, int type, int intvalue);
struct ASTnode *ASTnode_unary(int op, int type, int intvalue, struct ASTnode *left);

struct token *eat(parser_T *parser, int token);

struct ASTnode *prefix(parser_T *parser);
struct ASTnode *parser_parse_primary(parser_T *parser);
struct ASTnode *parser_parse_expr(parser_T *parser, int tok_prec);
struct ASTnode *parser_parse_assignment(parser_T *parser);
struct ASTnode *parser_parse_call(parser_T *parser);
struct ASTnode *parser_parse_function(parser_T *parser_T, int type);
struct ASTnode *parser_parse_variable(parser_T *parser, int type);
struct ASTnode *parser_parse_statement(parser_T *parser_T);
struct ASTnode *parser_parse_compound_statement(parser_T *parser);

void global_decl(parser_T *parser);

int ASTnode_op(struct token *token);
int ASTnode_op_prec(struct token *token);
int get_type(parser_T *parser);
int get_size(int type);
char *get_size_str(int size);
int type_check(int left, int right);
int pointer_to(int type);
int pointer_at(int type);
