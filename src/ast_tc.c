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

#define BUILTIN_ANY ((return_type_t) - 1)

typedef struct {
  const char *name;
  return_type_t ret_type;
  int argc;
  return_type_t params[3];
} builtin_func_t;

// Built-in functions
static const builtin_func_t BUILTINS[] = {
    {"print", RETURN_UNIT, 1, {BUILTIN_ANY}},
    {"toStr", RETURN_STRING, 1, {BUILTIN_ANY}},
    {"readString", RETURN_STRING, 0, {RETURN_NONE}},
    {"readStr", RETURN_STRING, 0, {RETURN_NONE}},
    {"readInt", RETURN_INT, 0, {RETURN_NONE}},
    {"readDouble", RETURN_DOUBLE, 0, {RETURN_NONE}},
    {"readBoolean", RETURN_BOOLEAN, 0, {RETURN_NONE}},
    {"floor", RETURN_INT, 1, {RETURN_DOUBLE}},
    {"length", RETURN_INT, 1, {RETURN_STRING}},
    {"substring", RETURN_STRING, 3, {RETURN_STRING, RETURN_INT, RETURN_INT}},
    {"strcmp", RETURN_INT, 2, {RETURN_STRING, RETURN_STRING}},
    {"ord", RETURN_INT, 2, {RETURN_STRING, RETURN_INT}},
    {"chr", RETURN_STRING, 1, {RETURN_INT}},
    {NULL, RETURN_NONE, 0, {RETURN_NONE}},
};

static int retcode = 0;

void print_error(const char *str) { fprintf(stderr, "error: %s\n", str); }

static bool is_untyped(return_type_t t) {
  return t == RETURN_NONE || t == RETURN_UNKNOWN;
}

static const builtin_func_t *find_builtin(const char *name) {
  if (!name) {
    return NULL;
  }
  for (size_t i = 0; BUILTINS[i].name != NULL; i++) {
    if (strcmp(BUILTINS[i].name, name) == 0) {
      return &BUILTINS[i];
    }
  }
  return NULL;
}

static ast_node_t *get_block_last_node(ast_node_t *block) {
  if (!block || block->type != AST_LINKED_LIST || !block->data.llist.end) {
    return NULL;
  }
  return block->data.llist.end->data.llist_item.node;
}

static bool all_branches_return(ast_node_t *node) {
  if (!node) {
    return false;
  }
  if (node->type == AST_RETURN) {
    return true;
  }
  if (node->type != AST_IF_STATEMENT) {
    return false;
  }
  ast_node_t *if_blk = node->data.if_statement.if_block;
  ast_node_t *else_blk = node->data.if_statement.else_block;
  ast_node_t *elsifs = node->data.if_statement.llist_else_if;

  if (!else_blk || !all_branches_return(get_block_last_node(if_blk)) ||
      !all_branches_return(get_block_last_node(else_blk))) {
    return false;
  }

  if (elsifs) {
    ast_node_t *item = elsifs->data.llist.begin;
    while (item) {
      ast_node_t *ei = item->data.llist_item.node;
      if (!ei || !all_branches_return(
                     get_block_last_node(ei->data.else_if.else_if_block))) {
        return false;
      }
      item = item->data.llist_item.next;
    }
  }
  return true;
}

static bool is_single_expr_return_body(ast_node_t *func) {
  ast_node_t *body = func->data.func_def.tree_list;
  if (!body || !body->data.llist.begin ||
      body->data.llist.begin != body->data.llist.end) {
    return false;
  }
  ast_node_t *only = body->data.llist.begin->data.llist_item.node;
  if (!only || only->type != AST_RETURN || !only->data.return_node.value) {
    return false;
  }
  return stack_get(func->data.func_def.return_nodes, 1) == NULL;
}

static int check_builtin_call(ast_node_t *call, const builtin_func_t *b) {
  ast_node_t *arg_list = call->data.func_call.parameters;
  ast_node_t *arg_item = arg_list ? arg_list->data.llist.begin : NULL;

  int idx = 0;
  while (arg_item && idx < b->argc) {
    ast_node_t *arg = arg_item->data.llist_item.node;
    return_type_t expected = b->params[idx];

    if (!arg || is_untyped(arg->ret_type)) {
      print_error("invalid expression type passed to built-in function");
      retcode = 6;
      return -1;
    }
    if (expected != BUILTIN_ANY) {
      if (arg->ret_type == RETURN_INT && expected == RETURN_DOUBLE) {
        arg->to_double = true;
      } else if (arg->ret_type != expected) {
        print_error("built-in function argument type mismatch");
        retcode = 5;
        return -1;
      }
    }
    arg_item = arg_item->data.llist_item.next;
    idx++;
  }

  if (arg_item != NULL || idx != b->argc) {
    print_error("built-in function argument count mismatch");
    retcode = 5;
    return -1;
  }

  call->ret_type = b->ret_type;
  return 0;
}

static bool unify_branch_type(return_type_t *acc, return_type_t candidate) {
  if (is_untyped(candidate)) {
    return false;
  }
  if (*acc == RETURN_UNKNOWN) {
    *acc = candidate;
    return true;
  }
  if (*acc == candidate) {
    return true;
  }
  if ((*acc == RETURN_INT && candidate == RETURN_DOUBLE) ||
      (*acc == RETURN_DOUBLE && candidate == RETURN_INT)) {
    *acc = RETURN_DOUBLE;
    return true;
  }
  return false;
}

/**
 * @brief Check particular function for recursiveness
 */
int check_for_recursion(ast_node_t *node, hashtable_t *ht, symtable_t *sym,
                        bool checked_func) {
  stack_t *stack = node->data.func_def.dir_reach_funcs;
  char **func_name = NULL;

  for (size_t idx = 0; (func_name = stack_get(stack, idx)) != NULL; idx++) {
    if (find_builtin(*func_name) != NULL) {
      continue;
    }

    void *val;
    int ret = hashtable_find(ht, *func_name, &val);
    symbol_t *symbol = symtable_find(sym, *func_name);

    if (!symbol || !symbol->is_function) {
      print_error("undefined function");
      retcode = 3;
      return -1;
    }
    if (!symbol->declaration) {
      continue;
    }

    if (ret == 0) {
      continue;
    } else if (ret == 1) {
      if (hashtable_insert(ht, *func_name, NULL)) {
        retcode = 99;
        return -1;
      }
      if (check_for_recursion(symbol->declaration, ht, sym, false)) {
        return -1;
      }
    } else {
      retcode = 99;
      return -1;
    }
  }

  if (!checked_func) {
    return 0;
  }

  void *val;
  int ret = hashtable_find(ht, node->data.func_def.name, &val);
  if (ret == 0) {
    node->data.func_def.possible_recursion = true;
  }

  return 0;
}

/**
 * @brief Set functions's possible_recursion flag if the function is recursive
 */
int set_recursion_flags(ast_node_t *root, symtable_t *symtable) {
  if (!root || !root->data.root.func_def_list) {
    retcode = 99;
    return -1;
  }

  ast_node_t *func_item = root->data.root.func_def_list->data.llist.begin;
  while (func_item) {
    ast_node_t *func = func_item->data.llist_item.node;

    hashtable_t *reachable_funcs;
    if (hashtable_init(&reachable_funcs)) {
      retcode = 99;
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
}

/**
 * @brief Pushes string str if stack does not already contain it
 */
int push_if_not_present(stack_t *stack, char *str) {
  size_t idx = 0;
  char **str_stack = stack_get(stack, idx++);

  while (str_stack) {
    if (!strcmp(str, *str_stack)) {
      return 0;
    }
    str_stack = stack_get(stack, idx++);
  }

  if (stack_push(stack, &str)) {
    return -1;
  }

  return 0;
}

int figure_implicit_conv(ast_node_t *parent, ast_binary_class_t class,
                         ast_node_t *left, ast_node_t *right) {
  if (!left || !right) {
    print_error("missing binary operand");
    retcode = 6;
    return -1;
  }

  return_type_t left_type = left->ret_type;
  return_type_t right_type = right->ret_type;
  ast_binary_type_t op = parent->data.binary.binary_type;
  return_type_t res_type = RETURN_UNKNOWN;

  if (left_type == RETURN_INT && right_type == RETURN_DOUBLE) {
    left->to_double = true;
    res_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_DOUBLE && right_type == RETURN_INT) {
    right->to_double = true;
    res_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_INT && right_type == RETURN_INT) {
    res_type = RETURN_INT;
  } else if (left_type == RETURN_DOUBLE && right_type == RETURN_DOUBLE) {
    res_type = RETURN_DOUBLE;
  } else if (left_type == RETURN_STRING && right_type == RETURN_STRING) {
    if (class == AST_BIN_ARITH && op != AST_ADDITION) {
      print_error("cannot perform arithmetic operations on String besides +");
      retcode = 6;
      return -1;
    }
    res_type = RETURN_STRING;
  } else if (left_type == RETURN_UNIT && right_type == RETURN_UNIT) {
    if (op != AST_EQUAL && op != AST_NEQUAL) {
      print_error("only == and != are allowed on Unit operands");
      retcode = 6;
      return -1;
    }
    res_type = RETURN_BOOLEAN;
  } else if (left_type == RETURN_BOOLEAN && right_type == RETURN_BOOLEAN) {
    if (op != AST_EQUAL && op != AST_NEQUAL) {
      print_error("only == and != are allowed on Boolean operands");
      retcode = 6;
      return -1;
    }
    res_type = RETURN_BOOLEAN;
  } else {
    print_error("incompatible operand types");
    retcode = 6;
    return -1;
  }

  parent->ret_type = (class == AST_BIN_ARITH) ? res_type : RETURN_BOOLEAN;
  return 0;
}

int walk_binary(ast_node_t *parent, ast_node_t *node, symtable_t *symtable) {
  ast_node_t *left_node = node->data.binary.left;
  ast_node_t *right_node = node->data.binary.right;
  ast_binary_class_t class = node->data.binary.binary_class;
  ast_binary_type_t type = node->data.binary.binary_type;

  if (walk_tree(parent, left_node, symtable)) {
    return -1;
  }
  if (walk_tree(parent, right_node, symtable)) {
    return -1;
  }

  switch (class) {
  case AST_BIN_ARITH:
    if (type == AST_SUBTRACTION && right_node == NULL) {
      if (!left_node || (left_node->ret_type != RETURN_INT &&
                         left_node->ret_type != RETURN_DOUBLE)) {
        print_error("unary minus requires Int or Double operand");
        retcode = 6;
        return -1;
      }
      node->ret_type = left_node->ret_type;
      return 0;
    }
    if (figure_implicit_conv(node, class, left_node, right_node)) {
      return -1;
    }
    break;

  case AST_BIN_LOGIC:
    if (type == AST_NOT) {
      ast_node_t *operand = left_node ? left_node : right_node;
      if (!operand || operand->ret_type != RETURN_BOOLEAN) {
        print_error("operator ! requires Boolean operand");
        retcode = 6;
        return -1;
      }
      node->ret_type = RETURN_BOOLEAN;
      return 0;
    }
    if (type == AST_AND || type == AST_OR) {
      if (!left_node || !right_node || left_node->ret_type != RETURN_BOOLEAN ||
          right_node->ret_type != RETURN_BOOLEAN) {
        print_error("logical && and || require Boolean operands");
        retcode = 6;
        return -1;
      }
      node->ret_type = RETURN_BOOLEAN;
      return 0;
    }
    if (figure_implicit_conv(node, class, left_node, right_node)) {
      return -1;
    }
    break;

  case AST_BIN_ASSIGN:
    if (!left_node || left_node->type != AST_LEAF) {
      print_error("cannot assign to rvalue");
      retcode = 6;
      return -1;
    } else if (left_node->data.leaf->is_const) {
      print_error("cannot change value of a constant");
      retcode = 7;
      return -1;
    } else if (!right_node || is_untyped(right_node->ret_type)) {
      print_error("cannot assign untyped expression");
      retcode = 6;
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
  if (!line) {
    return 0;
  }

  switch (line->type) {
  case AST_INTEGER:
    line->ret_type = RETURN_INT;
    break;
  case AST_DOUBLE:
    line->ret_type = RETURN_DOUBLE;
    break;
  case AST_STRING:
    line->ret_type = RETURN_STRING;
    break;
  case AST_UNIT:
    line->ret_type = RETURN_UNIT;
    break;
  case AST_BOOLEAN:
    line->ret_type = RETURN_BOOLEAN;
    break;
  case AST_LEAF:
    if (!line->data.leaf || line->data.leaf->is_function ||
        !line->data.leaf->declaration) {
      print_error("invalid identifier reference");
      retcode = 3;
      return -1;
    }
    line->ret_type = line->data.leaf->declaration->data.declaration.return_type;
    if (is_untyped(line->ret_type)) {
      print_error("variable has unresolved type");
      retcode = 6;
      return -1;
    }
    break;
  case AST_BINARY:
    return walk_binary(parent, line, symtable);
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
      print_error("while condition must be Boolean");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.while_statement.while_block, symtable)) {
      return -1;
    }
    line->ret_type = RETURN_UNIT;
    break;
  case AST_FOR_LOOP: {
    if (walk_tree(parent, line->data.for_loop.gen_start, symtable)) {
      return -1;
    }
    return_type_t g_start =
        line->data.for_loop.gen_start->data.declaration.return_type;
    if (g_start != RETURN_INT) {
      print_error("generator start must be Int");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.for_loop.gen_end, symtable)) {
      return -1;
    }
    if (line->data.for_loop.gen_end->ret_type != RETURN_INT) {
      print_error("generator end must be Int");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.for_loop.gen_step, symtable)) {
      return -1;
    }
    ast_node_t *g_step = line->data.for_loop.gen_step;
    if (g_step) {
      if (g_step->ret_type != RETURN_INT) {
        print_error("generator step must be Int");
        retcode = 6;
        return -1;
      }
      if (g_step->type == AST_INTEGER && g_step->data.integer == 0) {
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
          print_error("for guard must be Boolean");
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
  }
  case AST_IF_STATEMENT: {
    if (walk_tree(parent, line->data.if_statement.condition, symtable)) {
      return -1;
    }
    if (line->data.if_statement.condition->ret_type != RETURN_BOOLEAN) {
      print_error("if condition must be Boolean");
      retcode = 6;
      return -1;
    }
    ast_node_t *if_blk = line->data.if_statement.if_block;
    ast_node_t *else_blk = line->data.if_statement.else_block;
    ast_node_t *elsifs = line->data.if_statement.llist_else_if;

    if (walk_tree(parent, if_blk, symtable)) {
      return -1;
    }
    if (walk_tree(parent, elsifs, symtable)) {
      return -1;
    }
    if (walk_tree(parent, else_blk, symtable)) {
      return -1;
    }

    if (!else_blk) {
      line->ret_type = RETURN_UNIT;
      break;
    }

    // Unify result types across if, all else-if, and else branches
    return_type_t unified = RETURN_UNKNOWN;
    bool compatible = true;

    ast_node_t *if_tail = get_block_last_node(if_blk);
    if (if_tail && !all_branches_return(if_tail)) {
      compatible &= unify_branch_type(&unified, if_tail->ret_type);
    }

    if (elsifs) {
      ast_node_t *ei_item = elsifs->data.llist.begin;
      while (ei_item && compatible) {
        ast_node_t *ei = ei_item->data.llist_item.node;
        ast_node_t *ei_tail =
            get_block_last_node(ei->data.else_if.else_if_block);
        if (ei_tail && !all_branches_return(ei_tail)) {
          compatible &= unify_branch_type(&unified, ei_tail->ret_type);
        }
        ei_item = ei_item->data.llist_item.next;
      }
    }

    ast_node_t *else_tail = get_block_last_node(else_blk);
    if (compatible && else_tail && !all_branches_return(else_tail)) {
      compatible &= unify_branch_type(&unified, else_tail->ret_type);
    }

    if (!compatible) {
      line->ret_type = RETURN_NONE;
      break;
    }

    if (unified == RETURN_UNKNOWN) {
      line->ret_type = RETURN_UNIT;
      break;
    }

    if (unified == RETURN_DOUBLE) {
      if (if_tail && !all_branches_return(if_tail) &&
          if_tail->ret_type == RETURN_INT) {
        if_tail->to_double = true;
      }
      if (elsifs) {
        ast_node_t *ei_item = elsifs->data.llist.begin;
        while (ei_item) {
          ast_node_t *ei = ei_item->data.llist_item.node;
          ast_node_t *ei_tail =
              get_block_last_node(ei->data.else_if.else_if_block);
          if (ei_tail && !all_branches_return(ei_tail) &&
              ei_tail->ret_type == RETURN_INT) {
            ei_tail->to_double = true;
          }
          ei_item = ei_item->data.llist_item.next;
        }
      }
      if (else_tail && !all_branches_return(else_tail) &&
          else_tail->ret_type == RETURN_INT) {
        else_tail->to_double = true;
      }
    }

    line->ret_type = unified;
    break;
  }
  case AST_ELSE_IF:
    if (walk_tree(parent, line->data.else_if.cond, symtable)) {
      return -1;
    }
    if (line->data.else_if.cond->ret_type != RETURN_BOOLEAN) {
      print_error("else-if condition must be Boolean");
      retcode = 6;
      return -1;
    }
    if (walk_tree(parent, line->data.else_if.else_if_block, symtable)) {
      return -1;
    }
    break;
  case AST_FUNC_DEF:
    if (line->data.func_def.function_walked) {
      return 0;
    }
    line->data.func_def.function_walked = true;
    line->ret_type = line->data.func_def.return_type;

    if (walk_tree(line, line->data.func_def.parameters, symtable)) {
      return -1;
    }
    if (walk_tree(line, line->data.func_def.tree_list, symtable)) {
      return -1;
    }
    if (is_untyped(line->data.func_def.return_type)) {
      ast_node_t *last_node =
          get_block_last_node(line->data.func_def.tree_list);
      if (!last_node) {
        line->ret_type = RETURN_UNIT;
      } else if (is_untyped(last_node->ret_type)) {
        print_error("cannot infer function return type");
        retcode = 6;
        return -1;
      } else {
        line->ret_type = last_node->ret_type;
      }
      line->data.func_def.return_type = line->ret_type;
    }
    break;
  case AST_FUNC_CALL: {
    if (walk_tree(parent, line->data.func_call.parameters, symtable)) {
      return -1;
    }

    const builtin_func_t *builtin = find_builtin(line->data.func_call.name);
    if (builtin) {
      return check_builtin_call(line, builtin);
    }

    symbol_t *sym = symtable_find(symtable, line->data.func_call.name);
    if (!sym || !sym->is_function || !sym->declaration) {
      print_error("undefined function");
      retcode = 3;
      return -1;
    }
    if (walk_tree(parent, sym->declaration, symtable)) {
      return -1;
    }

    ast_node_t *call_param_list = line->data.func_call.parameters;
    ast_node_t *def_param_list = sym->declaration->data.func_def.parameters;
    ast_node_t *param_call =
        call_param_list ? call_param_list->data.llist.begin : NULL;
    ast_node_t *param_def =
        def_param_list ? def_param_list->data.llist.begin : NULL;

    while (param_call && param_def) {
      ast_node_t *pc = param_call->data.llist_item.node;
      ast_node_t *pd = param_def->data.llist_item.node;

      if (is_untyped(pc->ret_type)) {
        print_error("invalid expression passed as function argument");
        retcode = 6;
        return -1;
      }
      if (pc->ret_type == RETURN_INT &&
          pd->data.declaration.return_type == RETURN_DOUBLE) {
        pc->to_double = true;
      } else if (pc->ret_type != pd->data.declaration.return_type) {
        print_error("function call parameter types do not match");
        retcode = 5;
        return -1;
      }
      param_call = param_call->data.llist_item.next;
      param_def = param_def->data.llist_item.next;
    }
    if (param_call || param_def) {
      print_error("number of parameters in function call does not match "
                  "function declaration");
      retcode = 5;
      return -1;
    }

    line->ret_type = sym->declaration->ret_type;
    break;
  }
  case AST_ROOT:
    return walk_tree(NULL, line->data.root.func_def_list, symtable);
  case AST_RETURN:
    if (walk_tree(parent, line->data.return_node.value, symtable)) {
      return -1;
    }
    if (!line->data.return_node.value) {
      line->ret_type = RETURN_UNIT;
    } else {
      line->ret_type = line->data.return_node.value->ret_type;
    }
    break;
  case AST_DECLARATION: {
    if (walk_tree(parent, line->data.declaration.value, symtable)) {
      return -1;
    }
    return_type_t dec_ret = line->data.declaration.return_type;
    if (!line->data.declaration.value) {
      // Function parameter declaration
      line->ret_type = RETURN_UNIT;
      break;
    }
    return_type_t val_ret = line->data.declaration.value->ret_type;
    if (is_untyped(val_ret)) {
      print_error("cannot initialize variable from untyped expression");
      retcode = 6;
      return -1;
    }
    if (is_untyped(dec_ret)) {
      line->data.declaration.return_type = val_ret;
    } else if (val_ret == RETURN_INT && dec_ret == RETURN_DOUBLE) {
      line->data.declaration.value->to_double = true;
    } else if (dec_ret != val_ret) {
      print_error("variable initialization of differing type");
      retcode = 6;
      return -1;
    }
    line->ret_type = RETURN_UNIT;
    break;
  }
  default:
    retcode = 99;
    return -1;
  }

  return 0;
}

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
}

int check_func_ret_type_validity(ast_node_t *root) {
  ast_node_t *func_llist = root->data.root.func_def_list;
  if (!func_llist) {
    retcode = 99;
    return -1;
  }

  ast_node_t *func_item = func_llist->data.llist.begin;
  while (func_item) {
    ast_node_t *func = func_item->data.llist_item.node;

    bool is_recursive = func->data.func_def.possible_recursion;
    bool has_return_node = func->data.func_def.has_return_node;
    bool has_ret_unknown = is_untyped(func->data.func_def.return_type);

    if (is_recursive && has_ret_unknown) {
      print_error("recursive functions must have explicit return types");
      retcode = 10;
      return -1;
    }
    if (has_return_node && has_ret_unknown &&
        !is_single_expr_return_body(func)) {
      print_error(
          "functions with return statements must have explicit return type");
      retcode = 10;
      return -1;
    }

    func_item = func_item->data.llist_item.next;
  }

  return 0;
}

int check_main(ast_node_t *root) {
  ast_node_t *main = root->data.root.main_func;

  if (main->data.func_def.parameters &&
      main->data.func_def.parameters->data.llist.begin != NULL) {
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
    retcode = 99;
    return -1;
  }

  ast_node_t *func_item = func_llist->data.llist.begin;
  while (func_item) {
    ast_node_t *func = func_item->data.llist_item.node;

    // Check all explicit return statements
    stack_t *retnodes = func->data.func_def.return_nodes;
    ast_node_t **ret;
    for (size_t i = 0; (ret = stack_get(retnodes, i)) != NULL; i++) {
      ast_node_t *ret_node = *ret;
      if (func->ret_type == RETURN_UNIT) {
        if (ret_node->data.return_node.value &&
            is_untyped(ret_node->ret_type)) {
          print_error("invalid expression in return statement");
          retcode = 6;
          return -1;
        }
      } else if (func->ret_type == RETURN_DOUBLE &&
                 ret_node->ret_type == RETURN_INT) {
        ret_node->to_double = true;
        if (ret_node->data.return_node.value) {
          ret_node->data.return_node.value->to_double = true;
        }
      } else if (func->ret_type != ret_node->ret_type) {
        print_error(
            "return statement type does not match function return type");
        retcode = 6;
        return -1;
      }
    }

    // Check implicit return for non-Unit functions
    if (func->ret_type != RETURN_UNIT) {
      ast_node_t *f_last = get_block_last_node(func->data.func_def.tree_list);
      if (!f_last) {
        print_error("non-Unit function has empty body");
        retcode = 6;
        return -1;
      }
      if (!all_branches_return(f_last)) {
        if (func->ret_type == RETURN_DOUBLE && f_last->ret_type == RETURN_INT) {
          f_last->to_double = true;
        } else if (f_last->ret_type != func->ret_type) {
          print_error(
              "implicit return type does not match function return type");
          retcode = 6;
          return -1;
        }
      }
    }

    func_item = func_item->data.llist_item.next;
  }

  return 0;
}

int check_ast(ast_node_t *root, symtable_t *symtable) {
  retcode = 0;
  if (!root || !symtable) {
    return 99;
  }

  if (!root->data.root.main_func) {
    print_error("program has no main function");
    retcode = 3;
    return retcode;
  }
  if (walk_tree_meta(NULL, root)) {
    return retcode ? retcode : 99;
  }
  if (set_recursion_flags(root, symtable)) {
    return retcode ? retcode : 99;
  }
  if (check_func_ret_type_validity(root)) {
    return retcode ? retcode : 99;
  }
  if (walk_tree(NULL, root, symtable)) {
    return retcode ? retcode : 99;
  }
  if (check_main(root)) {
    return retcode ? retcode : 99;
  }
  if (check_return_types(root)) {
    return retcode ? retcode : 99;
  }

  return retcode;
}