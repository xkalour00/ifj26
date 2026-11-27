/**
 * @file ast.c
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - Implementation of an abstract syntax tree
 *
 * @date 2026-11-27
 */

#include "ast.h"
#include "ast_tc.h"
#include "stack.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ast_node_t *make_node(ast_node_type_t type, void *data) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = type;
  node->ret_type = RETURN_UNKNOWN;
  node->to_double = false;

  switch (type) {
  case AST_UNIT:
    node->ret_type = RETURN_UNIT;
    break;
  case AST_LEAF:
    node->data.leaf = data;
    break;
  case AST_INTEGER:
    node->ret_type = RETURN_INT;
    node->data.integer = *(int64_t *)data;
    break;
  case AST_DOUBLE:
    node->ret_type = RETURN_DOUBLE;
    node->data.fp = *(double *)data;
    break;
  case AST_STRING:
    node->ret_type = RETURN_STRING;
    node->data.str = data;
    break;
  case AST_RETURN:
    node->data.return_node.value = data;
    break;
  case AST_BOOLEAN:
    node->ret_type = RETURN_BOOLEAN;
    node->data.bool_value = *(bool *)data;
    break;
  default:
    free(node);
    return NULL;
  }

  return node;
} // ast_node_t *make_node(ast_node_type_t type, void *data)

ast_node_t *make_root_node() {
  ast_node_t *node = malloc(sizeof(ast_node_t));
  ast_node_t *func_llist = init_llist();

  if (!node) {
    free(func_llist);
    return NULL;
  }

  if (!func_llist) {
    free(node);
    return NULL;
  }

  node->type = AST_ROOT;
  node->ret_type = RETURN_UNKNOWN;
  node->data.root.main_func = NULL;
  node->data.root.func_def_list = func_llist;
  node->to_double = false;

  return node;
}

ast_node_t *make_binary_tree(ast_binary_type_t bin_op, ast_node_t *left,
                             ast_node_t *right) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_BINARY;
  node->ret_type = RETURN_UNKNOWN;
  node->data.binary.binary_type = bin_op;
  node->data.binary.left = left;
  node->data.binary.right = right;
  node->to_double = false;

  // Assign binary class depending on the exact type
  switch (bin_op) {
  case AST_ADDITION:
  case AST_SUBTRACTION:
  case AST_MULTIPLICATION:
  case AST_DIVISION:
    node->data.binary.binary_class = AST_BIN_ARITH;
    break;
  case AST_ASSIGNMENT:
    node->data.binary.binary_class = AST_BIN_ASSIGN;
    break;
  case AST_EQUAL:
  case AST_NEQUAL:
  case AST_LEQUAL:
  case AST_GEQUAL:
  case AST_LESS:
  case AST_GREATER:
  case AST_NOT:
  case AST_AND:
  case AST_OR:
    node->data.binary.binary_class = AST_BIN_LOGIC;
    break;
  }

  return node;
}

ast_node_t *make_if_else_tree(ast_node_t *cond, ast_node_t *if_block,
                              ast_node_t *elsifs, ast_node_t *else_block) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_IF_STATEMENT;
  node->ret_type = RETURN_UNKNOWN;
  node->data.if_statement.condition = cond;
  node->data.if_statement.if_block = if_block;
  node->data.if_statement.else_block = else_block;
  node->data.if_statement.llist_else_if = elsifs;
  node->to_double = false;

  return node;
}

ast_node_t *make_else_if_node(ast_node_t *cond, ast_node_t *block) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_ELSE_IF;
  node->ret_type = RETURN_UNKNOWN;
  node->data.else_if.cond = cond;
  node->data.else_if.else_if_block = block;
  node->to_double = false;

  return node;
}

ast_node_t *make_while_tree(ast_node_t *cond, ast_node_t *while_block) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_WHILE_LOOP;
  node->ret_type = RETURN_UNKNOWN;
  node->data.while_statement.condition = cond;
  node->data.while_statement.while_block = while_block;
  node->to_double = false;

  return node;
}

ast_node_t *make_for_loop_node(ast_node_t *i_node, ast_node_t *end,
                               ast_node_t *step, bool is_to,
                               ast_node_t *llguards, ast_node_t *block) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_FOR_LOOP;
  node->ret_type = RETURN_UNKNOWN;
  node->data.for_loop.gen_start = i_node;
  node->data.for_loop.gen_end = end;
  node->data.for_loop.gen_step = step;
  node->data.for_loop.gen_is_to = is_to;
  node->data.for_loop.llist_guards = llguards;
  node->data.for_loop.code_block = block;

  node->to_double = false;

  return node;
}

ast_node_t *init_llist() {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_LINKED_LIST;
  node->ret_type = RETURN_UNKNOWN;
  node->data.llist.begin = NULL;
  node->data.llist.end = NULL;
  node->to_double = false;

  return node;
}

int add_llist_item(ast_node_t *llist, ast_node_t *item) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return -1;
  }

  node->type = AST_LINKED_LIST_ITEM;
  node->ret_type = RETURN_UNKNOWN;
  node->data.llist_item.node = item;
  node->data.llist_item.next = NULL;
  node->to_double = false;

  // If llist is empty
  if (!llist->data.llist.begin) {
    llist->data.llist.begin = node;
    llist->data.llist.end = node;
  } else {
    ast_node_t *end_node = llist->data.llist.end;

    llist->data.llist.end = node;
    end_node->data.llist_item.next = node;
  }

  return 0;
}

ast_node_t *make_func_call_tree(char *func_name, ast_node_t *params) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_FUNC_CALL;
  node->ret_type = RETURN_UNKNOWN;
  node->data.func_call.parameters = params;
  node->data.func_call.name = func_name;
  node->to_double = false;

  return node;
}

int add_func_def_tree(ast_node_t *root_node, return_type_t return_type,
                      char *func_name, ast_node_t *params, ast_node_t *body,
                      bool is_main) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return -1;
  }

  node->type = AST_FUNC_DEF;
  node->ret_type = RETURN_UNKNOWN;
  node->data.func_def.return_type = return_type;
  node->data.func_def.possible_recursion = false;
  node->data.func_def.has_return_node = false;
  stack_t **dr_funcs = &(node->data.func_def.dir_reach_funcs);
  stack_t **ret_nodes = &(node->data.func_def.return_nodes);
  node->data.func_def.name = func_name;
  node->data.func_def.parameters = params;
  node->data.func_def.tree_list = body;
  node->to_double = false;
  node->data.func_def.function_walked = false;

  // Initialize stack that holds dynamically allocated strings
  if (stack_init(dr_funcs, sizeof(char *))) {
    free(func_name);
    free(node);
    return -1;
  }

  if (stack_init(ret_nodes, sizeof(ast_node_t *))) {
    stack_free(*dr_funcs);
    free(func_name);
    free(node);
    return -1;
  }

  if (walk_tree_meta(NULL, node)) {
    stack_free(*dr_funcs);
    stack_free(*ret_nodes);
    free(func_name);
    free(node);
    return -1;
  }

  // If function is main and no main was made before
  if (is_main && !root_node->data.root.main_func) {
    root_node->data.root.main_func = node;
  }
  // If function is main and main is already assigned
  else if (is_main && root_node->data.root.main_func) {
    stack_free(*dr_funcs);
    stack_free(*ret_nodes);
    free(func_name);
    free(node);
    // Return code as defined in specification
    return 4;
  }

  ast_node_t *llist = root_node->data.root.func_def_list;

  if (add_llist_item(llist, node)) {
    stack_free(*dr_funcs);
    stack_free(*ret_nodes);
    free(func_name);
    free(node);
    return -1;
  }

  return 0;
} // int add_func_def_tree(ast_node_t *root_node, return_type_t return_type,
  //                  char *func_name, ast_node_t *params, ast_node_t *body,
  //                  bool is_main)

ast_node_t *make_declaration_node(char *name, return_type_t return_type,
                                  ast_node_t *value) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_DECLARATION;
  node->ret_type = RETURN_UNKNOWN;
  node->data.declaration.name = name;
  node->data.declaration.codegen_name = NULL;
  node->data.declaration.return_type = return_type;
  node->data.declaration.value = value;
  node->to_double = false;

  return node;
}

void free_tree(ast_node_t *root) {
  if (!root) {
    return;
  }

  ast_node_t *node;

  switch (root->type) {
  case AST_INTEGER:
  case AST_DOUBLE:
  case AST_LEAF:
  case AST_BOOLEAN:
    break;
  case AST_STRING:
    free(root->data.str);
    break;
  case AST_BINARY:
    free_tree(root->data.binary.left);
    free_tree(root->data.binary.right);
    break;
  case AST_LINKED_LIST:
    node = root->data.llist.begin;
    // Free items till we have none
    while (node) {
      ast_node_t *next = node->data.llist_item.next;
      free_tree(node);
      node = next;
    }
    break;
  case AST_LINKED_LIST_ITEM:
    free_tree(root->data.llist_item.node);
    break;
  case AST_WHILE_LOOP:
    free_tree(root->data.while_statement.condition);
    free_tree(root->data.while_statement.while_block);
    break;
  case AST_FOR_LOOP:
    free_tree(root->data.for_loop.gen_start);
    free_tree(root->data.for_loop.gen_end);
    free_tree(root->data.for_loop.gen_step);
    free_tree(root->data.for_loop.llist_guards);
    free_tree(root->data.for_loop.code_block);
    break;
  case AST_IF_STATEMENT:
    free_tree(root->data.if_statement.condition);
    free_tree(root->data.if_statement.if_block);
    free_tree(root->data.if_statement.else_block);
    free_tree(root->data.if_statement.llist_else_if);
    break;
  case AST_ELSE_IF:
    free_tree(root->data.else_if.cond);
    free_tree(root->data.else_if.else_if_block);
    break;
  case AST_FUNC_DEF:
    free(root->data.func_def.name);
    stack_free(root->data.func_def.dir_reach_funcs);
    stack_free(root->data.func_def.return_nodes);
    free_tree(root->data.func_def.tree_list);
    free_tree(root->data.func_def.parameters);
    break;
  case AST_FUNC_CALL:
    free(root->data.func_call.name);
    free_tree(root->data.func_call.parameters);
    break;
  case AST_RETURN:
    free_tree(root->data.return_node.value);
    break;
  case AST_DECLARATION:
    free_tree(root->data.declaration.value);
    free(root->data.declaration.name);
    break;
  case AST_UNIT:
    break;
  case AST_ROOT:
    free_tree(root->data.root.func_def_list);
    break;
  } // switch (root->type)

  // After every child is freed, free itself
  free(root);
} // void free_tree(ast_node_t *root)