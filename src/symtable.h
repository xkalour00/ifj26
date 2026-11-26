/**
 * @file symtable.h
 *
 * @brief FIT VUT | IFJ Project 2026 - symbol table header
 * @authors Filip Obermajer (xobermf00), Patrik Kapec (xkapecp00)
 * @date 2026-11-23
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"

typedef struct node ast_node_t;

typedef struct {
  bool is_function;
  ast_node_t *declaration;
  bool is_const; // var or val
} symbol_t;

typedef struct symtable symtable_t;

/**
 * @brief Initialize a new symbol table
 * @param symtable Pointer to the symbol table
 * @return 0 on success, 99 on failure
 */
int symtable_init(symtable_t **symtable);

/**
 * @brief Insert a new symbol into the symbol table
 * @param symtable Pointer to the symbol table
 * @param node AST node representing the symbol
 * @param is_function True if the symbol is a function, false otherwise
 * @param is_const True if the symbol is a constant, false otherwise
 * @return 0 on success, 99 on failure
 */
int symtable_insert(symtable_t *symtable, ast_node_t *node, bool is_function,
                    bool is_const);

/**
 * @brief Find a symbol in the symbol table
 * @param symtable Pointer to the symbol table
 * @param name The name of the symbol to find
 * @return Pointer to the symbol if found, NULL otherwise
 */
symbol_t *symtable_find(const symtable_t *symtable, const char *name);

/**
 * @brief Push a new scope onto the symbol table
 * @param symtable Pointer to the symbol table
 * @return 0 on success, 99 on failure
 */
int symtable_scope_push(symtable_t *symtable);

/**
 * @brief Pop the current scope from the symbol table
 * @param symtable Pointer to the symbol table
 * @return 0 on success, 99 on failure
 */
int symtable_scope_pop(symtable_t *symtable);

/**
 * @brief Free the memory allocated for the symbol table
 * @param symtable Pointer to the symbol table
 */
void symtable_free(symtable_t *symtable);

#endif // SYMTABLE_H
