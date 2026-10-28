/**
 * @file scanner.h
 *
 * @brief FIT VUT | IFJ Project 2026 - lexical scanner header
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-27
 */

#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

int get_next_token(token_t **token);

#endif // SCANNER_H