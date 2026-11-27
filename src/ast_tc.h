/**
 * @file ast_tc.h
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - AST type-check function interface header
 *
 * @date 2026-11-26
 */

#ifndef AST_TC_H
#define AST_TC_H

#include "symtable.h"

/**
 * @brief Perform type checking on whole AST
 *
 * @param root Root node of the AST
 * @param symtable Pointer to symbol table
 * @return int Return code; 0 on success, non-zero on failure
 */
int check_ast(ast_node_t *root, symtable_t *symtable);

/**
 * @brief Traverse AST and perform TC and set return types to each node
 *
 * @param parent Current function that is traversed
 * @param line Current line that is traversed
 * @param symtable Pointer to symbol table
 * @return int
 */
int walk_tree(ast_node_t *parent, ast_node_t *line, symtable_t *symtable);

#endif