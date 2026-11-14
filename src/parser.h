/**
 * @file parser.h
 *
 * @brief FIT VUT | IFJ Project 2026 - parser header
 * @author Patrik Kapec (xkapecp00)
 * @date 14.11.2026
 */

#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"

#define LEXICAL_ERROR 1
#define SYNTAX_ERROR 2
#define MALLOC_FAIL 99


/**
 * @brief parses the whole program
 * @return root node of the ast
 */


ast_node_t* parse(void);

/**
 * @brief frees the current token, the scanner and the AST , then exits
 * @param code exit code (1 = lexical error, 2 = syntax error, 99 = malloc fail i guess)
 */
void fail(int code);

/**
 * @brief frees the current token and loads the next one from the scanner
 *        updates current_token_parser and current_token_type_parser
 *        exit 1 lexical error
 */

void consume(void);

/**
 * @brief checks that the current token == expected type and devoures it
 *        exit 2 if it does not match
 * @param token expected token type
 */

void expect(token_type_t token);

// just passing tokens into precedence parser
extern token_t* current_token_parser;
extern token_type_t current_token_type_parser;




/**
 * @brief copies a string allocated memory exits with 99 if malloc fails, same thing as a strdup from c library
 * @param s string to copy
 * @return pointer to the copy 
 */

char *str_dup(const char *s);

#endif