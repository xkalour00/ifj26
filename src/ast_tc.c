/**
 * @file ast_tc.c
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - Implementation of AST type-checking
 * function and related functions
 *
 * @date 2026-11-21
 */

#include "ast.h"
#include "stack.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Check particular function for recursiveness
 *
 * @param node Node of the function definition
 * @param ht Hash table storing reachable functions
 * @param symtable Pointer to symbol table
 */
static void pre_tc_check_for_recursion(ast_node_t *node, void *ht,
                                       void *symtable) {
  size_t idx = 0;
  stack_t *stack = node->data.func_def.dir_reach_funcs;
  char *func_name = stack_get(stack, idx++);

  while (func_name) {
    // TODO: If func_name is **NOT** in ht, add it and call again
    // pre_tc_check_for_recursion() with func_name as new node (resolve new node
    // though symtable), if it is in ht, set func_def.possible_recursion to true
    // and return
    // FIXME: remove these two lines, this in only temporary to shut up gcc
    ht = ht;
    symtable = symtable;

    // Get next func_name in stack
    func_name = stack_get(stack, idx++);
  }
} // static void pre_tc_check_for_recursion(ast_node_t *node, void *ht,
  //                                   void *symtable)

/**
 * @brief Set functions's possible_recursion flag if the function is recursive
 *
 * @param root AST root node
 * @param symtable Pointer to symbol table
 * @return int Return code; 0 on success, non-zero on failure
 */
static int pre_tc_set_recursion_flags(ast_node_t *root, void *symtable) {
  if (!root) {
    return -1;
  }

  // Get function list
  ast_node_t *func_llist = root->data.root.func_def_list;

  if (!func_llist) {
    return -1;
  }

  ast_node_t *func_item = func_llist->data.llist.begin;
  ast_node_t *func;

  // Loop all functions and set its flags
  while (func_item) {
    func = func_item->data.llist_item.node;

    // TODO: init hashtable storing reachable functions
    void *reachable_funcs = NULL;

    pre_tc_check_for_recursion(func, reachable_funcs, symtable);

    // TODO: free reachable_funcs hashtable

    func_item = func_item->data.llist_item.next;
  }

  return 0;
} // static int pre_tc_set_recursion_flags(ast_node_t *root, void *symtable)

/**
 * @brief Pushes string str if stack does not already contain it
 *
 * @param stack
 * @param str
 * @return int
 */
static int push_if_not_present(stack_t *stack, char *str) {
  size_t idx = 0;
  char *str_stack = stack_get(stack, idx++);

  // Loop while stack has something at index idx
  while (str_stack) {
    // If strings match, str is present at stack and we do not push
    if (!strcmp(str, str_stack)) {
      return 0;
    }
    str_stack = stack_get(stack, idx++);
  }

  char *str_malloced = malloc(sizeof(char) * (strlen(str) + 1));

  if (!str_malloced) {
    return -1;
  }

  strcpy(str_malloced, str);

  stack_push(stack, str_malloced);

  return 0;
} // static int push_if_not_present(stack_t *stack, char *str)

int search_for_dr_funcs_return(stack_t *dr_funcs, bool *ret_node_flag,
                               ast_node_t *node) {
  ast_node_t *item;

  switch (node->type) {
  case AST_INTEGER:
  case AST_DOUBLE:
  case AST_LEAF:
  case AST_STRING:
    // We won't find function calls in here
    break;
  case AST_BINARY:
    search_for_dr_funcs_return(dr_funcs, ret_node_flag, node->data.binary.left);
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.binary.right);
    break;
  case AST_LINKED_LIST:
    item = node->data.llist.begin;
    while (item) {
      search_for_dr_funcs_return(dr_funcs, ret_node_flag, item);
      item = item->data.llist_item.next;
    }
    break;
  case AST_LINKED_LIST_ITEM:
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.llist_item.node);
    break;
  case AST_WHILE_LOOP:
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.while_statement.condition);
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.while_statement.while_block);
    break;
  case AST_IF_STATEMENT:
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.if_statement.condition);
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.if_statement.if_block);
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.if_statement.else_block);
    break;
  case AST_FUNC_DEF:
    // Can function be defined inside another function? For now, no.
    break;
  case AST_FUNC_CALL:
    // Only here, we add to the hashtable (if not already present)
    push_if_not_present(dr_funcs, node->data.func_call.name);
    break;
  case AST_ROOT:
    // We probably (certainly) won't find another AST in function definition
    break;
  case AST_RETURN:
    // Presence of at least one return node raises flag
    *ret_node_flag = true;
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.return_node.value);
    break;
  case AST_DECLARATION:
    search_for_dr_funcs_return(dr_funcs, ret_node_flag,
                               node->data.declaration.value);
    break;
  case AST_UNIT:
    // There's literally nothing to look for
    break;
  }

  return 0;
} // int search_for_dr_funcs_return(stack_t *dr_funcs, bool *ret_node_flag,
  //                           ast_node_t *node)

/**
 * @brief Check function for return type validity
 *
 * @param root AST root node
 * @return int Return code; 0 on success, non-zero on failure
 */
static int check_func_ret_type_validity(ast_node_t *root) {
  ast_node_t *func_llist = root->data.root.func_def_list;

  if (!func_llist) {
    return -1;
  }

  ast_node_t *func_item = func_llist->data.llist.begin;
  ast_node_t *func;

  while (func_item) {
    func = func_item->data.llist_item.node;
    bool is_recursive = func->data.func_def.possible_recursion;
    bool has_return_node = func->data.func_def.has_return_node;
    bool has_ret_unknown = func->data.func_def.return_type == RETURN_UNKNOWN;

    if ((is_recursive || has_return_node) && has_ret_unknown) {
      // TODO: This results in type error,
      // function must have explicitly defined
      // return type if function is recursive
      // (either directly or indirectly) or has
      // anywhere "return"
      return -1;
    }

    func_item = func_item->data.llist_item.next;
  }

  return 0;
} // static int
  // check_func_ret_type_validity(ast_node_t
  // *root)

// TODO: replace symtable type with actual type
// after merge
int check_type_validity(ast_node_t *root, void *symtable) {
  // Notes what we need to watch out for:

  // if definition does not have defined type, it
  // must be able to *statically* infer its type,
  // else error code 6 (error handling not yet
  // implemented) attempt to assign wrong type to
  // variable leads to error code 6

  // in function that has return type "Unit"
  // explicitly defined, lone "return" keyword is
  // allowed and returns "()" any computed result
  // is ignored and "()" is returned

  // if no return is supplied, last line's value
  // is returned if last line is variable
  // definition, variable assignment, if
  // statement or while loop, return value is
  // "()" (Unit)

  // if function has direct or indirect recursion
  // or has a "return" keyword anywhere in the
  // function's body, function MUST have
  // explicitly defined return type, else type is
  // infered

  // recursion check for function foo() - get all
  // functions foo() might call, to a stack
  // maybe. check whether foo() is not in the
  // stack, if it is - direct recursion, free
  // stack and return that function is directly
  // recursive. if it is not, initialize another
  // stack, enter loop if the stack is not empty.
  // pop one function from the stack to a
  // variable enter the definition of the
  // function in a variable and add this function
  // to second stack, functions in this stack are
  // considered "explored". again, check what the
  // function might call and if it matches foo()
  // - indirect recustion, if not and the called
  // function are not already in the first stack,
  // add them. keep popping and exploring
  // functions till indirect recursion is
  // confirmed, else recursion is not possible.

  pre_tc_set_recursion_flags(root, symtable);
  check_func_ret_type_validity(root);

  return -1;
} // int check_type_validity(ast_node_t *root,
  // void *symtable)