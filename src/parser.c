#include "include/parser.h"
#include "include/macro.h"
#include "include/global.h"

parser_T *init_parser(lexer_T *lexer)
{
    parser_T *new_parser = calloc(1, sizeof(struct PARSER_STRUCT));

    new_parser->lexer = lexer;
    new_parser->token = lexer_next_token(lexer);

    return new_parser;
}

struct ASTnode *init_ASTnode(int op, int type, int intvalue, struct ASTnode *left, struct ASTnode *mid, struct ASTnode *right)
{
    struct ASTnode *new_ASTnode = calloc(1, sizeof(struct ASTnode));

    new_ASTnode->op = op;
    new_ASTnode->type = type;
    new_ASTnode->intvalue = intvalue;
    new_ASTnode->left = left;
    new_ASTnode->mid = mid;
    new_ASTnode->right = right;

    return new_ASTnode;
}

struct ASTnode *ASTnode_leaf(int op, int type, int intvalue)
{
    return init_ASTnode(op, type, intvalue, NULL, NULL, NULL);
}

struct ASTnode *ASTnode_unary(int op, int type, int intvalue, struct ASTnode *left)
{
    return init_ASTnode(op, type, intvalue, left, NULL, NULL);
}

struct token *eat(parser_T *parser, int token)
{
    if (parser->token->token != token)
        log(
            3,
            "ln:%d:%d\n\tsyntax err, expected: %s, got %s",
            parser->token->ln,
            parser->token->clm - 1,
            tok_type_to_string(token),
            tok_type_to_string(parser->token->token)
        );

    struct token *to_return = parser->token;
    parser->token = lexer_next_token(parser->lexer);

    return to_return;
}

struct token *eat_undo(parser_T *parser, int offset)
{
    int total_offset = parser->lexer->i - offset;

    if (total_offset < parser->lexer->size && total_offset > 0)
    {
        // FIXME: unable to load old clm, ln
        parser->lexer->i = total_offset;
        parser->lexer->c = parser->lexer->src[parser->lexer->i];
        parser->token = lexer_next_token(parser->lexer);
    }

    return parser->token;
}

struct token *expected_tok(parser_T *parser, int token)
{
    if (parser->token->token != token) log(3, "ln:%d:%d\n\tsyntax err, expected: `%s` got `%s`", parser->token->ln, parser->token->clm - 1, tok_type_to_string(token), tok_type_to_string(parser->token->token));
    struct token *tok = parser->token;
    parser->token = lexer_next_token(parser->lexer);

    return tok;
}

struct ASTnode *prefix(parser_T *parser)
{
    struct ASTnode *tree;
    switch (parser->token->token)
    {
        case T_AMPER:
            {
                eat(parser, parser->token->token);
                tree = prefix(parser);

                if (tree->op != AST_IDENT) log(3, "%s", "& op must followed by an identifier.");
                tree->op = AST_ADDR; tree->type = pointer_to(tree->type);
                break;
            }
        case T_STAR:
            {
                eat(parser, parser->token->token);
                tree = prefix(parser);
                if (tree->op != AST_IDENT && tree->op != AST_DEREF) log(3, "%s", "* op must be followed by an identifier or *");

                tree = ASTnode_unary(AST_DEREF, pointer_at(tree->type), 0, tree);
                break;
            }
        default: tree = parser_parse_primary(parser);
    }

    return tree;
}

struct ASTnode *parser_parse_primary(parser_T *parser)
{
    switch (parser->token->token)
    {
        case T_INTLIT:
            {
                int value = atoi(parser->token->value);
                eat(parser, T_INTLIT);

                return ASTnode_leaf(AST_INTLIT, P_nil, value);
            }
        case T_IDENT:
            {
                int id = symbol_get(parser->token->value);
                if (symbol_table_find(id)->s_type == S_FUNCTION) return parser_parse_call(parser);

                if (id == -1)
                {
                    log(3, "ln:%d:%d\n\tUndefined variable `%s`", parser->token->ln, parser->token->clm - 1, parser->token->value);
                }
                struct ASTnode *node = ASTnode_leaf(AST_IDENT, symbol_table_find(id)->type, id);
                eat(parser, T_IDENT);

                return node;
            }
        default: log(3, "ln:%d:%d\n\tsyntax err `%s`", parser->token->ln, parser->token->clm - 1, parser->token->value);
    }

    return 0;
}

struct ASTnode *parser_parse_expr(parser_T *parser, int tok_prec)
{
    struct ASTnode *left, *right;
    struct token *tok;

    left = prefix(parser);

    tok = parser->token;
    if (tok->token == T_SEMI || tok->token == T_RPAREN)
        return left;

    while (ASTnode_op_prec(tok) > tok_prec)
    {
        parser->token = lexer_next_token(parser->lexer);

        right = parser_parse_expr(parser, ASTnode_op_prec(tok));

        int leftype = left->type;
        int rightype = right->type;

        left = init_ASTnode(ASTnode_op(tok), left->type, 0, left, NULL, right);

        tok = parser->token;
        if (tok->token == T_SEMI || tok->token == T_RPAREN)
            return left;
    }

    return left;
}

struct ASTnode *parser_parse_assignment(parser_T *parser)
{
    struct ASTnode *left, *right, *tree;
    int id = symbol_get(last_variable == NULL ? parser->token->value : last_variable->value);

    if (id == -1)
    {
        log(3, "ln:%d:%d\n\tUndefined variable `%s`", parser->token->ln, parser->token->clm - 1, parser->token->value);
    }

    right = ASTnode_leaf(AST_LVAL, symbol_table_find(id)->type, id);
    if (last_variable == NULL) eat(parser, T_IDENT);
    expected_tok(parser, T_ASSIGN);

    left = parser_parse_expr(parser, 0);
    // left = prefix(parser);
    // left->type = symbol_table_find(id)->type;

    left->type = right->type;

    int leftype = left->type; int rightype = right->type;
    type_check(leftype, rightype);
    tree = init_ASTnode(AST_ASSIGN, symbol_table_find(id)->type, 0, left, NULL, right);

    if (parser->token->token != T_RPAREN) expected_tok(parser, T_SEMI);

    last_variable = NULL;

    return tree;
}

struct ASTnode *parser_parse_variable(parser_T *parser, int type)
{

    while (1)
    {
        // TODO: it might not work because of old poisition collapse

        struct token *tok = parser->token;

        char *identifier = calloc(1, sizeof(strlen(tok->value)));
        strcpy(identifier, tok->value);
        eat(parser, T_IDENT);

        create_symbol_table(identifier, type, S_VARIABLE, 0);

        if (parser->token->token == T_SEMI)
        {
            eat(parser, T_SEMI); return NULL;
        }
        else if (parser->token->token == T_COMMA)
        {
            eat(parser, T_COMMA); continue;
        }
        else if (parser->token->token == T_ASSIGN)
        {
            last_variable = tok;
            return parser_parse_assignment(parser);
        }
        else return NULL;
    }

    log(3, "ln:%d:%d\n\texpected ; or ,", parser->token->ln, parser->token->clm);
}

struct ASTnode *parser_parse_for(parser_T *parser)
{
    struct ASTnode *condtionalAST, *bodyAST = NULL;
    struct ASTnode *preopAST, *postopAST = NULL;
    struct ASTnode *tree = NULL;

    eat(parser, T_FOR);
    eat(parser, T_LPAREN);

    if (parser->token->token < T_char || parser->token->token > T_i64)
    {
        log(3, "ln:%d:%d\n\texpected `char`, `i16`, `i32` or `i64`, got `%s`", parser->token->ln, parser->token->clm - 1, parser->token->value);
    }

    parser_parse_statement(parser);
    preopAST = parser_parse_statement(parser);
    condtionalAST = parser_parse_expr(parser, 0);
    expected_tok(parser, T_SEMI);
    if (condtionalAST->op < AST_EQU || condtionalAST->op > AST_LEQ) log(3, "invalid cmp, `%s`", tok_type_to_string(condtionalAST->op));
    postopAST = parser_parse_statement(parser);
    eat(parser, T_RPAREN);

    bodyAST = parser_parse_compound_statement(parser);
    bodyAST = (bodyAST == NULL) ? ASTnode_unary(AST_NOOP, P_nil, 0, bodyAST) : bodyAST;

    tree = init_ASTnode(AST_GLUE, P_nil, 0, bodyAST, NULL, postopAST);
    tree = init_ASTnode(AST_WHILE, P_nil, 0, condtionalAST, NULL, tree);
    return init_ASTnode(AST_GLUE, P_nil, 0, preopAST, NULL, tree);
}

struct ASTnode *parser_parse_function(parser_T *parser, int type)
{
    char *ident = calloc(1, sizeof(strlen(parser->token->value)));
    strcpy(ident, parser->token->value);
    eat(parser, T_IDENT);

    create_symbol_table(ident, type, S_FUNCTION, 0);
    int slot = symbol_get(ident);
    current_function_call = slot;
    eat(parser, T_LPAREN);
    eat(parser, T_RPAREN);

    struct ASTnode *tree = parser_parse_compound_statement(parser);
    struct ASTnode *last;

    if (type != P_void)
    {
        last = (tree->op == AST_GLUE) ? tree->right : tree;
        if (last == NULL || last->op != AST_RETURN) log(3, "%s", "Non-void returns a valid value.");
        // if (type != last->type) log(3, "%s", "expected i32 got char");
    }

    return ASTnode_unary(AST_FUNCTION, type, slot, tree);
}

struct ASTnode *parser_parse_call(parser_T *parser)
{
    return NULL;
}

struct ASTnode *parser_parse_return(parser_T *parser)
{
    if (symbol_table_find(current_function_call)->type == P_void) log(3, "%s", "void function cannot return.");
    struct ASTnode *tree;

    eat(parser, T_RETURN);

    tree = parser_parse_expr(parser, 0);
    if (tree->op == AST_INTLIT) tree->type = symbol_table_find(current_function_call)->type;

    if (tree->type != symbol_table_find(current_function_call)->type) log(3, "%s", "function return type don't match.");
    tree = ASTnode_unary(AST_RETURN, P_nil, current_function_call, tree);
    eat(parser, T_SEMI);

    return tree;
}


struct ASTnode *parser_parse_statement(parser_T *parser)
{
    struct ASTnode *tree;

    switch (parser->token->token)
    {
        case T_RETURN: tree = parser_parse_return(parser); break;
        case T_FOR: tree = parser_parse_for(parser); break;
        case T_char: tree = parser_parse_variable(parser, get_type(parser)); break;
        case T_i16: tree = parser_parse_variable(parser, get_type(parser)); break;
        case T_i32: tree = parser_parse_variable(parser, get_type(parser)); break;
        case T_i64: tree = parser_parse_variable(parser, get_type(parser)); break;
        case T_void: tree = parser_parse_variable(parser, get_type(parser)); break;
        case T_IDENT:
                    {
                        if (token_peek(parser->lexer, 1)->token == T_ASSIGN) tree = parser_parse_assignment(parser); break;
                    }
        default: tree = parser_parse_expr(parser, 0); break;
    }

    return tree;
}

struct ASTnode *parser_parse_compound_statement(parser_T *parser)
{
    struct ASTnode *tree;
    struct ASTnode *left = NULL;

    eat(parser, T_LBRACE);


    while (parser->token->token != T_RBRACE)
    {
        switch (parser->token->token)
        {
            case T_LBRACE: tree = parser_parse_compound_statement(parser); break;
            default: tree = parser_parse_statement(parser);
        }
        // tree = parser_parse_statement(parser);


        if (tree)
        {
            if (left == NULL) left = tree;
            else left = init_ASTnode(AST_GLUE, P_nil, 0, left, NULL, tree);
        }

        if (parser->token->token == T_SEMI) eat(parser, T_SEMI);
    }

    eat(parser, T_RBRACE);

    return left;
}

void global_decl(parser_T *parser)
{
    struct ASTnode *tree;
    int type;

    while (1)
    {
        type = get_type(parser);

        // eat token
        if (token_peek(parser->lexer, 1)->token == T_LPAREN)
        {
            tree = parser_parse_function(parser, type);
            // generate ast
        }
        else parser_parse_variable(parser, type);

        if (parser->token->token == T_EOF) break;
    }
}

int ASTnode_op(struct token *token)
{
    switch (token->token)
    {
        case T_PLUS: return AST_ADD;
        case T_MINUS: return AST_SUB;
        case T_STAR: return AST_MUL;
        case T_SLASH: return AST_DIV;
        case T_MODULO: return AST_MOD;
        case T_EQU: return AST_EQU;
        case T_NEQ: return AST_NEQ;
        case T_GT: return AST_GT;
        case T_LT: return AST_LT;
        case T_GEQ: return AST_GEQ;
        case T_LEQ: return AST_LEQ;
        default: log(3, "Unrecognised token `%s`", tok_string(token));
    }

    // to remove warning
    return 0;
}

int ASTnode_op_prec(struct token *token)
{
    switch (token->token)
    {
        case T_PLUS:
        case T_MINUS:
            return 10;

        case T_STAR:
        case T_SLASH:
        case T_MODULO:
            return 20;

        case T_EQU:
        case T_NEQ:
            return 30;

        case T_LT:
        case T_GT:
        case T_LEQ:
        case T_GEQ:
            return 40;

        default: log(3, "ln:%d:%d\n\tsyntax err, tok: %s", token->ln, token->clm, tok_string(token));
    }

    return 0;
}

int get_type(parser_T *parser)
{
    int newtype;
    switch (parser->token->token)
    {
        case T_void: newtype = P_void; break;
        case T_char: newtype = P_char; break;
        case T_i16: newtype = P_i16; break;
        case T_i32: newtype = P_i32; break;
        case T_i64: newtype = P_i64; break;
        default: log(3, "%s", "Unrecognised data type.");
    }

    while(1)
    {
        eat(parser, parser->token->token);
        if (parser->token->token != T_STAR) break;
        newtype = pointer_to(newtype);
    }

    return newtype;
}

int get_size(int type)
{
    switch(type)
    {
        case P_void: return 0;
        case P_char: return 1;
        case P_i16: return 2;
        case P_i32: return 4;
        case P_i64:
        case P_voidptr:
        case P_charptr:
        case P_i16ptr:
        case P_i32ptr:
        case P_i64ptr:
                    return 8;
        default: return 0;
    }
}

char *get_size_str(int size)
{
    return NULL;
}

int type_check(int left, int right)
{
    // incomplete checking

    return 0;
}

int pointer_to(int type)
{
    int newtype;
    switch (type)
    {
        case P_void: newtype = P_voidptr; break;
        case P_char: newtype = P_charptr; break;
        case P_i16: newtype = P_i16ptr; break;
        case P_i32: newtype = P_i32ptr; break;
        case P_i64: newtype = P_i64ptr; break;
        default: log(3, "%s", "failed to locate pointer to unknown pointer type.");
    }

    return newtype;
}

int pointer_at(int type)
{
    int newtype;
    switch (type)
    {
        case P_voidptr: newtype = P_void; break;
        case P_charptr: newtype = P_char; break;
        case P_i16ptr: newtype = P_i16; break;
        case P_i32ptr: newtype = P_i32; break;
        case P_i64ptr: newtype = P_i64; break;
        default: log(3, "%s", "failed to locate pointer at unknown pointer type.");
    }

    return newtype;
}
