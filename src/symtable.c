/**
 * @file symtable.c
 *
 * @brief FIT VUT | IFJ Project 2026 - symbol table implementation
 * @authors Filip Obermajer (xobermf00), Patrik Kapec (xkapecp00)
 * @date 2026-11-17
 */

#include "symtable.h"
#include "stack.h"
#include <string.h>

#define SYMTABLE_RETURN_SUCCESS 0
#define SYMTABLE_RETURN_FAILURE -1

struct symtable {
  int scope_level;
  stack_t *scopes;
};

int symtable_init(symtable_t **symtable) {
  if (symtable == NULL) {
    return SYMTABLE_RETURN_FAILURE;
  }

  symtable_t *new_symtable = malloc(sizeof(symtable_t));
  if (new_symtable == NULL) {
    return SYMTABLE_RETURN_FAILURE;
  }

  new_symtable->scope_level = 0;
  stack_t *stack;
  stack_init(&stack, sizeof(hashtable_t *));
  if (stack == NULL) {
    free(new_symtable);
    new_symtable = NULL;
    return SYMTABLE_RETURN_FAILURE;
  }
  new_symtable->scopes = stack;

  symtable_scope_push(new_symtable);

  *symtable = new_symtable;
  return SYMTABLE_RETURN_SUCCESS;
}

int symtable_insert(symtable_t *symtable, ast_node_t *node, bool is_function,
                    bool is_const) {
  if (symtable == NULL || node == NULL) {
    return SYMTABLE_RETURN_FAILURE;
  }
  char *name;

  if (is_function) {
    name = node->data.func_def.name;
  } else {
    name = node->data.declaration.name;
  }

  hashtable_t *current_scope = *(hashtable_t **)stack_top(symtable->scopes);
  void *found;
  if (current_scope == NULL ||
      hashtable_find(current_scope, name, &found) == 0) {
    return SYMTABLE_RETURN_FAILURE;
  }

  symbol_t *symbol = malloc(sizeof(symbol_t));
  if (symbol == NULL) {
    return SYMTABLE_RETURN_FAILURE;
  }
  symbol->is_function = is_function;
  symbol->declaration = node;
  symbol->is_const = is_const;

  if (hashtable_insert(current_scope, name, symbol) != 0) {
    free(symbol);
    return SYMTABLE_RETURN_FAILURE;
  }

  return SYMTABLE_RETURN_SUCCESS;
}

symbol_t *symtable_find(const symtable_t *symtable, const char *name) {
  for (int i = 0; i < symtable->scope_level; i++) {
    void *found;
    hashtable_t *current_scope =
        *(hashtable_t **)stack_get(symtable->scopes, i);
    if (hashtable_find(current_scope, name, &found) == 0) {
      return found;
    }
  }
  return NULL;
}

int symtable_scope_push(symtable_t *symtable) {
  if (symtable == NULL) {
    return SYMTABLE_RETURN_FAILURE;
  }

  hashtable_t *new_scope;
  if (hashtable_init(&new_scope) != 0) {
    return SYMTABLE_RETURN_FAILURE;
  }
  if (stack_push(symtable->scopes, &new_scope) != 0) {
    hashtable_free(new_scope);
    return SYMTABLE_RETURN_FAILURE;
  }
  symtable->scope_level++;
  return SYMTABLE_RETURN_SUCCESS;
}

void symtable_free_func(void *value) {
  symbol_t *symbol = (symbol_t *)value;
  free(symbol);
}

void symtable_scope_pop(symtable_t *symtable) {
  if (symtable == NULL || symtable->scope_level < 0) {
    return;
  }

  hashtable_t *current_scope = *(hashtable_t **)stack_top(symtable->scopes);
  if (current_scope != NULL) {
    hashtable_clear(current_scope, symtable_free_func);
    hashtable_free(current_scope);
    stack_pop(symtable->scopes);
    symtable->scope_level--;
  }
}

void symtable_free(symtable_t *symtable) {
  if (symtable == NULL) {
    return;
  }

  while (!stack_is_empty(symtable->scopes)) {
    void *top_ptr = stack_top(symtable->scopes);

    if (top_ptr != NULL) {
      hashtable_t *item = *(hashtable_t **)top_ptr;
      hashtable_clear(item, symtable_free_func);
      hashtable_free(item);
    }
    stack_pop(symtable->scopes);
  }
  stack_free(symtable->scopes);
  symtable->scopes = NULL;
  free(symtable);
}