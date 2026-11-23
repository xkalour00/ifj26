/**
 * @file codegen.h
 *
 * @author Dávid Čech-Gut
 * @brief FIT VUT | IFJ project 2026
 * @date 2026-11-18
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Main function for generating code from AST
 * @param root Root node of AST
 * @param out Pointer to file destination
 */
int generate_code(const ast_node_t *root, FILE *out);

/**
 * @brief Helper function for easier output of IFJcode26 instructions
 * @param fmt Formatting string
 */
void output(const char *fmt, ...);

#endif