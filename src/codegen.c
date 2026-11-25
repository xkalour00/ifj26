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

#define MAX_VARS 1000

static char *varNames[MAX_VARS];
static int varCount = 0;
static FILE *dst;
static int labelCounter = 0;

static int generate_node(const ast_node_t *node, const gen_mode mode);
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
    output("DEFVAR GF@for_start");
    output("DEFVAR GF@for_step");
    output("DEFVAR GF@for_end");
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
            generate_node(item->data.llist_item.node, GEN_DISCARD);
        }
    return 0;
}

/**
 * @brief Processes a node from the AST
 * @param node Pointer to node, waiting to be processed
 * @return Returns error code
 */
static int generate_node(const ast_node_t *node, const gen_mode mode){

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
        // case AST_BOOL:
        //     output("PUSHS bool@%s", node->data.bool_value ? "true" : "false");
        //     break;
        
        // assignment is special from the rest, does not need to generate left node
        case AST_BINARY: 
            if (node->data.binary.binary_type == AST_ASSIGNMENT) {
                generate_node(node->data.binary.right, GEN_PUSH);
                output("POPS LF@%s", node->data.binary.left->data.leaf);
                if(mode == GEN_PUSH) output("PUSHS nil@nil");
                break;
            }

            // if(node->data.binary.binary_type == AST_AND){
            //     generate_node(node->data.binary.left);
            //     output("PUSHS bool@false");
            //     output("JUMPIFEQS ast_and_short_circuit");
            //     output("PUSHS bool@true");
            //     generate_node(node->data.binary.right);
            //     output("ANDS");
            //     output("JUMP ast_and_end");
            //     output("LABEL ast_and_short_circuit");
            //     output("PUSHS bool@false");
            //     output("LABEL ast_and_end");
            //     if(mode == GEN_DISCARD) output("POPS GF@tmp");
            //     break;
            // }

            // if(node->data.binary.binary_type == AST_OR){
            //     generate_node(node->data.binary.left);
            //     output("PUSHS bool@true");
            //     output("JUMPIFEQS ast_or_short_circuit");
            //     output("PUSHS bool@false");
            //     generate_node(node->data.binary.right);
            //     output("ORS");
            //     output("JUMP ast_or_end");
            //     output("LABEL ast_or_short_circuit");
            //     output("PUSHS bool@true");
            //     output("LABEL ast_or_end");
            //     if(mode == GEN_DISCARD) output("POPS GF@tmp");
            //     break;
            // }

            // preorder traversal
            generate_node(node->data.binary.left, GEN_PUSH);
            generate_node(node->data.binary.right, GEN_PUSH);

            switch(node->data.binary.binary_type){
                case AST_ADDITION:  {
                    // if(node->return_type == AST_STRING){
                    //     output("POPS GF@concat_string_tmp2");
                    //     output("POPS GF@concat_string_tmp1");
                    //     output("CONCAT GF@concat_string_tmp1 GF@concat_string_tmp1 GF@concat_string_tmp2");
                    //     output("PUSHS GF@concat_string_tmp1");
                    // }
                    // else{
                    //     output("ADDS");
                    // }
                    // if(mode == GEN_DISCARD) output("POPS GF@tmp");
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
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                case AST_MULTIPLICATION: 
                    output("MULS"); 
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                case AST_DIVISION: 
                    // if(node->return_type == AST_INTEGER) output("IDIVS");
                    // else 
                    output("DIVS"); 
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                case AST_EQUAL:
                    output("EQS");
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;
                
                case AST_NEQUAL:
                    output("EQS");
                    output("NOTS");
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                case AST_LEQUAL: 
                    output("GTS");
                    output("NOTS");
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                case AST_GEQUAL:
                    output("LTS");
                    output("NOTS");
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                case AST_LESS:
                    output("LTS");
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                case AST_GREATER:
                    output("GTS");
                    if(mode == GEN_DISCARD) output("POPS GF@tmp");
                    break;

                // case AST_NOT:
                //     output("NOTS");
                //     if(mode == GEN_DISCARD) output("POPS GF@tmp");
                //     break;

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
            generate_node(node->data.func_def.tree_list, GEN_PUSH);
            output("");
            break;

        // pushes the last parameter first, so that first parameter is on top of the stack
        case AST_FUNC_CALL:
            if(node->data.func_call.parameters){
                push_parameters_reversed(node->data.func_call.parameters->data.llist.begin);
            }
            output("CALL func_%s", node->data.func_call.name);
            if(mode == GEN_DISCARD) output("POPS GF@tmp");
            break;

        case AST_WHILE_LOOP: {
            int id = labelCounter++;
            
            output("LABEL while%d", id);
            generate_node(node->data.while_statement.condition, GEN_PUSH);
            output("PUSHS bool@false");
            output("JUMPIFEQS end_while%d", id);

            generate_node(node->data.while_statement.while_block, GEN_DISCARD);
            output("JUMP while%d", id);
            output("LABEL end_while%d", id);

            if(mode == GEN_PUSH) output("PUSHS nil@nil");
            break;
        }

        case AST_IF_STATEMENT: {
            int id = labelCounter++;

            generate_node(node->data.if_statement.condition, GEN_PUSH);
            output("PUSHS bool@false");
            if(node->data.if_statement.else_block){
                output("JUMPIFEQS else%d", id);
            }
            else{
                output("JUMPIFEQS end_if%d", id);
            }
            generate_node(node->data.if_statement.if_block, GEN_PUSH);
            output("JUMP end_if%d", id);

            // if(node->data.if_statement.else_if_block){
                // int else_if_count = 0;
                // for(ast_node_t *item = node->data.if_statement.else_if_block->data.llist.begin;
                //     item != NULL; item = item->data.llist_item.next)
                //     {
                //         generate_node(item->data.llist_item.node->data.else_if_statement.cond, GEN_PUSH);
                //         output("PUSHS bool@false");
                //         output("JUMPIFEQS %d_end_else_if%d", else_if_count, id);
                //          
                //         generate_node(item->data.llist_item.node->data.else_if_statement.else_if_block, GEN_PUSH);
                //         output("JUMP end_if%d", id);
                //         output("LABEL %d_end_else_if%d", else_if_count, id);
                //         else_if_count++;
                // }
            // }

            // else block optional
            if(node->data.if_statement.else_block){
                output("LABEL else%d", id);
                generate_node(node->data.if_statement.else_block, GEN_PUSH);
                output("LABEL end_if%d", id);
            }
        
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
                output("POPS LF@%s", node->data.declaration.name);
                if(mode == GEN_PUSH) output("PUSHS nil@nil");
            }
        default: break;
        
        case AST_FOR_LOOP:{
            int id = labelCounter++;
            generate_node(node->data.for_loop.gen_start, GEN_PUSH);
            output("DEFVAR GF@for_start%d", id);
            output("POPS GF@for_start%d", id);
            generate_node(node->data.for_loop.gen_end, GEN_PUSH);
            output("DEFVAR GF@for_end%d", id);
            output("POPS GF@for_end%d", id);

            output("DEFVAR GF@for_step%d", id);

            // step is not mandatory, standard is 1
            if(node->data.for_loop.gen_step == NULL){
                output("MOVE GF@for_step%d int@1", id);
            }
            // exit program if step == 0
            else{
                generate_node(node->data.for_loop.gen_step, GEN_PUSH);
                output("POPS GF@for_step%d", id);
                output("JUMPIFNEQ for_step_is_not_0%d GF@for_step int@0", id);
                output("EXIT int@40");
                output("LABEL for_step_is_not_0%d", id);
            }
            output("JUMP for_loop_start%d", id);

            output("LABEL for_loop_increment_start%d", id);
            // increment iteration counter
            output("ADD GF@for_start%d GF@for_start%d GF@for_step%d", id, id, id);
            output("LABEL for_loop_start%d", id);
            // check for end of for loop based on (to / until)
            if(node->data.for_loop.gen_is_to){
                output("GT GF@tmp GF@for_start%d GF@for_end%d", id, id);
            }
            else{
                output("LT GF@tmp GF@for_start%d GF@for_end%d", id, id);
                output("NOT GF@tmp GF@tmp");
            }
            output("JUMPIFEQ for_loop_end%d GF@tmp bool@true", id);
            
            // generate all guards
            // currently only skips iteration if guard is false
            for(ast_node_t *item = node->data.for_loop.llist_guards->data.llist.begin;
                item != NULL; item = item->data.llist_item.next)
                {
                generate_node(item->data.llist_item.node->data.if_statement.condition, GEN_PUSH);
                output("PUSHS bool@false");
                output("JUMPIFEQS for_loop_increment_start%d", id);
            }

            generate_node(node->data.for_loop.code_block, GEN_DISCARD);
            output("JUMP for_loop_increment_start%d", id);

            output("LABEL for_loop_end%d", id);
            if(mode == GEN_PUSH) output("PUSHS nil@nil");
        }
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
        case AST_ELSE_IF_STATEMENT:
            generate_function_vars(node->data.else_if.cond);
            generate_function_vars(node->data.else_if.else_if_block);
        case AST_FOR_LOOP:
            generate_function_vars(node->data.for_loop.gen_start);
            generate_function_vars(node->data.for_loop.gen_end);
            generate_function_vars(node->data.for_loop.gen_step);
            generate_function_vars(node->data.for_loop.llist_guards);
            generate_function_vars(node->data.for_loop.code_block);
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