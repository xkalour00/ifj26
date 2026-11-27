/**
 * @file codegen.c
 *
 * @author Dávid Čech-Gut
 * @brief FIT VUT | IFJ project 2026 - implementation of code generator
 * @date 2026-11-02
 */

#include "codegen.h"
#include "ast.h"
#include "codegen_builtin.h"
#include "hashtable.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define INTERNAL_ERR 99
#define GEN_SUCCESS 0

static int varCount = 0;
static FILE *dst;
static int labelCounter = 0;
static hashtable_t *hashtable = NULL;

static int generate_node(ast_node_t *node, const gen_mode mode);
static int add_var(ast_node_t *node);
static int collect_function_vars(ast_node_t *node);
static void push_parameters_reversed(const ast_node_t *node);
static void output_string(const char *str);

/*
local variables  - name$unique_int
global variables - &name
functions        - _func_name
labels           - !label_name_labelCounter
*/

/**
 * @brief Generates header, global variables and all function definitions
 * @param root Root node of AST
 * @param out Pointer to file destination
 * @return Error code
 */
int generate_code(ast_node_t *root, FILE *out){
    dst = out;
    output(".IFJcode26");
    output("");
    output("DEFVAR GF@&tmp");
    output("DEFVAR GF@&concat_string_tmp1");
    output("DEFVAR GF@&concat_string_tmp2");
    output("DEFVAR GF@&exit_code");
    output("MOVE GF@&exit_code int@0");
    output("");
    output("CALL _func_%s", root->data.root.main_func->data.func_def.name);
    output("EXIT GF@&exit_code");
    output("");

    generate_print_def();
    generate_read_line_def();
    generate_read_int_def();
    generate_read_double_def();
    generate_read_boolean_def();
    generate_to_str_def();
    generate_to_int_def();
    generate_str_length_def();
    generate_sub_str_def();
    generate_ord_def();
    generate_chr_def();

    if(hashtable_init(&hashtable) != 0){
        return INTERNAL_ERR;
    }

    // generates all functions
    for(ast_node_t *item = root->data.root.func_def_list->data.llist.begin; 
        item != NULL; item = item->data.llist_item.next)
        {
            if(generate_node(item->data.llist_item.node, GEN_DISCARD) != 0) {
                hashtable_free(hashtable);
                hashtable = NULL;
                return INTERNAL_ERR;
            }
        }
    hashtable_free(hashtable);
    hashtable = NULL;
    return GEN_SUCCESS;
}

/**
 * @brief Processes a node from the AST
 * @param node Pointer to node, waiting to be processed
 * @return Returns error code
 */
static int generate_node(ast_node_t *node, const gen_mode mode){

    if(!node) return GEN_SUCCESS;

    switch(node->type) {
        // implicit conversion of integer node
        case AST_INTEGER: 
            if(node->to_double){
                output("INT2FLOAT GF@&tmp int@%d", node->data.integer);
                output("PUSHS GF@&tmp");
                break;
            }
            output("PUSHS int@%d", node->data.integer); 
            break;
        case AST_DOUBLE: 
            output("PUSHS float@%a", node->data.fp); 
            break;
        case AST_LEAF:
            output("PUSHS LF@%s", node->data.leaf->declaration->data.declaration.codegen_name);
            break;
        case AST_STRING:
            output_string(node->data.str);
            break;
        case AST_BOOLEAN:
            output("PUSHS bool@%s", node->data.bool_value ? "true" : "false");
            break;
        
        // assignment is special from the rest, does not need to generate left node
        case AST_BINARY: {
            int id = labelCounter++;
            if (node->data.binary.binary_type == AST_ASSIGNMENT) {
                generate_node(node->data.binary.right, GEN_PUSH);
                output("POPS LF@%s", node->data.binary.left->data.leaf->declaration->data.declaration.codegen_name);
                if(mode == GEN_PUSH) output("PUSHS nil@nil");
                break;
            }

            // short circuit evaluation of and
            if(node->data.binary.binary_type == AST_AND){
                generate_node(node->data.binary.left, GEN_PUSH);
                output("PUSHS bool@false");
                output("JUMPIFEQS !and_short_circuit_%d", id);
                output("PUSHS bool@true");
                generate_node(node->data.binary.right, GEN_PUSH);
                output("ANDS");
                output("JUMP !and_end_%d", id);
                output("LABEL !and_short_circuit_%d", id);
                output("PUSHS bool@false");
                output("LABEL !and_end_%d", id);
                if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                break;
            }

            // short circuit evaluation of or
            if(node->data.binary.binary_type == AST_OR){
                generate_node(node->data.binary.left, GEN_PUSH);
                output("PUSHS bool@true");
                output("JUMPIFEQS !or_short_circuit_%d", id);
                output("PUSHS bool@false");
                generate_node(node->data.binary.right, GEN_PUSH);
                output("ORS");
                output("JUMP !or_end_%d", id);
                output("LABEL !or_short_circuit_%d", id);
                output("PUSHS bool@true");
                output("LABEL !or_end_%d", id);
                if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                break;
            }
            }

            // preorder traversal
            generate_node(node->data.binary.left, GEN_PUSH);
            generate_node(node->data.binary.right, GEN_PUSH);

            switch(node->data.binary.binary_type){
                case AST_ADDITION:  {
                    if(node->ret_type == RETURN_STRING){
                        output("POPS GF@&concat_string_tmp2");
                        output("POPS GF@&concat_string_tmp1");
                        output("CONCAT GF@&concat_string_tmp1 GF@&concat_string_tmp1 GF@&concat_string_tmp2");
                        output("PUSHS GF@&concat_string_tmp1");
                    }
                    else{
                        output("ADDS");
                    }
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;
                }

                case AST_SUBTRACTION: 
                    output("SUBS"); 
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_MULTIPLICATION: 
                    output("MULS"); 
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_DIVISION: 
                    if(node->ret_type == AST_INTEGER) output("DIVS");
                    else output("IDIVS"); 
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_EQUAL:
                    output("EQS");
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;
                
                case AST_NEQUAL:
                    output("EQS");
                    output("NOTS");
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_LEQUAL: 
                    output("GTS");
                    output("NOTS");
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_GEQUAL:
                    output("LTS");
                    output("NOTS");
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_LESS:
                    output("LTS");
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_GREATER:
                    output("GTS");
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                case AST_NOT:
                    output("NOTS");
                    if(mode == GEN_DISCARD) output("POPS GF@&tmp");
                    break;

                default: break;
            }
            break;
        // function, if, else or while blocks are generated line by line where each line can be a tree
        case AST_LINKED_LIST:
            for(ast_node_t *item = node->data.llist.begin; 
                item != NULL; item = item->data.llist_item.next)
                {
                    if(item == node->data.llist.end && mode == GEN_PUSH){
                        generate_node(item->data.llist_item.node, GEN_PUSH);
                        break;
                    }
                    generate_node(item->data.llist_item.node, GEN_DISCARD);
                }
            break;

        // Parameters are pushed from right to left,
        // this way we can process them starting from the first parameter
        case AST_FUNC_DEF:
            output("LABEL _func_%s", node->data.func_def.name); 
            output("CREATEFRAME");
            output("PUSHFRAME");

            hashtable_clear(hashtable, NULL);
            varCount = 0;
            if(node->data.func_def.parameters){
                for(ast_node_t *item = node->data.func_def.parameters->data.llist.begin;
                    item != NULL; item = item->data.llist_item.next)
                    {
                        if(add_var(item->data.llist_item.node) != 0) return INTERNAL_ERR;
                        output("POPS LF@%s", item->data.llist_item.node->data.declaration.codegen_name);
                }
            }
            collect_function_vars(node->data.func_def.tree_list);
            // TODO add GEN_DISCARD if function is of UNIT type
            if(node->ret_type == RETURN_UNIT){
                generate_node(node->data.func_def.tree_list, GEN_DISCARD);
                output("PUSHS nil@nil");
            }
            else{
                generate_node(node->data.func_def.tree_list, GEN_PUSH);
            }
            // generate_node(node->data.func_def.tree_list, GEN_PUSH);
            if(!node->data.func_def.has_return_node){
                output("POPFRAME");
                output("RETURN");
            }
            output("");
            break;

        // pushes the last parameter first, so that first parameter is on top of the stack
        case AST_FUNC_CALL:
            if(node->data.func_call.parameters){
                push_parameters_reversed(node->data.func_call.parameters->data.llist.begin);
            }
            output("CALL _func_%s", node->data.func_call.name);
            if(mode == GEN_DISCARD) output("POPS GF@&tmp");
            break;

        case AST_WHILE_LOOP: {
            int id = labelCounter++;
            
            output("LABEL !while_%d", id);
            generate_node(node->data.while_statement.condition, GEN_PUSH);
            output("PUSHS bool@false");
            output("JUMPIFEQS !end_while_%d", id);

            generate_node(node->data.while_statement.while_block, GEN_DISCARD);
            output("JUMP !while_%d", id);
            output("LABEL !end_while_%d", id);

            if(mode == GEN_PUSH) output("PUSHS nil@nil");
            break;
        }

        case AST_IF_STATEMENT: {
            int id = labelCounter++;

            generate_node(node->data.if_statement.condition, GEN_PUSH);
            output("PUSHS bool@false");
            output("JUMPIFEQS !next_if_%d", id);
            generate_node(node->data.if_statement.if_block, GEN_PUSH);
            output("JUMP !end_if_%d", id);
            output("LABEL !next_if_%d", id);

            if(node->data.if_statement.llist_else_if){
                int else_if_count = 0;
                for(ast_node_t *item = node->data.if_statement.llist_else_if->data.llist.begin;
                    item != NULL; item = item->data.llist_item.next)
                    {
                        generate_node(item->data.llist_item.node->data.else_if.cond, GEN_PUSH);
                        output("PUSHS bool@false");
                        output("JUMPIFEQS !%d_end_else_if_%d", else_if_count, id);
                         
                        generate_node(item->data.llist_item.node->data.else_if.else_if_block, GEN_PUSH);
                        output("JUMP !end_if_%d", id);
                        output("LABEL !%d_end_else_if_%d", else_if_count, id);
                        else_if_count++;
                }
            }

            // else block optional
            if(node->data.if_statement.else_block){
                output("LABEL !else_%d", id);
                generate_node(node->data.if_statement.else_block, GEN_PUSH);
            }
            output("LABEL !end_if_%d", id);
        
            break;
        }

        // returns nil@nil if it has no value
        case AST_RETURN: 
            if(node->data.return_node.value == NULL){
                output("PUSHS nil@nil");
                output("POPFRAME");
                output("RETURN");
                break;
            }
            generate_node(node->data.return_node.value, GEN_PUSH);
            output("POPFRAME");
            output("RETURN");
            break;

        case AST_DECLARATION:
            if (node->data.declaration.value != NULL){
                generate_node(node->data.declaration.value, GEN_PUSH);
                output("POPS LF@%s", node->data.declaration.codegen_name);
                if(mode == GEN_PUSH) output("PUSHS nil@nil");
            }
        default: break;
        
        case AST_FOR_LOOP:{
            int id = labelCounter++;
            generate_node(node->data.for_loop.gen_start, GEN_DISCARD);
            generate_node(node->data.for_loop.gen_end, GEN_PUSH);
            output("DEFVAR GF@&for_end%d", id);
            output("POPS GF@&for_end%d", id);

            output("DEFVAR GF@&for_step%d", id);

            // step is not mandatory, standard is 1
            if(node->data.for_loop.gen_step == NULL){
                output("MOVE GF@&for_step%d int@1", id);
            }
            // exit program if step == 0
            else{
                generate_node(node->data.for_loop.gen_step, GEN_PUSH);
                output("POPS GF@&for_step%d", id);
                output("JUMPIFNEQ !for_step_is_not_0_%d GF@&for_step%d int@0", id, id);
                output("EXIT int@40");
                output("LABEL !for_step_is_not_0_%d", id);
            }
            // skips first addition to i
            output("JUMP !for_loop_start_%d", id);

            output("LABEL !for_loop_increment_start_%d", id);
            // increment iteration counter
            output("ADD LF@%s LF@%s GF@&for_step%d", node->data.for_loop.gen_start->data.declaration.codegen_name, 
                                                    node->data.for_loop.gen_start->data.declaration.codegen_name, id);
            output("LABEL !for_loop_start_%d", id);
            // check for end of for loop based on (to / until)
            if(node->data.for_loop.gen_is_to){
                output("GT GF@&tmp LF@%s GF@&for_end%d", node->data.for_loop.gen_start->data.declaration.codegen_name, id);
            }
            else{
                output("LT GF@&tmp LF@%s GF@&for_end%d", node->data.for_loop.gen_start->data.declaration.codegen_name, id);
                output("NOT GF@&tmp GF@&tmp");
            }
            output("JUMPIFEQ !for_loop_end_%d GF@&tmp bool@true", id);
            
            // generate all guards
            // currently only skips iteration if guard is false (check) TODO
            for(ast_node_t *item = node->data.for_loop.llist_guards->data.llist.begin;
                item != NULL; item = item->data.llist_item.next)
                {
                generate_node(item->data.llist_item.node, GEN_PUSH);
                output("PUSHS bool@false");
                output("JUMPIFEQS !for_loop_increment_start_%d", id);
            }

            generate_node(node->data.for_loop.code_block, GEN_DISCARD);
            output("JUMP !for_loop_increment_start_%d", id);

            output("LABEL !for_loop_end_%d", id);
            if(mode == GEN_PUSH) output("PUSHS nil@nil");
        }
    }

    return GEN_SUCCESS;
}

/**
 * @brief Collects all variables found in a function
 * @param node Pointer to beginning of function code block
 */
static int collect_function_vars(ast_node_t *node){
    
    if(!node) return GEN_SUCCESS;

    switch(node->type) {
        case AST_BINARY:
            collect_function_vars(node->data.binary.left);
            collect_function_vars(node->data.binary.right);
            break;
        case AST_LINKED_LIST:
            for(ast_node_t *item = node->data.llist.begin; 
                item != NULL; item = item->data.llist_item.next)
                {
                    collect_function_vars(item->data.llist_item.node);
                }
            break;
        case AST_FUNC_CALL:
            collect_function_vars(node->data.func_call.parameters);
            break;
        case AST_WHILE_LOOP:
            collect_function_vars(node->data.while_statement.condition);
            collect_function_vars(node->data.while_statement.while_block);
            break;
        case AST_IF_STATEMENT:
            collect_function_vars(node->data.if_statement.condition);
            collect_function_vars(node->data.if_statement.if_block);
            collect_function_vars(node->data.if_statement.else_block);
            break;
        case AST_DECLARATION:
            if(add_var(node) != 0) return INTERNAL_ERR;
            collect_function_vars(node->data.declaration.value);
            break;
        case AST_RETURN:
            collect_function_vars(node->data.return_node.value);
            break;
        case AST_ELSE_IF:
            collect_function_vars(node->data.else_if.cond);
            collect_function_vars(node->data.else_if.else_if_block);
            break;
        case AST_FOR_LOOP:
            collect_function_vars(node->data.for_loop.gen_start);
            collect_function_vars(node->data.for_loop.gen_end);
            collect_function_vars(node->data.for_loop.gen_step);
            collect_function_vars(node->data.for_loop.llist_guards);
            collect_function_vars(node->data.for_loop.code_block);
            break;
        default: break;
    }
    return GEN_SUCCESS;
}

/**
 * @brief Checks name of variable for duplicates and defines it in IFJCode26
 * @param name Variable name
 */
static int add_var(ast_node_t *node){
    if(node->data.declaration.codegen_name != NULL){
        return GEN_SUCCESS;
    }
    bool found;
    void *value = (void *)1;

    if(hashtable_find_or_insert(hashtable, node->data.declaration.name, &found, &value) != 0){
        return INTERNAL_ERR;
    }
    if(found) varCount++;
    output("DEFVAR LF@%s$%d", node->data.declaration.name, varCount);
    char buffer[256];
    sprintf(buffer, "%s$%d", node->data.declaration.name, varCount);
    node->data.declaration.codegen_name = str_dup(buffer);

    return GEN_SUCCESS;
}
 
/**
 * @brief Pushes parameters of a function to the stack in reverse
 * @param node Pointer to first parameter
 */
static void push_parameters_reversed(const ast_node_t *node){
    if(node != NULL){
        push_parameters_reversed(node->data.llist_item.next);
        generate_node(node->data.llist_item.node, GEN_PUSH);
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
    int length = strlen(str);
    for(int i = 0; i < length; i++){
        if(str[i] < 33 || str[i] == 35 || str[i] == 92){
            fprintf(dst, "\\%03d", str[i]);
        }
        else{
            fprintf(dst, "%c", str[i]);
        }
    }
    fprintf(dst, "\n");
}   