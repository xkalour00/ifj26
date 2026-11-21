/**
 * @file ast_tc.h
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - AST type-check function interface header
 *
 * @date 2026-11-21
 */

#ifndef AST_TC_H
#define AST_TC_H

#include "ast.h"

/**
 * @brief Recursively search node for directly reachable functions and add them
 * to stack dr_funcs
 *
 * @param dr_funcs Stack holding names of reachable functions
 * @param ret_node_flag Flag stores whether function has return node
 * @param node Node from which to start searching
 */
void search_for_dr_funcs_return(stack_t *dr_funcs, bool *ret_node_flag,
                                ast_node_t *node);

#endif