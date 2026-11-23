/**
 * @file symtable.c
 *
 * @brief FIT VUT | IFJ Project 2026 - symbol table implementation
 * @authors Filip Obermajer (xobermf00), Patrik Kapec (xkapecp00)
 * @date 2026-11-23
 */

#include "symtable.h"
#include "stack.h"
#include <string.h>

#define SYMTABLE_SUCCESS 0
#define SYMTABLE_INTERNAL_ERROR 99
#define SEMANTIC_SYMBOL_REDEFINITION_ERROR 4

struct symtable {
  int scope_level;
  stack_t *scopes;
};

/**
 * @brief Get the current scope from the scopes stack
 * @return The current scope
 */
static hashtable_t *get_current_scope(symtable_t *symtable) {
  void *top_element = stack_top(symtable->scopes);
  if (top_element == NULL) {
    return NULL;
  }
  hashtable_t *scope = *(hashtable_t **)top_element;
  return scope;
}

/**
 * @brief Free a symbol
 * @param value The symbol to free
 */
static void symtable_free_func(void *value) {
  symbol_t *symbol = (symbol_t *)value;
  free(symbol);
}

/**
 * @brief Destroy a scope
 * @param scope The scope to destroy
 */
static void scope_destroy(hashtable_t *scope) {
  if (scope == NULL) {
    return;
  }
  hashtable_clear(scope, symtable_free_func);
  hashtable_free(scope);
}

int symtable_init(symtable_t **symtable) {
  if (symtable == NULL) {
    return SYMTABLE_INTERNAL_ERROR;
  }

  symtable_t *new_symtable = malloc(sizeof(symtable_t));
  if (new_symtable == NULL) {
    return SYMTABLE_INTERNAL_ERROR;
  }

  new_symtable->scope_level = 0;
  stack_t *stack;
  if (stack_init(&stack, sizeof(hashtable_t *)) != 0) {
    free(new_symtable);
    new_symtable = NULL;
    return SYMTABLE_INTERNAL_ERROR;
  }
  if (stack == NULL) {
    free(new_symtable);
    new_symtable = NULL;
    return SYMTABLE_INTERNAL_ERROR;
  }
  new_symtable->scopes = stack;

  if (symtable_scope_push(new_symtable) != SYMTABLE_SUCCESS) {
    stack_free(new_symtable->scopes);
    free(new_symtable);
    new_symtable = NULL;
    return SYMTABLE_INTERNAL_ERROR;
  }

  *symtable = new_symtable;
  return SYMTABLE_SUCCESS;
}

int symtable_insert(symtable_t *symtable, ast_node_t *node, bool is_function,
                    bool is_const) {
  if (symtable == NULL || node == NULL) {
    return SYMTABLE_INTERNAL_ERROR;
  }
  char *name;

  if (is_function) {
    name = node->data.func_def.name;
  } else {
    name = node->data.declaration.name;
  }

  hashtable_t *current_scope = get_current_scope(symtable);
  if (current_scope == NULL) {
    return SYMTABLE_INTERNAL_ERROR;
  }
  void *found;
  if (hashtable_find(current_scope, name, &found) == 0) {
    return SEMANTIC_SYMBOL_REDEFINITION_ERROR;
  }

  symbol_t *symbol = malloc(sizeof(symbol_t));
  if (symbol == NULL) {
    return SYMTABLE_INTERNAL_ERROR;
  }
  symbol->is_function = is_function;
  symbol->declaration = node;
  symbol->is_const = is_const;

  if (hashtable_insert(current_scope, name, symbol) != 0) {
    free(symbol);
    return SYMTABLE_INTERNAL_ERROR;
  }

  return SYMTABLE_SUCCESS;
}

symbol_t *symtable_find(const symtable_t *symtable, const char *name) {
  if (symtable == NULL || name == NULL) {
    return NULL;
  }
  for (int i = 0; i < symtable->scope_level; i++) {
    void *found;

    void *stack_element = stack_get(symtable->scopes, i);
    if (stack_element == NULL) {
      continue;
    }
    hashtable_t *current_scope = *(hashtable_t **)stack_element;
    if (current_scope == NULL) {
      continue;
    }
    if (hashtable_find(current_scope, name, &found) == 0) {
      return found;
    }
  }
  return NULL;
}

int symtable_scope_push(symtable_t *symtable) {
  if (symtable == NULL) {
    return SYMTABLE_INTERNAL_ERROR;
  }

  hashtable_t *new_scope;
  if (hashtable_init(&new_scope) != 0) {
    return SYMTABLE_INTERNAL_ERROR;
  }
  if (stack_push(symtable->scopes, &new_scope) != 0) {
    hashtable_free(new_scope);
    return SYMTABLE_INTERNAL_ERROR;
  }
  symtable->scope_level++;
  return SYMTABLE_SUCCESS;
}

void symtable_scope_pop(symtable_t *symtable) {
  if (symtable == NULL || symtable->scope_level <= 0) {
    return;
  }

  hashtable_t *current_scope = get_current_scope(symtable);
  if (current_scope != NULL) {
    scope_destroy(current_scope);
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
      scope_destroy(item);
    }
    stack_pop(symtable->scopes);
  }
  stack_free(symtable->scopes);
  symtable->scopes = NULL;
  free(symtable);
}