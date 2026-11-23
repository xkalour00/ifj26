/**
 * @file codegen.c
 *
 * @author Dávid Čech-Gut
 * @brief
 * @date 2026-11-02
 */

#include "codegen.h"
#include "ast.h"
#include "codegen_builtin.h"

#define MAX_VARS 1000

static char *varNames[MAX_VARS];
static int varCount = 0;
static FILE *dst;
static int labelCounter;

static int generate_node(const ast_node_t *node);
static void add_var(char *name);
static void generate_function_vars(const ast_node_t *node);
static void push_parameters_reversed(const ast_node_t *node);
static void output_string(const char *str);

/**
 * @brief 
 * @param root Root node of AST
 * @param out Pointer to file destination
 * @return Error code
 */
int generate_code(const ast_node_t *root, FILE *out){
    dst = out;
    output(".IFJcode26");
    output("");
    output("DEFVAR GF@tmp");
    output("DEFVAR GF@concat_string_tmp1");
    output("DEFVAR GF@concat_string_tmp2");
    output("DEFVAR GF@exit_code");
    output("MOVE GF@exit_code int@0");
    output("");
    output("CALL func_%s", root->data.root.main_func->data.func_def.name);
    output("EXIT GF@exit_code");
    output("");

    generate_print_def();
    generate_read_line_def();
    generate_read_int_def();
    generate_read_double_def();
    generate_to_str_def();
    generate_to_int_def();
    generate_str_length_def();
    generate_sub_str_def();
    generate_ord_def();
    generate_chr_def();

    for(ast_node_t *item = root->data.root.func_def_list->data.llist.begin; 
        item != NULL; item = item->data.llist_item.next)
        {
            generate_node(item->data.llist_item.node);
        }
    return 0;
}

/**
 * @brief Processes a node from the AST
 * @param node Pointer to node, waiting to be processed
 * @return Returns error code
 */
static int generate_node(const ast_node_t *node){

    if(!node) return 0;

    switch(node->type) {
        // implicit conversion of integer node
        case AST_INTEGER: 
            if(node->data.integer.to_double){
                output("INT2FLOAT GF@tmp int@%d", node->data.integer.value);
                output("PUSHS GF@tmp");
                break;
            }
            output("PUSHS int@%d", node->data.integer.value); 
            break;
        case AST_DOUBLE: 
            output("PUSHS float@%a", node->data.fp); 
            break;
        case AST_LEAF:
            output("PUSHS LF@%s", node->data.leaf);
            break;
        case AST_STRING:
            output_string(node->data.str);
            break;
        // assignment is special from the rest, does not need to generate left node
        case AST_BINARY: 
            if (node->data.binary.binary_type == AST_ASSIGNMENT) {
                generate_node(node->data.binary.right);
                output("POPS LF@%s", node->data.binary.left->data.leaf);
                break;
            }
            // preorder traversal
            generate_node(node->data.binary.left);
            generate_node(node->data.binary.right);

            switch(node->data.binary.binary_type){
                case AST_ADDITION:  {
                    int id = labelCounter++;
                    output("POPS GF@tmp");
                    output("PUSHS GF@tmp");
                    output("TYPE GF@tmp GF@tmp");
                    output("JUMPIFEQ ast_concat_string%d GF@tmp string@string", id);
                    output("ADDS"); 
                    output("JUMP end_ast_addition%d", id);
                    output("LABEL ast_concat_string%d", id);
                    output("POPS GF@concat_string_tmp2");
                    output("POPS GF@concat_string_tmp1");
                    output("CONCAT GF@concat_string_tmp1 GF@concat_string_tmp1 GF@concat_string_tmp2");
                    output("PUSHS GF@concat_string_tmp1");
                    output("LABEL end_ast_addition%d", id);
                    break;
                }

                case AST_SUBTRACTION: 
                    output("SUBS"); 
                    break;

                case AST_MULTIPLICATION: 
                    output("MULS"); 
                    break;

                case AST_DIVISION: 
                    output("DIVS"); 
                    break;

                case AST_EQUAL:
                    output("EQS");
                    break;
                
                case AST_NEQUAL:
                    output("EQS");
                    output("NOTS");
                    break;

                case AST_LEQUAL: 
                    output("GTS");
                    output("NOTS");
                    break;

                case AST_GEQUAL:
                    output("LTS");
                    output("NOTS");
                    break;

                case AST_LESS:
                    output("LTS");
                    break;

                case AST_GREATER:
                    output("GTS");
                    break;

                default: break;
            }
            break;
        // function, if, else or while blocks are generated line by line where each line can be a tree
        case AST_LINKED_LIST:
            for(ast_node_t *item = node->data.llist.begin; 
                item != NULL; item = item->data.llist_item.next)
                {
                    generate_node(item->data.llist_item.node);
                }
            break;

        // Parameters are pushed from right to left,
        // this way we can process them starting from the first parameter
        case AST_FUNC_DEF:
            output("LABEL func_%s", node->data.func_def.name); 
            output("CREATEFRAME");
            output("PUSHFRAME");

            // reset varCount for each function definition
            varCount = 0;
            if(node->data.func_def.parameters){
                for(ast_node_t *item = node->data.func_def.parameters->data.llist.begin;
                    item != NULL; item = item->data.llist_item.next)
                    {
                        add_var(item->data.llist_item.node->data.declaration.name);
                        output("POPS LF@%s", item->data.llist_item.node->data.declaration.name);
                }
            }
            generate_function_vars(node->data.func_def.tree_list);
            generate_node(node->data.func_def.tree_list);
            output("");
            break;

        // pushes the last parameter first, so that first parameter is on top of the stack
        case AST_FUNC_CALL:
            if(node->data.func_call.parameters){
                push_parameters_reversed(node->data.func_call.parameters->data.llist.begin);
            }
            output("CALL func_%s", node->data.func_call.name);
            break;

        case AST_WHILE_LOOP: {
            int id = labelCounter++;
            
            output("LABEL while%d", id);
            generate_node(node->data.while_statement.condition);
            output("PUSHS bool@false");
            output("JUMPIFEQS end_while%d", id);

            generate_node(node->data.while_statement.while_block);
            output("JUMP while%d", id);
            output("LABEL end_while%d", id);
            break;
        }

        case AST_IF_STATEMENT: {
            int id = labelCounter++;

            generate_node(node->data.if_statement.condition);
            output("PUSHS bool@false");
            output("JUMPIFEQS else%d", id);
            generate_node(node->data.if_statement.if_block);
            output("JUMP end_if%d", id);

            output("LABEL else%d", id);
            generate_node(node->data.if_statement.else_block);
            output("LABEL end_if%d", id);

            break;
        }

        case AST_RETURN: 
            if(node->data.return_node.value == NULL){
                // output("PUSHS nil@nil");
                output("POPFRAME");
                output("RETURN");
                break;
            }
            generate_node(node->data.return_node.value);
            output("POPFRAME");
            output("RETURN");
            break;

        case AST_DECLARATION:
            if (node->data.declaration.value != NULL){
                generate_node(node->data.declaration.value);
                output("POPS LF@%s", node->data.declaration.name);
            }
        default: break;
    }

    return 0;
}

/**
 * @brief Collects all variables found in a function
 * @param node Pointer to beginning of function code block
 */
static void generate_function_vars(const ast_node_t *node){
    
    if(!node) return;

    switch(node->type) {
        case AST_LEAF:{
            add_var(node->data.leaf);
            break;
        }
        case AST_BINARY:
            generate_function_vars(node->data.binary.left);
            generate_function_vars(node->data.binary.right);
            break;
        case AST_LINKED_LIST:
            for(ast_node_t *item = node->data.llist.begin; 
                item != NULL; item = item->data.llist_item.next)
                {
                    generate_function_vars(item->data.llist_item.node);
                }
            break;
        case AST_FUNC_CALL:
            generate_function_vars(node->data.func_call.parameters);
            break;
        case AST_WHILE_LOOP:
            generate_function_vars(node->data.while_statement.condition);
            generate_function_vars(node->data.while_statement.while_block);
            break;
        case AST_IF_STATEMENT:
            generate_function_vars(node->data.if_statement.condition);
            generate_function_vars(node->data.if_statement.if_block);
            generate_function_vars(node->data.if_statement.else_block);
            break;
        case AST_DECLARATION:
            add_var(node->data.declaration.name);
            generate_function_vars(node->data.declaration.value);
            break;
        case AST_RETURN:
            generate_function_vars(node->data.return_node.value);
            break;
        default: break;
    }
}

/**
 * @brief Checks name of variable for duplicates and defines it in IFJCode26
 * @param name Variable name
 */

 // TODO faster implementation with hash table
static void add_var(char *name){
    for(int i = 0; i < varCount; i++){
        if(strcmp(name, varNames[i]) == 0) return;
    }
    varNames[varCount++] = name;
    output("DEFVAR LF@%s", name);
}
 
/**
 * @brief Pushes parameters of a function to the stack in reverse
 * @param node Pointer to first parameter
 */
static void push_parameters_reversed(const ast_node_t *node){
    if(node != NULL){
        push_parameters_reversed(node->data.llist_item.next);
        generate_node(node->data.llist_item.node);
    }
}

/**
 * @brief Handles easier output to out.ifjcode
 * @param fmt Formatting string
 */
void output(const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    vfprintf(dst, fmt, args);
    fprintf(dst, "\n");
    va_end(args);
}

/**
 * @brief Correctly outputs escape sequences in IFJcode26
 * @param str string from AST
 */
static void output_string(const char *str){
    fprintf(dst, "PUSHS string@");
    for(int i = 0; i < strlen(str); i++){
        if(str[i] < 33 || str[i] == 35 || str[i] == 92){
            fprintf(dst, "\\%03d", str[i]);
        }
        else{
            fprintf(dst, "%c", str[i]);
        }
    }
    fprintf(dst, "\n");
}   