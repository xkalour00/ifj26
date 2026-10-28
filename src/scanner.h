/**
 * @file scanner.h
 *
 * @brief FIT VUT | IFJ Project 2026 - lexical scanner header
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-28
 */

#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

/**
 * @brief Initialize the scanner
 * @return 0 on success, -1 on failure
 */
int scanner_init();

/**
 * @brief Frees the resources used by the scanner
 */
void scanner_free();

/**
 * @brief Get the next token from the input
 * @param token Pointer to the token to be created
 * @return 0 on success, -1 on failure
 */
int get_next_token(token_t **token);

#endif // SCANNER_H