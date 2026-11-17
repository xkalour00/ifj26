/**
 * @file ast.c
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - Implementation of an abstract syntax tree
 *
 * @date 2026-11-17
 */

#include "ast.h"
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

  switch (type) {
  case AST_UNIT:
    break;
  case AST_LEAF:
    node->data.leaf = data;
    break;
  case AST_INTEGER:
    node->data.integer = *(int64_t *)data;
    break;
  case AST_DOUBLE:
    node->data.fp = *(double *)data;
    break;
  case AST_STRING:
    node->data.str = data;
    break;
  case AST_RETURN:
    node->data.return_node.value = data;
    break;
  default:
    free(node);
    return NULL;
  }

  return node;
}

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
  node->data.root.main_func = NULL;
  node->data.root.func_def_list = func_llist;

  return node;
}

ast_node_t *make_binary_tree(ast_binary_type_t bin_op, ast_node_t *left,
                             ast_node_t *right) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_BINARY;
  node->data.binary.binary_type = bin_op;
  node->data.binary.left = left;
  node->data.binary.right = right;

  return node;
}

ast_node_t *make_if_else_tree(ast_node_t *cond, ast_node_t *if_block,
                              ast_node_t *else_block) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_IF_STATEMENT;
  node->data.if_statement.condition = cond;
  node->data.if_statement.if_block = if_block;
  node->data.if_statement.else_block = else_block;

  return node;
}

ast_node_t *make_while_tree(ast_node_t *cond, ast_node_t *while_block) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_WHILE_LOOP;
  node->data.while_statement.condition = cond;
  node->data.while_statement.while_block = while_block;

  return node;
}

ast_node_t *init_llist() {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_LINKED_LIST;
  node->data.llist.begin = NULL;
  node->data.llist.end = NULL;

  return node;
}

int add_llist_item(ast_node_t *llist, ast_node_t *item) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return -1;
  }

  node->type = AST_LINKED_LIST_ITEM;
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

ast_node_t *make_func_call_tree(char *func_name, ast_node_t *params) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_FUNC_CALL;
  node->data.func_call.parameters = params;
  node->data.func_call.name = func_name;

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
  node->data.func_def.return_type = return_type;
  node->data.func_def.name = func_name;
  node->data.func_def.parameters = params;
  node->data.func_def.tree_list = body;

  ast_node_t *llist = root_node->data.root.func_def_list;
  int append_result = add_llist_item(llist, node);

  if (append_result) {
    free(node);
    return -1;
  }

  if (is_main) {
    root_node->data.root.main_func = node;
  }

  return 0;
}

ast_node_t *make_declaration_node(char *name, return_type_t return_type,
                                  ast_node_t *value) {
  ast_node_t *node = malloc(sizeof(ast_node_t));

  if (!node) {
    return NULL;
  }

  node->type = AST_DECLARATION;
  node->data.declaration.name = name;
  node->data.declaration.return_type = return_type;
  node->data.declaration.value = value;

  return node;
}

void free_tree(ast_node_t *root) {
  if (!root) {
    return;
  }

  ast_node_t *node;

  switch (root->type) {
  case AST_INTEGER:
    break;
  case AST_DOUBLE:
    break;
  case AST_LEAF:
    free(root->data.leaf);
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
  case AST_IF_STATEMENT:
    free_tree(root->data.if_statement.condition);
    free_tree(root->data.if_statement.if_block);
    free_tree(root->data.if_statement.else_block);
    break;
  case AST_FUNC_DEF:
    free(root->data.func_def.name);
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

int check_type_validity(ast_node_t *root) {
  // TODO
  return -1;
}