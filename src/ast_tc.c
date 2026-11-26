/**
 * @file ast_tc.c
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - Implementation of AST type-checking
 * function and related functions
 *
 * @date 2026-11-26
 */

#include "ast_tc.h"
#include "ast.h"
#include "hashtable.h"
#include "stack.h"
#include "symtable.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Check particular function for recursiveness
 *
 * @param node Node of the function definition
 * @param ht Hash table storing reachable functions
 * @param sym Pointer to symbol table
 * @param checked_func Whether node is the function checked for recursiveness
 * @return int Return code; 0 on success, non-zero on failure
 */
int check_for_recursion(ast_node_t *node, hashtable_t *ht, symtable_t *sym,
                        bool checked_func) {
  size_t idx = 0;
  stack_t *stack = node->data.func_def.dir_reach_funcs;
  char *func_name = stack_get(stack, idx++);

  while (func_name) {
    // val is not to be used
    void *val;
    int ret = hashtable_find(ht, func_name, &val);
    symbol_t *symbol = symtable_find(sym, func_name);

    if (!symbol) {
      // Symbol not found
      return -1;
    }
    if (ret == 0) {
      // Function found
      break;
    } else if (ret == 1) {
      // Function not found
      hashtable_insert(ht, func_name, NULL);
      check_for_recursion(symbol->declaration, ht, sym, false);
    } else {
      // Hashtable lookup error
      return -1;
    }

    // Get next func_name in stack
    func_name = stack_get(stack, idx++);
  }

  if (!checked_func) {
    return 0;
  }

  // val is not to be used
  void *val;
  int ret = hashtable_find(ht, node->data.func_def.name, &val);
  if (ret == 0) {
    node->data.func_def.possible_recursion = true;
  }

  return 0;
} // int check_for_recursion(ast_node_t *node, hashtable_t *ht, symtable_t *sym,
  //                         bool checked_func)

/**
 * @brief Set functions's possible_recursion flag if the function is
 * recursive
 *
 * @param root AST root node
 * @param symtable Pointer to symbol table
 * @return int Return code; 0 on success, non-zero on failure
 */
int set_recursion_flags(ast_node_t *root, symtable_t *symtable) {
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

    hashtable_t *reachable_funcs;
    if (hashtable_init(&reachable_funcs)) {
      return -1;
    }

    check_for_recursion(func, reachable_funcs, symtable, true);

    hashtable_free(reachable_funcs);

    func_item = func_item->data.llist_item.next;
  }

  return 0;
} // int set_recursion_flags(ast_node_t *root, symtable_t *symtable)

/**
 * @brief Pushes string str if stack does not
 * already contain it
 *
 * @param stack Stack where to push
 * @param str String to push
 * @return int Return code; 0 if present or
 * successfully pushed, non-zero otherwise
 */
int push_if_not_present(stack_t *stack, char *str) {
  size_t idx = 0;
  char *str_stack = stack_get(stack, idx++);

  // Loop while stack has something at index idx
  while (str_stack) {
    // If strings match, str is present at stack
    // and we do not push
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
} // int push_if_not_present(stack_t *stack, char *str)

/**
 * @brief
 *
 * @param parent
 * @param left
 * @param right
 * @return int
 */
int figure_implicit_conv(ast_node_t *parent, ast_node_t *left,
                         ast_node_t *right) {
  return_type_t left_type = left->ret_type;
  return_type_t right_type = left->ret_type;

  if (left_type == RETURN_INT && right_type == RETURN_DOUBLE) {
    // Implicit conversion occurrs in left node
    left->data.integer.to_double = true;
    parent->ret_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_DOUBLE && right_type == RETURN_INT) {
    // Implicit conversion occurrs in right node
    right->data.integer.to_double = true;
    parent->ret_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_INT && right_type == RETURN_INT) {
    // Parent node receives the same type
    parent->ret_type = RETURN_INT;
  } else if (left_type == RETURN_DOUBLE && right_type == RETURN_DOUBLE) {
    parent->ret_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_STRING && right_type == RETURN_STRING) {
    parent->ret_type = RETURN_STRING;
  } else {
    // TODO: print error
    return -1;
  }

  return 0;
}

/**
 * @brief
 *
 * @param parent
 * @param node
 * @param symtable
 * @return int
 */
int walk_binary(ast_node_t *parent, ast_node_t *node, symtable_t *symtable) {
  ast_node_t *left_node = node->data.binary.left;
  ast_node_t *right_node = node->data.binary.right;
  ast_binary_class_t class = node->data.binary.binary_class;

  if (walk_tree(parent, left_node, symtable)) {
    return -1;
  }
  if (walk_tree(parent, right_node, symtable)) {
    return -1;
  }

  switch (class) {
  case AST_BIN_ARITH:
    return figure_implicit_conv(node, left_node, right_node);
    break;
  case AST_BIN_LOGIC:
    // TODO: logic should be boolean ret_type or something
    break;
  case AST_BIN_ASSIGN:
    node->ret_type = RETURN_UNIT;
    break;
  }

  return 0;
}

int walk_tree(ast_node_t *parent, ast_node_t *line, symtable_t *symtable) {
  if (!line || line->ret_type != RETURN_UNKNOWN) {
    return 0;
  }

  switch (line->type) {
  case AST_INTEGER:
  case AST_DOUBLE:
  case AST_STRING:
  case AST_UNIT:
    // Type was already set when the node was
    // created
    break;
  case AST_LEAF:
    line->ret_type =
        line->data.leaf->declaration->data.declaration.value->ret_type;
    break;
  case AST_BINARY:
    return walk_binary(parent, line, symtable);
    break;
  case AST_LINKED_LIST:
    return walk_tree(parent, line->data.llist.begin, symtable);
  case AST_LINKED_LIST_ITEM:
    if (walk_tree(parent, line->data.llist_item.node, symtable)) {
      return -1;
    }
    return walk_tree(parent, line->data.llist_item.next, symtable);
  case AST_WHILE_LOOP:
    if (walk_tree(parent, line->data.while_statement.condition, symtable)) {
      return -1;
    }
    if (walk_tree(parent, line->data.while_statement.while_block, symtable)) {
      return -1;
    }
    line->ret_type = RETURN_UNIT;
    break;
  case AST_IF_STATEMENT:
    if (walk_tree(parent, line->data.if_statement.condition, symtable)) {
      return -1;
    }
    if (walk_tree(parent, line->data.if_statement.if_block, symtable)) {
      return -1;
    }
    if (walk_tree(parent, line->data.if_statement.else_block, symtable)) {
      return -1;
    }
    line->ret_type = RETURN_UNIT;
    // TODO: In CONTROL extension, it can become
    // other type than Unit
    break;
  case AST_FUNC_DEF:
    if (walk_tree(line, line->data.func_def.tree_list, symtable)) {
      return -1;
    }
    // TODO: if return was unspecified
    break;
  case AST_FUNC_CALL:
    if (parent->data.func_def.first_walk) {
      // Only try to insert the name if we know we're
      // walking the function for the first time
      if (push_if_not_present(parent->data.func_def.dir_reach_funcs,
                              line->data.func_call.name)) {
        return -1;
      }
      return 0;
    }

    if (walk_tree(parent, line->data.func_call.parameters, symtable)) {
      return -1;
    }
    symbol_t *sym = symtable_find(symtable, line->data.func_call.name);
    if (!sym || !sym->is_function) {
      return -1;
    }
    if (walk_tree(parent, sym->declaration, symtable)) {
      return -1;
    }
    // TODO: we must check for type consistency
    // in parameters, throw error or set implicit
    // type conversion
    // TODO: must check for return type of
    // func_call's func_def. if set, set it to
    // that value, if not, call walk_tree on the
    // func_def
    break;
  case AST_ROOT:
    return walk_tree(NULL, line->data.root.func_def_list, symtable);
  case AST_RETURN:
    // Push line to function's stack of return nodes
    if (parent->data.func_def.first_walk) {
      // Presence of at least one return node raises the flag
      parent->data.func_def.has_return_node = true;
      if (stack_push(parent->data.func_def.return_nodes, line)) {
        return -1;
      }
      return 0;
    }

    // TODO: We must check that all returns in
    // function are of the same type (does not
    // matter if function's return type is
    // inferred or explicitly stated) If
    // return_node.value == NULL then ret_type is
    // Unit

    return walk_tree(parent, line->data.return_node.value, symtable);
  case AST_DECLARATION:
    if (walk_tree(parent, line->data.declaration.value, symtable)) {
      return -1;
    }
    line->ret_type = RETURN_UNIT;
    break;
  default:
    return -1;
  }

  return 0;
} // int walk_tree(ast_node_t *parent, ast_node_t *line, symtable_t *symtable)

/**
 * @brief Check function for return type validity
 *
 * @param root AST root node
 * @return int Return code; 0 on success,
 * non-zero on failure
 */
int check_func_ret_type_validity(ast_node_t *root) {
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

    // Function must have explicitly stated return value if the function is
    // either directly or indirectly recursive or the function has at least
    // one return node
    if ((is_recursive || has_return_node) && has_ret_unknown) {
      // TODO: print type error
      return -1;
    }

    if (true) {
      // TODO: check all return nodes that they are of the same type and that
      // they match return type
    }

    func_item = func_item->data.llist_item.next;
  }

  return 0;
} // int check_func_ret_type_validity(ast_node_t *root)

int check_ast(ast_node_t *root, symtable_t *symtable) {
  // If tree has no main function
  if (!root->data.root.main_func) {
    return -1;
  }
  // Check whether functions have valid return types should they be recursive
  if (set_recursion_flags(root, symtable)) {
    return -1;
  }
  // Assign return values to expressions
  if (walk_tree(NULL, root, symtable)) {
    return -1;
  }
  // Check if functions have valid return types
  if (check_func_ret_type_validity(root)) {
    return -1;
  }

  return 0;
} // int check_ast(ast_node_t *root, symtable_t *symtable)