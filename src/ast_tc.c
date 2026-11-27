/**
 * @file ast_tc.c
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - Implementation of AST type-checking
 * function and related functions
 *
 * @date 2026-11-27
 */

#include "ast_tc.h"
#include "ast.h"
#include "hashtable.h"
#include "stack.h"
#include "symtable.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int retcode = 0;

void print_error(const char *str) { fprintf(stderr, "error: %s\n", str); }

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
  stack_t *stack = node->data.func_def.dir_reach_funcs;
  char **func_name = NULL;

  for (size_t idx = 0; (func_name = stack_get(stack, idx)) != NULL; idx++) {
    // val is not to be used
    void *val;
    int ret = hashtable_find(ht, *func_name, &val);
    symbol_t *symbol = symtable_find(sym, *func_name);

    if (!symbol) {
      // Symbol not found
      return -1;
    }
    if (!symbol->is_function || !symbol->declaration) {
      // If for some reason searching function name returned
      // variable declaration or the function declaration is NULL
      return -1;
    }

    if (ret == 0) {
      // Function found
      continue;
    } else if (ret == 1) {
      // Function not found
      if (hashtable_insert(ht, *func_name, NULL)) {
        return -1;
      }
      if (check_for_recursion(symbol->declaration, ht, sym, false)) {
        return -1;
      }
    } else {
      // Hashtable lookup error
      return -1;
    }
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

    if (check_for_recursion(func, reachable_funcs, symtable, true)) {
      hashtable_free(reachable_funcs);
      return -1;
    }

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
  char **str_stack = stack_get(stack, idx++);

  // Loop while stack has something at index idx
  while (str_stack) {
    // If strings match, str is present at stack
    // and we do not push
    if (!strcmp(str, *str_stack)) {
      return 0;
    }
    str_stack = stack_get(stack, idx++);
  }

  if (stack_push(stack, &str)) {
    return -1;
  }

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
int figure_implicit_conv(ast_node_t *parent, ast_binary_class_t class,
                         ast_node_t *left, ast_node_t *right) {
  return_type_t left_type = left->ret_type;
  return_type_t right_type = right->ret_type;

  return_type_t res_type = RETURN_UNKNOWN;

  if (left_type == RETURN_INT && right_type == RETURN_DOUBLE) {
    // Implicit conversion occurrs in left node
    left->to_double = true;
    res_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_DOUBLE && right_type == RETURN_INT) {
    // Implicit conversion occurrs in right node
    right->to_double = true;
    res_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_INT && right_type == RETURN_INT) {
    // Parent node receives the same type
    res_type = RETURN_INT;
  } else if (left_type == RETURN_DOUBLE && right_type == RETURN_DOUBLE) {
    res_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_STRING && right_type == RETURN_STRING) {
    if (parent->data.binary.binary_type != AST_ADDITION &&
        class == AST_BIN_ARITH) {
      print_error("cannot perform arithmetic operations on type string "
                  "(besides operator plus)");
      retcode = 6;
      return -1;
    }
    res_type = RETURN_STRING;
  } else if (left_type == RETURN_UNIT && right_type == RETURN_UNIT) {
    if (class == AST_BIN_ARITH) {
      print_error("cannot perform arithmetic operations on Unit pair");
      retcode = 6;
      return -1;
    }
    res_type = RETURN_BOOLEAN;
  } else if (left_type == RETURN_BOOLEAN && right_type == RETURN_BOOLEAN) {
    if (class == AST_BIN_ARITH) {
      print_error("cannot perform arithmetic operations on Boolean pair");
      retcode = 6;
      return -1;
    }
    res_type = RETURN_BOOLEAN;
  } else {
    print_error("cannot cast operators to single type (implicit type "
                "conversion error)");
    retcode = 6;
    return -1;
  }

  // Since implicit conversion check is performed for AST_BIN_ARITH and
  // AST_BIN_LOGIC, we must set type accordingly
  if (class == AST_BIN_ARITH) {
    parent->ret_type = res_type;
  } else {
    parent->ret_type = RETURN_BOOLEAN;
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
  case AST_BIN_LOGIC:
    if (figure_implicit_conv(node, class, left_node, right_node)) {
      // Implicit conversion error
      return -1;
    }
    break;
  case AST_BIN_ASSIGN:
    if (left_node->type != AST_LEAF) {
      print_error("cannot assign to rvalue");
      retcode = 6;
      return -1;
    } else if (left_node->data.leaf->is_const) {
      print_error("cannot change value of a constant");
      retcode = 7;
      return -1;
    } else if (left_node->ret_type == RETURN_DOUBLE &&
               right_node->ret_type == RETURN_INT) {
      right_node->to_double = true;
    } else if (left_node->ret_type != right_node->ret_type) {
      print_error("assignment of differing type");
      retcode = 6;
      return -1;
    }
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
  case AST_BOOLEAN:
    // Type was already set when the node was created
    break;
  case AST_LEAF:
    if (line->data.leaf->is_function) {
      // I don't think this should happen
      return -1;
    }
    line->ret_type = line->data.leaf->declaration->data.declaration.return_type;
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
    if (line->data.while_statement.condition->ret_type != RETURN_BOOLEAN) {
      print_error("condition must have a boolean value");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.while_statement.while_block, symtable)) {
      return -1;
    }
    line->ret_type = RETURN_UNIT;
    break;
  case AST_FOR_LOOP:
    if (walk_tree(parent, line->data.for_loop.gen_start, symtable)) {
      return -1;
    }
    return_type_t g_start =
        line->data.for_loop.gen_start->data.declaration.return_type;
    if (g_start != RETURN_INT) {
      print_error("generator start must be of integer type");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.for_loop.gen_end, symtable)) {
      return -1;
    }
    return_type_t g_end = line->data.for_loop.gen_end->ret_type;
    if (g_end != RETURN_INT) {
      print_error("generator end must be of integer type");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.for_loop.gen_step, symtable)) {
      return -1;
    }
    ast_node_t *g_step = line->data.for_loop.gen_step;
    if (g_step) {
      if (g_step->ret_type != RETURN_INT) {
        print_error("generator step must be of integer type");
        retcode = 6;
        return -1;
      }
      if (g_step->data.integer == 0) {
        print_error("generator step must not be zero");
        retcode = 40;
        return -1;
      }
    }
    if (walk_tree(parent, line->data.for_loop.llist_guards, symtable)) {
      return -1;
    }
    ast_node_t *guard_ll = line->data.for_loop.llist_guards;
    if (guard_ll) {
      ast_node_t *guard = guard_ll->data.llist.begin;
      while (guard) {
        if (guard->data.llist_item.node->ret_type != RETURN_BOOLEAN) {
          print_error("guard must be of boolean type");
          retcode = 6;
          return -1;
        }
        guard = guard->data.llist_item.next;
      }
    }

    if (walk_tree(parent, line->data.for_loop.code_block, symtable)) {
      return -1;
    }
    line->ret_type = RETURN_UNIT;
    break;
  case AST_IF_STATEMENT:
    if (walk_tree(parent, line->data.if_statement.condition, symtable)) {
      return -1;
    }
    if (line->data.if_statement.condition->ret_type != RETURN_BOOLEAN) {
      print_error("condition must have a boolean value");
      retcode = 6;
      return -1;
    }
    ast_node_t *if_blk = line->data.if_statement.if_block;
    ast_node_t *else_blk = line->data.if_statement.else_block;
    ast_node_t *elsifs = line->data.if_statement.llist_else_if;
    if (walk_tree(parent, if_blk, symtable)) {
      return -1;
    }
    if (walk_tree(parent, else_blk, symtable)) {
      return -1;
    }
    if (walk_tree(parent, elsifs, symtable)) {
      return -1;
    }
    if (!if_blk || !else_blk) {
      line->ret_type = RETURN_UNIT;
      return 0;
    }
    bool ifb_line = if_blk->data.llist.begin == if_blk->data.llist.end;
    bool else_line = else_blk->data.llist.begin == else_blk->data.llist.end;
    bool is_exp = ifb_line && else_line && else_blk && !elsifs;
    ast_node_t *ifb = if_blk->data.llist.begin;
    ast_node_t *elseb = else_blk->data.llist.begin;
    return_type_t ifb_ret = ifb->data.llist_item.node->ret_type;
    return_type_t else_ret = elseb->data.llist_item.node->ret_type;
    if (is_exp) {
      if (ifb_ret == RETURN_INT && else_ret == RETURN_DOUBLE) {
        ifb->data.llist_item.node->to_double = true;
        line->ret_type = RETURN_DOUBLE;
      } else if (ifb_ret == RETURN_DOUBLE && else_ret == RETURN_INT) {
        elseb->data.llist_item.node->to_double = true;
        line->ret_type = RETURN_DOUBLE;
      } else if (ifb_ret == else_ret) {
        line->ret_type = ifb_ret;
      } else {
        print_error("if-else expression must be of the same type or castable "
                    "to one type");
        retcode = 6;
        return -1;
      }
    } else {
      line->ret_type = RETURN_UNIT;
    }
    break;
  case AST_ELSE_IF:
    if (walk_tree(parent, line->data.else_if.cond, symtable)) {
      return -1;
    }
    if (line->data.else_if.cond->ret_type != RETURN_BOOLEAN) {
      print_error("condition must have a boolean value");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.else_if.else_if_block, symtable)) {
      return -1;
    }
    break;
  case AST_FUNC_DEF:
    // This is to prevent recursive functions from getting called repeatedly
    if (line->data.func_def.function_walked) {
      return 0;
    }
    line->data.func_def.function_walked = true;
    line->ret_type = line->data.func_def.return_type;

    // Order parameters -> tree_list is important
    if (walk_tree(line, line->data.func_def.parameters, symtable)) {
      return -1;
    }
    if (walk_tree(line, line->data.func_def.tree_list, symtable)) {
      return -1;
    }
    if (!line->data.func_def.has_return_node &&
        line->data.func_def.return_type == RETURN_UNKNOWN) {
      ast_node_t *llist = line->data.func_def.tree_list;
      line->ret_type = llist->data.llist.end->data.llist_item.node->ret_type;
      line->data.func_def.return_type = line->ret_type;
    }
    break;
  case AST_FUNC_CALL:
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
    // Check parameter types
    ast_node_t *call_param_list = line->data.func_call.parameters;
    ast_node_t *def_param_list = sym->declaration->data.func_def.parameters;
    // Both have something or both have nothing
    if (call_param_list && def_param_list) {
      // If it is the "both have nothing"
      if (!call_param_list) {
        line->ret_type = sym->declaration->ret_type;
        return 0;
      }
      // Both have something
      ast_node_t *param_call =
          line->data.func_call.parameters->data.llist.begin;
      ast_node_t *param_def =
          sym->declaration->data.func_def.parameters->data.llist.begin;
      // Loop till one of them runs out or both run out at the same time
      while (param_call && param_def) {
        ast_node_t *pc = param_call->data.llist_item.node;
        ast_node_t *pd = param_def->data.llist_item.node;

        if (pc->ret_type == RETURN_INT &&
            pd->data.declaration.return_type == RETURN_DOUBLE) {
          pc->to_double = true;
        } else if (pc->ret_type != pd->data.declaration.return_type) {
          print_error("function call paramter types do not match");
          retcode = 6;
          return -1;
        }
        param_call = param_call->data.llist_item.next;
        param_def = param_def->data.llist_item.next;
      }
      if (param_call || param_def) {
        print_error(
            "number of parameters in function call do not match function "
            "declaration");
        retcode = 5;
        return -1;
      }
    } else if (!call_param_list && !def_param_list) {
      // Both functions have empty parameters
    } else {
      print_error("number of parameters in function call do not match function "
                  "declaration");
      retcode = 5;
      return -1;
    }
    line->ret_type = sym->declaration->ret_type;
    break;
  case AST_ROOT:
    return walk_tree(NULL, line->data.root.func_def_list, symtable);
  case AST_RETURN:
    if (walk_tree(parent, line->data.return_node.value, symtable)) {
      return -1;
    }
    if (!line->data.return_node.value) {
      line->ret_type = RETURN_UNIT;
      break;
    } else {
      line->ret_type = line->data.return_node.value->ret_type;
    }
    break;
  case AST_DECLARATION:
    if (walk_tree(parent, line->data.declaration.value, symtable)) {
      return -1;
    }
    return_type_t dec_ret = line->data.declaration.return_type;
    return_type_t val_ret;
    if (line->data.declaration.value) {
      val_ret = line->data.declaration.value->ret_type;
    } else {
      val_ret = line->data.declaration.return_type;
    }
    if (dec_ret == RETURN_UNKNOWN) {
      // Type inference
      line->data.declaration.return_type = val_ret;
    } else if (val_ret == RETURN_INT && dec_ret == RETURN_DOUBLE) {
      // Implicit type conversion
      line->data.declaration.value->to_double = true;
    } else if (dec_ret != val_ret) {
      print_error("assignment of differing type");
      retcode = 6;
      return -1;
    }
    // Declaration line itself is of type Unit as is defined in specification
    line->ret_type = RETURN_UNIT;
    break;
  default:
    return -1;
  }

  return 0;
} // int walk_tree(ast_node_t *parent, ast_node_t *line, symtable_t *symtable)

int walk_tree_meta(ast_node_t *parent, ast_node_t *line) {
  if (!line) {
    return 0;
  }

  switch (line->type) {
  case AST_INTEGER:
  case AST_DOUBLE:
  case AST_STRING:
  case AST_UNIT:
  case AST_LEAF:
  case AST_BOOLEAN:
    break;
  case AST_BINARY:
    if (walk_tree_meta(parent, line->data.binary.left)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.binary.right)) {
      return -1;
    }
    break;
  case AST_LINKED_LIST:
    if (walk_tree_meta(parent, line->data.llist.begin)) {
      return -1;
    }
    break;
  case AST_LINKED_LIST_ITEM:
    if (walk_tree_meta(parent, line->data.llist_item.node)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.llist_item.next)) {
      return -1;
    }
    break;
  case AST_WHILE_LOOP:
    if (walk_tree_meta(parent, line->data.while_statement.condition)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.while_statement.while_block)) {
      return -1;
    }
    break;
  case AST_FOR_LOOP:
    if (walk_tree_meta(parent, line->data.for_loop.gen_start)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.for_loop.gen_end)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.for_loop.gen_step)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.for_loop.llist_guards)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.for_loop.code_block)) {
      return -1;
    }
    break;
  case AST_IF_STATEMENT:
    if (walk_tree_meta(parent, line->data.if_statement.condition)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.if_statement.if_block)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.if_statement.else_block)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.if_statement.llist_else_if)) {
      return -1;
    }
    break;
  case AST_ELSE_IF:
    if (walk_tree_meta(parent, line->data.else_if.cond)) {
      return -1;
    }
    if (walk_tree_meta(parent, line->data.else_if.else_if_block)) {
      return -1;
    }
    break;
  case AST_FUNC_DEF:
    if (walk_tree_meta(line, line->data.func_def.tree_list)) {
      return -1;
    }
    break;
  case AST_FUNC_CALL:
    if (walk_tree_meta(parent, line->data.func_call.parameters)) {
      return -1;
    }
    if (push_if_not_present(parent->data.func_def.dir_reach_funcs,
                            line->data.func_call.name)) {
      retcode = 99;
      return -1;
    }
    break;
  case AST_ROOT:
    if (walk_tree_meta(NULL, line->data.root.func_def_list)) {
      return -1;
    }
    break;
  case AST_RETURN:
    parent->data.func_def.has_return_node = true;
    if (walk_tree_meta(parent, line->data.return_node.value)) {
      return -1;
    }
    if (stack_push(parent->data.func_def.return_nodes, &line)) {
      retcode = 99;
      return -1;
    }
    break;
  case AST_DECLARATION:
    if (walk_tree_meta(parent, line->data.declaration.value)) {
      return -1;
    }
    break;
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
    if (is_recursive && has_ret_unknown) {
      print_error("recursive functions must have explicit return types");
      retcode = 10;
      return -1;
    }
    if (has_return_node && has_ret_unknown) {
      print_error(
          "functions with return statements must have explicit return type");
      retcode = 10;
      return -1;
    }

    func_item = func_item->data.llist_item.next;
  }

  return 0;
} // int check_func_ret_type_validity(ast_node_t *root)

int check_main(ast_node_t *root) {
  ast_node_t *main = root->data.root.main_func;

  if (main->data.func_def.parameters) {
    print_error("main function must not have any parameters");
    retcode = 5;
    return -1;
  }
  if (main->ret_type != RETURN_UNIT) {
    print_error("main function must have return type Unit");
    retcode = 6;
    return -1;
  }

  return 0;
}

int check_return_types(ast_node_t *root) {
  ast_node_t *func_llist = root->data.root.func_def_list;

  if (!func_llist) {
    return -1;
  }

  ast_node_t *func_item = func_llist->data.llist.begin;
  ast_node_t *func;

  while (func_item) {
    func = func_item->data.llist_item.node;

    ast_node_t *f_last = func->data.func_def.tree_list->data.llist.end;
    if (f_last->type == AST_IF_STATEMENT) {
      bool ret_unit = func->data.func_def.return_type == RETURN_UNIT;

      ast_node_t *if_blk = f_last->data.if_statement.if_block;
      ast_node_t *else_blk = f_last->data.if_statement.else_block;
      ast_node_t *elsifs_llist = f_last->data.if_statement.llist_else_if;
      bool ifb_line = if_blk->data.llist.begin == if_blk->data.llist.end;
      bool else_line = else_blk->data.llist.begin == else_blk->data.llist.end;
      bool is_exp = ifb_line && else_line && else_blk && !elsifs_llist;
      if (is_exp) {
        print_error("if-statement as an expression is not allowed in functions "
                    "of Unit return type");
        retcode = 6;
        return -1;
      }

      // FIXME
      /*bool bran_unit = true;

      bran_unit &=
      f_last->data.if_statement.if_block->data.llist.end->ret_type; bran_unit
      &= f_last->data.if_statement.else_block->data.llist.end->ret_type;
      ast_node_t *elsifs =
          f_last->data.if_statement.llist_else_if->data.llist.begin;
      ast_node_t *elsifs_node;
      while (elsifs) {
        elsifs_node = elsifs->data.llist_item.node;
        ast_node_t *last_node =
            elsifs_node->data.else_if.else_if_block->data.llist.end;
        bran_unit &= last_node->ret_type;
        elsifs = elsifs->data.llist_item.next;
      }*/
    }

    stack_t *retnodes = func->data.func_def.return_nodes;
    ast_node_t *ret = stack_top(retnodes);
    int res = stack_is_empty(retnodes);
    while (res == 0) {
      if (func->ret_type == RETURN_DOUBLE && ret->ret_type == RETURN_INT) {
        ret->to_double = true;
      } else if (func->data.func_def.return_type == RETURN_UNIT) {
        // Throw away the result
      } else if (func->ret_type != ret->ret_type) {
        print_error("all returns must match types");
        retcode = 6;
        return -1;
      }
      if (stack_pop(retnodes)) {
        retcode = 99;
        return -1;
      }
      ret = stack_top(retnodes);
      res = stack_is_empty(retnodes);
    }
    if (res == -1) {
      retcode = 99;
      return -1;
    }

    func_item = func_item->data.llist_item.next;
  }

  return 0;
}

int check_ast(ast_node_t *root, symtable_t *symtable) {
  if (!root || !symtable) {
    return -1;
  }

  // If tree has no main function
  if (!root->data.root.main_func) {
    print_error("program has no main function");
    retcode = 3;
    return retcode;
  }
  // Check whether functions have valid return types should they be recursive
  if (set_recursion_flags(root, symtable)) {
    return retcode;
  }
  // Check if functions have valid return types
  if (check_func_ret_type_validity(root)) {
    return retcode;
  }
  // Assign return values to expressions
  if (walk_tree(NULL, root, symtable)) {
    return retcode;
  }
  if (check_main(root)) {
    return retcode;
  }
  if (check_return_types(root)) {
    return retcode;
  }

  return retcode;
} // int check_ast(ast_node_t *root, symtable_t *symtable)