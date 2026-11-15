/**
 * @file precedence_parser.c
 *
 * @brief FIT VUT | IFJ Project 2026 - precedence parser implementation
 * @author Patrik Kapec (xkapecp00)
 * @date 14.11.2026
 */

#include "precedence_parser.h"
#include "stack.h"
#include <stdbool.h>
#include "token.h"
#include "parser.h"
#include "scanner.h"
#include "ast.h"
#include <string.h>
#include <stdlib.h>


/**
 * @brief precedence table
 *        row is the top TERMINAL on the stack
 *        column is the current input token
 *        < shift and mark the start of a handle
 *        > reduce
 *        = shift without a mark used for matching parentheses
 *        A accept
 *        x syntax error
 */
static char prec_table[15][15] = {
//          $    (    )    +    -   u-    *    /   ==   !=    <   <=    >   >=    i
/* $  */ {'A', '<', 'A', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<'},
/* (  */ {'x', '<', '=', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<'},
/* )  */ {'>', 'x', '>', '>', '>', 'x', '>', '>', '>', '>', '>', '>', '>', '>', 'x'},
/* +  */ {'>', '<', '>', '>', '>', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<'},
/* -  */ {'>', '<', '>', '>', '>', '<', '<', '<', '>', '>', '>', '>', '>', '>', '<'},
/* u- */ {'>', '<', '>', '>', '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<'},
/* *  */ {'>', '<', '>', '>', '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<'},
/* /  */ {'>', '<', '>', '>', '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<'},
/* == */ {'>', '<', '>', '<', '<', '<', '<', '<', 'x', 'x', 'x', 'x', 'x', 'x', '<'},
/* != */ {'>', '<', '>', '<', '<', '<', '<', '<', 'x', 'x', 'x', 'x', 'x', 'x', '<'},
/* <  */ {'>', '<', '>', '<', '<', '<', '<', '<', 'x', 'x', 'x', 'x', 'x', 'x', '<'},
/* <= */ {'>', '<', '>', '<', '<', '<', '<', '<', 'x', 'x', 'x', 'x', 'x', 'x', '<'},
/* >  */ {'>', '<', '>', '<', '<', '<', '<', '<', 'x', 'x', 'x', 'x', 'x', 'x', '<'},
/* >= */ {'>', '<', '>', '<', '<', '<', '<', '<', 'x', 'x', 'x', 'x', 'x', 'x', '<'},
/* i  */ {'>', 'x', '>', '>', '>', 'x', '>', '>', '>', '>', '>', '>', '>', '>', 'x'},
};


enum { DOLLAR, LPAR, RPAR, PLUS, MINUS, UMINUS, MUL, DIV, EQ, NEQ, LT, LE, GT, GE, I, E, SHIFT,REDUCE, };


static ast_node_t *parse_precedence_arguments(void);

/**
 * @brief finds the terminal on the top of the stack
 *        skips E and SHIFT('<')
 * @param stack symbol stack
 * @return the top terminal
 */

static int top_terminal(stack_t *stack){
    for(int i = 0;;i++){
        int *top_on_index = stack_get(stack,i);
        if(top_on_index == NULL) return DOLLAR;
        if(*top_on_index != E && *top_on_index != SHIFT){
            return *top_on_index;
        }
    }


}

/**
 * @brief maps a token to its column in the precedence table
 *        minus is binary after an operand and unary otherwise
 * @param current_token_type_parser type of the current token
 * @param previous_operand true if the previous symbol was an operand or closing parenthesis
 * @return column index
 */

static int get_index(token_type_t current_token_type_parser,bool previous_operand){
    if(current_token_type_parser == SP_OPENING_PARENTHESIS) return 1;
    else if (current_token_type_parser == SP_CLOSING_PARENTHESIS) return 2;
    else if (current_token_type_parser == OP_PLUS) return 3;
    else if (current_token_type_parser == OP_MINUS){
        if(previous_operand){
            return 4;
        }
        else{
            return 5;
        }
    }
    else if (current_token_type_parser == OP_MULTIPLY) return 6;
    else if (current_token_type_parser == OP_DIVIDE) return 7;
    else if (current_token_type_parser == OP_EQUAL) return 8;
    else if (current_token_type_parser == OP_NOT_EQUAL) return 9;
    else if (current_token_type_parser == OP_LESS) return 10;
    else if (current_token_type_parser == OP_LESS_OR_EQUAL) return 11;
    else if (current_token_type_parser == OP_GREATER) return 12;
    else if (current_token_type_parser == OP_GREATER_OR_EQUAL) return 13;
    else if (current_token_type_parser == STRING ||
            current_token_type_parser == INTEGER ||
            current_token_type_parser == DOUBLE ||
            current_token_type_parser == IDENTIFIER) return 14;
    else if (current_token_type_parser == SP_EOL ||
            current_token_type_parser == KW_THEN ||
            current_token_type_parser == KW_DO ||
            current_token_type_parser == SP_COMMA) return 0;
    else{
        fail(SYNTAX_ERROR);
        return 0;
    }
}



/**
 * @brief maps an operator from the table to its ast binary node type
 */
static const ast_binary_type_t bin_op[] = {
    [PLUS] = AST_ADDITION, [MINUS] = AST_SUBTRACTION, [MUL] = AST_MULTIPLICATION, [DIV] = AST_DIVISION,
    [EQ] = AST_EQUAL, [NEQ] = AST_NEQUAL, [LT] = AST_LESS, [LE] = AST_LEQUAL,
    [GT] = AST_GREATER, [GE] = AST_GEQUAL,
};


/**
 * @brief pushes a symbol on the symbol stack
 * @param stack symbol stack
 * @param symbol symbol to push
 */

static void stack_push_symbol(stack_t *stack,int symbol){
    if(stack_push(stack, &symbol) != 0) fail(MALLOC_FAIL);
}

/**
 * @brief pushes an ast node on the node stack
 * @param nodes node stack
 * @param node node to push
 */

static void stack_push_node(stack_t *nodes, ast_node_t *node){
    if(stack_push(nodes, &node) != 0) fail(MALLOC_FAIL);
}


/**
 * @brief pops the top node from the node stack
 * @param nodes node stack
 * @return the popped node
 */

static ast_node_t *stack_pop_node(stack_t *nodes) {
    ast_node_t **top = stack_top(nodes);
    if(top == NULL) fail(SYNTAX_ERROR);
    ast_node_t *node = *top;
    stack_pop(nodes);
    return node;
}



/**
 * @brief pops symbols up to the nearest SHIFT and replaces the handle with E
 *        E -> i
 *        E -> - E
 *        E -> ( )
 *        E -> E op E
 *        E -> ( E )
 *        builds the matching ast node on the node stack
 * @param stack symbol stack
 * @param nodes node stack
 */

static void reduce(stack_t *stack,stack_t *nodes){
    bool found = false;
    int depth = 0;
    int stack_reversed[4];

    for(int i = 0; i< 4;i++){
        int *top = stack_top(stack);
        if(top == NULL) fail(SYNTAX_ERROR);
        if(*top == DOLLAR) fail(SYNTAX_ERROR);
        int temp = *top;
        stack_pop(stack);
        if(temp == SHIFT){
            found = true;
            depth = i+1;
            break;
        }
        stack_reversed[i] = temp;
    }
    if(!found) fail(SYNTAX_ERROR);
    if(depth == 1) fail(SYNTAX_ERROR);
    if(depth == 2){
        if(stack_reversed[0] == I){
        stack_push_symbol(stack,E);
        return;
        }
        else fail(SYNTAX_ERROR);
    }
    if(depth == 3){
        if(stack_reversed[0] == E && stack_reversed[1] == UMINUS ){
            ast_node_t *operand = stack_pop_node(nodes);
            int64_t zero = 0;
            stack_push_node(nodes, make_binary_tree(AST_SUBTRACTION, make_node(AST_INTEGER, &zero), operand));
            stack_push_symbol(stack,E);
            return;
        }
        else if(stack_reversed[0] == RPAR && stack_reversed[1] == LPAR ){ //!!UNIT
            stack_push_node(nodes, make_node(AST_UNIT, NULL)); 
            stack_push_symbol(stack,E); 
            return;     
        }
        else fail(SYNTAX_ERROR);
    }
    if(depth == 4){
       if(stack_reversed[0] == E && stack_reversed[2] == E  &&
        (stack_reversed[1] == PLUS ||
        stack_reversed[1] == MINUS || 
        stack_reversed[1] == MUL || 
        stack_reversed[1] == DIV || 
        stack_reversed[1] == EQ ||
        stack_reversed[1] == NEQ ||
        stack_reversed[1] == LT || 
        stack_reversed[1] == LE || 
        stack_reversed[1] == GT || 
        stack_reversed[1] == GE )){
            ast_node_t *right = stack_pop_node(nodes); //!!!PRAVA JE NA VRCHU
            ast_node_t *left = stack_pop_node(nodes);
            stack_push_node(nodes, make_binary_tree(bin_op[stack_reversed[1]], left, right));
            stack_push_symbol(stack,E);
            return;
        }
        else if(stack_reversed[0] == RPAR && stack_reversed[1] == E && stack_reversed[2] == LPAR){
            stack_push_symbol(stack,E);
            return;

        }
        else {
        fail(SYNTAX_ERROR);
        }
    }
}


/**
 * @brief parses the arguments of a function call ( <ARGS> )
 *        <ARGS> -> epsilon
 *        <ARGS> -> <EXPR> <ARGS_REST>
 *        <ARGS_REST> -> , <EXPR> <ARGS_REST>
 *        <ARGS_REST> -> epsilon
 * @return linked list of argument expression nodes
 */

static ast_node_t *parse_precedence_arguments(void){
    ast_node_t* args = init_llist();
    consume();
    if(current_token_type_parser != SP_CLOSING_PARENTHESIS){
        add_llist_item(args, parse_precedence(NULL));
        while(current_token_type_parser == SP_COMMA){
        consume();
        add_llist_item(args, parse_precedence(NULL));
        }
    }
    
    expect(SP_CLOSING_PARENTHESIS);
    return args;
}

ast_node_t *parse_precedence(char* identifier_consumed){
    token_type_t old;
    ast_node_t* operand;
    stack_t *stack = NULL;
    stack_t *node_stack = NULL;
    char* name;
    if(stack_init(&stack, sizeof(int)) != 0) fail(MALLOC_FAIL);
    if(stack_init(&node_stack, sizeof(ast_node_t *)) != 0) fail(MALLOC_FAIL);
    stack_push_symbol(stack,DOLLAR);

    if(identifier_consumed != NULL){
      
        stack_push_symbol(stack,SHIFT);
        stack_push_symbol(stack,I);
        if(current_token_type_parser == SP_OPENING_PARENTHESIS){
           stack_push_node(node_stack, make_func_call_tree(make_node(AST_LEAF, identifier_consumed), parse_precedence_arguments()));
       }
    else{
        stack_push_node(node_stack, make_node(AST_LEAF, make_unique_name(identifier_consumed)));
        free(identifier_consumed);
    }
    }
    bool previous_operand = identifier_consumed == NULL ? false : true; //proste ked bol zjedeny operand tak je to true
    int *top;
    int a =top_terminal(stack);
    int b = get_index(current_token_type_parser,previous_operand);

    while (true){
        a = top_terminal(stack);
        b = get_index(current_token_type_parser,previous_operand);
        
        switch(prec_table[a][b]){
        case '=':
            stack_push_symbol(stack,b);
            consume();
            previous_operand = (b == I || b == RPAR);
            break;

        case '<':
            top = stack_top(stack);
            if(*top == E){
                stack_pop(stack);
                stack_push_symbol(stack,SHIFT);
                stack_push_symbol(stack,E);
            }
            else{
                stack_push_symbol(stack,SHIFT);
            }
            stack_push_symbol(stack,b);
            old = current_token_type_parser;
            operand = NULL;
            
            if(old == INTEGER){                               
                int64_t value;
                token_get_int(current_token_parser, &value);
                operand = make_node(AST_INTEGER, &value);
            }
            if(old == DOUBLE){
                double value;
                token_get_double(current_token_parser, &value);
                operand = make_node(AST_DOUBLE, &value);
            }
            if(old == STRING){
                const char *name;
                token_get_string(current_token_parser, &name);

                operand = make_node(AST_STRING, str_dup((char*)name));
            }
            if(old == IDENTIFIER){      
                const char *text;
                token_get_string(current_token_parser, &text);
                name = str_dup(text);
            }

            consume();
            if(old == IDENTIFIER && current_token_type_parser == SP_OPENING_PARENTHESIS){
                operand = make_func_call_tree(make_node(AST_LEAF,name), parse_precedence_arguments());
            }
            else if(old == IDENTIFIER){
                operand = make_node(AST_LEAF, make_unique_name(name));
                free(name);
            }

            if(b == I){
                stack_push_node(node_stack, operand);
            }
            previous_operand = (b == I || b == RPAR);
            break;
        case '>':
            reduce(stack,node_stack);
            break;
        case 'x':
            fail(SYNTAX_ERROR);
            stack_free(stack);
            return NULL;
        case 'A':
            top = stack_top(stack);
            if(*top == E){
            ast_node_t *result = stack_pop_node(node_stack);
            stack_free(stack);
            stack_free(node_stack);
            return result;
            }
            else {
                stack_free(stack);
                stack_free(node_stack);
                fail(SYNTAX_ERROR);
            }
        }
        
    }
    return NULL;
}