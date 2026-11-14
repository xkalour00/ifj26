/**
 * @file precedence_parser.h
 *
 * @brief FIT VUT | IFJ Project 2026 - precedence parser header
 * @author Patrik Kapec (xkapecp00)
 * @date 14.11.2026
 */

#ifndef PARSER_PRECEDENCE_H
#define PARSER_PRECEDENCE_H

#include "ast.h"

/**
 * @brief parses an expression using the precedence table
 *        stops before EOL or then  or do  or a comma or an unmatched closing parenthesis
 *        and doesnt eat the mentioned token
 * @param identifier_consumed identifier the caller already read or NULL
 * @return ast node of the expression
 */

ast_node_t *parse_precedence(char* identifier_consumed);

#endif