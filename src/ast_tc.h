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

#include "ast.h"
#include "symtable.h"

/**
 * @brief
 *
 * @param parent
 * @param line
 * @param symtable
 * @return int
 */
int walk_tree(ast_node_t *parent, ast_node_t *line, symtable_t *symtable);

/**
 * @brief Perform type checking on whole AST
 *
 * @param root Root node of the AST
 * @param symtable Pointer to symbol table
 * @return int Return code; 0 on success, non-zero on failure
 */
int check_ast(ast_node_t *root, symtable_t *symtable);

#endif