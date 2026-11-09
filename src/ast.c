/**
 * @file ast.c
 *
 * @author Radek Kalous (xkalour00)
 * @brief Implementation of an abstract syntax tree
 * @date 2026-10-17
 */

#include "ast.h"
#include <stdlib.h>

/**
 * @brief Allocate memory and assign it a type based on argument type
 *
 * @param type Type of node to set
 * @return ast_node_t* Pointer to allocated node
 */
ast_node_t *allocate_assign_node(ast_node_type_t type) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = type;

  return node;
}

ast_node_t *make_node(ast_node_type_t type, void *data) {
  ast_node_t *node = allocate_assign_node(type);

  ast_node_t *func_llist; // Only for AST_ROOT case

  switch (type) {
  case AST_LEAF:
    node->data.leaf = data;
  case AST_INTEGER:
    node->data.integer = *(int *)data;
  case AST_FLOAT:
    node->data.fp = *(float *)data;
    break;
  case AST_ROOT:
    func_llist = init_llist();
    if (!func_llist) {
      free(node);
      return NULL;
    }
    node->data.root.main_func = NULL;
    node->data.root.func_def_list = func_llist;
    break;
  default:
    free(node);
    return NULL;
  }

  return node;
}

ast_node_t *make_root_node() {
  ast_node_t *root = make_node(AST_ROOT, NULL);

  return root;
}

ast_node_t *make_binary_tree(ast_binary_type_t bin_op, ast_node_t *left,
                             ast_node_t *right) {
  ast_node_t *node = allocate_assign_node(AST_BINARY);

  if (!node) {
    return NULL;
  }

  node->data.binary.binary_type = bin_op;
  node->data.binary.left = left;
  node->data.binary.right = right;
  return node;
}

ast_node_t *make_if_else_tree(ast_node_t *cond, ast_node_t *if_block,
                              ast_node_t *else_block) {
  ast_node_t *node = allocate_assign_node(AST_IF_STATEMENT);

  if (!node) {
    return NULL;
  }

  node->data.if_statement.condition = cond;
  node->data.if_statement.if_block = if_block;
  node->data.if_statement.else_block = else_block;

  return node;
}

ast_node_t *make_while_tree(ast_node_t *cond, ast_node_t *while_block) {
  ast_node_t *node = allocate_assign_node(AST_WHILE_LOOP);

  if (!node) {
    return NULL;
  }

  node->data.while_statement.condition = cond;
  node->data.while_statement.while_block = while_block;

  return node;
}

ast_node_t *init_llist() {
  ast_node_t *node = allocate_assign_node(AST_LINKED_LIST);

  if (!node) {
    return NULL;
  }

  node->data.llist.begin = NULL;
  node->data.llist.end = NULL;

  return node;
}

int add_llist_item(ast_node_t *llist, ast_node_t *item) {
  ast_node_t *node = allocate_assign_node(AST_LINKED_LIST_ITEM);

  if (!node) {
    return -1;
  }

  node->data.llist_item.node = item;
  node->data.llist_item.next = NULL;

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

ast_node_t *make_func_call_tree(ast_node_t *func_call, ast_node_t *param) {
  // TODO
  return NULL;
}

int add_func_def_tree(ast_node_t *root_node, ast_node_t *func_root,
                      bool is_main) {
  // TODO
  return -1;
}

void free_tree(ast_node_t *root) {
  // TODO
}