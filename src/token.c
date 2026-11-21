/**
 * @file token.c
 *
 * @brief FIT VUT | IFJ Project 2026 - token structure implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-11-21
 */

#include "token.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_RETURN_SUCCESS 0
#define TOKEN_RETURN_FAILURE -1

typedef union token_attribute {
  int64_t int_value;
  double double_value;
  char *string_value;
} token_attribute_t;

struct token {
  token_type_t type;
  token_attribute_t attribute;
  bool nl_before;
  int line_number;
};

int token_init(token_t **token, token_type_t type) {
  if (token == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  *token = malloc(sizeof(token_t));
  if (*token == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  (*token)->type = type;
  if (type == STRING || type == IDENTIFIER) {
    (*token)->attribute.string_value = NULL;
  }
  (*token)->nl_before = false;
  (*token)->line_number = 0;
  return TOKEN_RETURN_SUCCESS;
}

int token_get_type(const token_t *token, token_type_t *type) {
  if (token == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  *type = token->type;
  return TOKEN_RETURN_SUCCESS;
}

int token_change_type(token_t *token, token_type_t new_type) {
  if (token == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  if ((token->type == STRING || token->type == IDENTIFIER) &&
      new_type != STRING && new_type != IDENTIFIER) {
    free(token->attribute.string_value);
    token->attribute.string_value = NULL;
  }
  if ((token->type != STRING && token->type != IDENTIFIER) &&
      (new_type == STRING || new_type == IDENTIFIER)) {
    token->attribute.string_value = NULL;
  }
  token->type = new_type;
  return TOKEN_RETURN_SUCCESS;
}

int token_set_int(token_t *token, int64_t value) {
  if (token == NULL || token->type != INTEGER) {
    return TOKEN_RETURN_FAILURE;
  }
  token->attribute.int_value = value;
  return TOKEN_RETURN_SUCCESS;
}

int token_get_int(const token_t *token, int64_t *value) {
  if (token == NULL || token->type != INTEGER || value == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  *value = token->attribute.int_value;
  return TOKEN_RETURN_SUCCESS;
}

int token_set_double(token_t *token, double value) {
  if (token == NULL || token->type != DOUBLE) {
    return TOKEN_RETURN_FAILURE;
  }
  token->attribute.double_value = value;
  return TOKEN_RETURN_SUCCESS;
}

int token_get_double(const token_t *token, double *value) {
  if (token == NULL || token->type != DOUBLE || value == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  *value = token->attribute.double_value;
  return TOKEN_RETURN_SUCCESS;
}

int token_set_string(token_t *token, const char *value) {
  if (token == NULL || (token->type != STRING && token->type != IDENTIFIER) ||
      value == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  if (token->attribute.string_value != NULL) {
    free(token->attribute.string_value);
    token->attribute.string_value = NULL;
  }
  token->attribute.string_value = malloc(strlen(value) + 1);
  if (token->attribute.string_value == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  strcpy(token->attribute.string_value, value);
  return TOKEN_RETURN_SUCCESS;
}

int token_get_string(const token_t *token, const char **value) {
  if (token == NULL || (token->type != STRING && token->type != IDENTIFIER) ||
      value == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  *value = token->attribute.string_value;
  return TOKEN_RETURN_SUCCESS;
}

int token_get_nl_before(const token_t *token, bool *nl_before_val) {
  if (token == NULL || nl_before_val == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  *nl_before_val = token->nl_before;
  return TOKEN_RETURN_SUCCESS;
}

int token_set_nl_before(token_t *token, bool nl_before_val) {
  if (token == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  token->nl_before = nl_before_val;
  return TOKEN_RETURN_SUCCESS;
}

int token_get_line_number(const token_t *token, int *line_number) {
  if (token == NULL || line_number == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  *line_number = token->line_number;
  return TOKEN_RETURN_SUCCESS;
}

int token_set_line_number(token_t *token, int line_number) {
  if (token == NULL) {
    return TOKEN_RETURN_FAILURE;
  }
  token->line_number = line_number;
  return TOKEN_RETURN_SUCCESS;
}

void token_free(token_t *token) {
  if (token == NULL) {
    return;
  }
  if ((token->type == STRING || token->type == IDENTIFIER) &&
      token->attribute.string_value != NULL) {
    free(token->attribute.string_value);
  }
  free(token);
}