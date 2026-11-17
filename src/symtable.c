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

  void *found;
  if (hashtable_find(stack_top(symtable->scopes), name, &found) == 0) {
    return SYMTABLE_RETURN_FAILURE;
  }

  symbol_t *symbol = malloc(sizeof(symbol_t));
  if (symbol == NULL) {
    return SYMTABLE_RETURN_FAILURE;
  }
  symbol->is_function = is_function;
  symbol->declaration = node;
  symbol->is_const = is_const;

  if (hashtable_insert(stack_top(symtable->scopes), name, symbol) != 0) {
    free(symbol);
    return SYMTABLE_RETURN_FAILURE;
  }

  return SYMTABLE_RETURN_SUCCESS;
}

symbol_t *symtable_find(const symtable_t *symtable, const char *name) {
  for (int i = 0; i < symtable->scope_level; i++) {
    void *found;
    if (hashtable_find(stack_get(symtable->scopes, i), name, &found) == 0) {
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
  stack_push(symtable->scopes, new_scope);
  symtable->scope_level++;
  return SYMTABLE_RETURN_SUCCESS;
}

void symtable_scope_pop(symtable_t *symtable) {
  if (symtable == NULL || symtable->scope_level < 0) {
    return;
  }

  hashtable_t *current_scope = stack_top(symtable->scopes);
  if (current_scope != NULL) {
    hashtable_free(current_scope);
    stack_pop(symtable->scopes);
    symtable->scope_level--;
  }
}

void symtable_free(symtable_t *symtable) {
  if (symtable == NULL) {
    return;
  }

  void *item = stack_top(symtable->scopes);
  while (item != NULL) {
    hashtable_free(item);
    stack_pop(symtable->scopes);
    item = stack_top(symtable->scopes);
  }
  stack_free(symtable->scopes);
  symtable->scopes = NULL;
  free(symtable);
}