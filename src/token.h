/**
 * @file token.h
 *
 * @brief FIT VUT | IFJ Project 2026 - token structure definition
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-17
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

typedef enum token_type {
  IDENTIFIER,
  INTEGER,
  DOUBLE,
  STRING,
  // keywords:
  KW_DEF,
  KW_DO,
  KW_DOUBLE,
  KW_ELSE,
  KW_IF,
  KW_IMPORT,
  KW_INT,
  KW_RETURN,
  KW_STRING,
  KW_THEN,
  KW_UNIT,
  KW_VAL,
  KW_VAR,
  KW_WHILE,
  // extension-only keywords: (might not have an implementation)
  KW_BOOLEAN,
  KW_CASE,
  KW_CLASS,
  KW_EXTENSION,
  KW_FOR,
  KW_LIST,
  KW_MATCH,
  KW_TYPE,
  // operators:
  OP_PLUS,
  OP_MINUS,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_EQUAL,
  OP_NOT_EQUAL,
  OP_ASSIGN,
  OP_LESS,
  OP_LESS_OR_EQUAL,
  OP_GREATER,
  OP_GREATER_OR_EQUAL,
  // special symbols:
  SP_INDENT,
  SP_DEDENT,
  SP_EOL,
  SP_EOF,
  SP_MAIN,
  SP_COLON,
  SP_SEMICOLON,
  SP_COMMA,
  SP_DOT,
  SP_UNDERSCORE,
  SP_OPENING_PARENTHESIS,
  SP_CLOSING_PARENTHESIS
} token_type_t;

typedef struct token token_t;

/**
 * @brief Initializes a token
 *
 * @param[out] token pointer to the initialised token
 * @param[in] type token type
 *
 * @return 0 on success, non-zero on failure
 *
 * @warning every initialized token must be freed using token_free() at the end
 */
int token_init(token_t **token, token_type_t type);

/**
 * @brief Gets the type of a token
 *
 * @param[in] token pointer to the token whose type is to be retrieved
 * @param[out] type type of the token
 *
 * @return 0 on success, non-zero on failure
 */
int token_get_type(const token_t *token, token_type_t *type);

/**
 * @brief Changes the type of a token
 *
 * @param[in,out] token pointer to the token whose type is to be changed
 * @param[in] new_type new token type
 *
 * @return 0 on success, non-zero on failure
 */
int token_change_type(token_t *token, token_type_t new_type);

/**
 * @brief Sets the integer value of a token
 *
 * @param[in,out] token pointer to the token whose value is to be set
 * @param[in] value new integer value
 *
 * @return 0 on success, non-zero on failure
 */
int token_set_int(token_t *token, int64_t value);

/**
 * @brief Gets the integer value of a token
 *
 * @param[in] token pointer to the token whose value is to be retrieved
 * @param[out] value pointer to the location where the integer value will be
 * stored
 *
 * @return 0 on success, non-zero on failure
 */
int token_get_int(const token_t *token, int64_t *value);

/**
 * @brief Sets the double value of a token
 *
 * @param[in,out] token pointer to the token whose value is to be set
 * @param[in] value new double value
 *
 * @return 0 on success, non-zero on failure
 */
int token_set_double(token_t *token, double value);

/**
 * @brief Gets the double value of a token
 *
 * @param[in] token pointer to the token whose value is to be retrieved
 * @param[out] value pointer to the location where the double value will be
 * stored
 *
 * @return 0 on success, non-zero on failure
 */
int token_get_double(const token_t *token, double *value);

/**
 * @brief Sets the string value of a token
 *
 * @param[in,out] token pointer to the token whose value is to be set
 * @param[in] value new string value
 *
 * @return 0 on success, non-zero on failure
 */
int token_set_string(token_t *token, const char *value);

/**
 * @brief Gets the string value of a token
 *
 * @param[in] token pointer to the token whose value is to be retrieved
 * @param[out] value pointer to the location where the string value will be
 * stored
 *
 * @return 0 on success, non-zero on failure
 */
int token_get_string(const token_t *token, const char **value);

/**
 * @brief Frees a token
 *
 * @param[in] token pointer to token that is to be freed
 */
void token_free(token_t *token);

#endif // TOKEN_H