/**
 * @file scanner.c
 *
 * @brief FIT VUT | IFJ Project 2026 - lexical scanner implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-11-21
 */

#include "scanner.h"
#include "stack.h"
#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCANNER_SUCCESS 0
#define SCANNER_LEXICAL_ERROR 1
#define SCANNER_INTERNAL_ERROR 99

#define INITIAL_BUFFER_SIZE 256
#define INDENT_BASE 0

#define INTERNAL_ERROR_MSG "internal compiler error"

typedef struct {
  char *buffer;
  size_t size;
  size_t index;
} buffer_t;

typedef struct {
  const char *keyword;
  token_type_t keyword_type;
} keyword_mapping_t;

typedef enum scanner_state {
  S_START,
  S_EOF,
  S_NOT_EQUAL,
  S_GREATER_OR_EQUAL,
  S_LESS_OR_EQUAL,
  S_ASSIGN_OR_EQUAL,
  S_AT_MAIN_M,
  S_AT_MAIN_MA,
  S_AT_MAIN_MAI,
  S_AT_MAIN_MAIN,
  S_UNDERSCORE,
  S_ID_OR_KW,
  S_STRING_START,
  S_STRING_CONTENT,
  S_STRING_ESCAPE,
  S_STRING_END,
  S_MULTILINE_STRING_CONTENT,
  S_MULTILINE_STRING_END1,
  S_MULTILINE_STRING_END2,
  S_MULTILINE_STRING_END3,
  S_INT_ZERO,
  S_INT_LEADING_ZEROS,
  S_INT_ZEROS_UNDERSCORE,
  S_DOUBLE_EXP_START,
  S_DOUBLE_EXP_SIGN,
  S_DOUBLE_EXP_CONTENT,
  S_DOUBLE_EXP_UNDERSCORE,
  S_INT_CONTENT,
  S_INT_UNDERSCORE,
  S_DOUBLE_DEC_START,
  S_DOUBLE_DEC_CONTENT,
  S_DOUBLE_DEC_UNDERSCORE,
  S_DOT_OR_DOUBLE_START,
  S_DIV_OR_CMNT_START,
  S_LINE_CMNT,
  S_MULTILINE_CMNT_START,
  S_MULTILINE_CMNT_NEST,
  S_MULTILINE_CMNT_END,
  S_MULTILINE_CMNT_EOL,
  S_INDENT,
} scanner_state_t;

typedef struct {
  int parentheses;         // parentheses tracking
  int pending_dedents;     // pending dedent tokens to return
  bool pending_divide;     // pending divide token to return
  bool count_spaces;       // indendation spaces counting flag
  stack_t *indent_stack;   // stack of indentation levels
  buffer_t *buffer;        // buffer for filling token values (strings, etc.)
  bool was_last_token_eol; // preventing multiple EOL tokens in a row
  bool nl_seen;            // flag to indicate if newline was seen
  int line_number;         // current line number
} scanner_variables_t;

static scanner_variables_t *scanner_vars = NULL;

static int return_error(int return_code, const char *msg, int line_number) {
  if (return_code == SCANNER_INTERNAL_ERROR)
    fprintf(stderr, "error: %s\n", msg);
  else
    fprintf(stderr, "error: %s at line %d\n", msg, line_number);
  return return_code;
}

/**
 * @brief Add a character to the buffer
 * @param buffer The buffer to add the character to
 * @param c The character to add
 * @return SCANNER_SUCCESS if the character was added,
 * SCANNER_INTERNAL_ERROR otherwise
 */
static int buffer_add_char(buffer_t *buffer, char c) {
  if (buffer->index >=
      buffer->size - 1) { // doubles the buffer size if it's full
    size_t new_size = buffer->size * 2;
    char *new_buffer = realloc(buffer->buffer, new_size);
    if (new_buffer == NULL) {
      return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
    }
    buffer->buffer = new_buffer;
    buffer->size = new_size;
  }
  buffer->buffer[buffer->index] = c;
  buffer->index++;
  buffer->buffer[buffer->index] = '\0';
  return SCANNER_SUCCESS;
}

/**
 * @brief Reset the buffer
 * @param buffer The buffer to reset
 */
static void buffer_reset(buffer_t *buffer) {
  buffer->index = 0;
  if (buffer->buffer != NULL) {
    buffer->buffer[0] = '\0';
  }
}

/**
 * @brief Get the current indentation level from the indentation stack
 * @return The current indentation level
 */
static int get_indentation() {
  void *top_element = stack_top(scanner_vars->indent_stack);
  if (top_element == NULL) {
    return INDENT_BASE; // Return base indentation if stack is empty
  }
  int indentation = *(int *)top_element;
  return indentation;
}

/**
 * @brief Check if a string is a keyword and return its type
 * @param str The string to check
 * @param type Pointer to the token type to be set
 * @return true if the string is a keyword, false otherwise
 */
static bool is_keyword(const char *str, token_type_t *type) {
  static const keyword_mapping_t keywords[] = {{"def", KW_DEF},
                                               {"do", KW_DO},
                                               {"Double", KW_DOUBLE},
                                               {"else", KW_ELSE},
                                               {"if", KW_IF},
                                               {"import", KW_IMPORT},
                                               {"Int", KW_INT},
                                               {"return", KW_RETURN},
                                               {"String", KW_STRING},
                                               {"then", KW_THEN},
                                               {"Unit", KW_UNIT},
                                               {"val", KW_VAL},
                                               {"var", KW_VAR},
                                               {"while", KW_WHILE},
                                               {"Boolean", KW_BOOLEAN},
                                               {"case", KW_CASE},
                                               {"class", KW_CLASS},
                                               {"extension", KW_EXTENSION},
                                               {"for", KW_FOR},
                                               {"List", KW_LIST},
                                               {"match", KW_MATCH},
                                               {"type", KW_TYPE},
                                               {NULL, 0}};

  for (int i = 0; keywords[i].keyword != NULL; i++) {
    if (strcmp(str, keywords[i].keyword) == 0) {
      *type = keywords[i].keyword_type;
      return true;
    }
  }

  return false;
}

/**
 * @brief Handle indentation tokens
 * @param tmp_spaces The number of spaces in the current line
 * @param type Pointer to the token type to be set
 * @return SCANNER_SUCCESS if the indentation is correct,
 * SCANNER_LEXICAL_ERROR if the indentation is incorrect and
 * SCANNER_INTERNAL_ERROR if there was an internal error
 */
static int handle_indents(int tmp_spaces, token_type_t *type) {
  if (tmp_spaces > get_indentation()) {
    if (stack_push(scanner_vars->indent_stack, &tmp_spaces) != 0) {
      return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
    }
    *type = SP_INDENT;
    return SCANNER_SUCCESS;
  } else if (tmp_spaces < get_indentation()) {
    while (tmp_spaces < get_indentation()) {
      stack_pop(scanner_vars->indent_stack);
      scanner_vars->pending_dedents++;
    }
    if (tmp_spaces == get_indentation()) {
      scanner_vars->pending_dedents--;
      *type = SP_DEDENT;
      return SCANNER_SUCCESS;
    }
  }

  return return_error(SCANNER_LEXICAL_ERROR, "inconsistent indentation",
                      scanner_vars->line_number);
}

static int return_token(token_t **token, token_type_t type) {
  if (type == SP_EOL) {
    scanner_vars->was_last_token_eol = true;
  }

  if (type == IDENTIFIER) {
    token_type_t tmp_type;
    if (is_keyword(scanner_vars->buffer->buffer, &tmp_type)) {
      type = tmp_type;
      buffer_reset(scanner_vars->buffer);
    }
  }

  if (token_init(token, type) == 0) {
    if (type == INTEGER) {
      if (token_set_int(*token,
                        strtoll(scanner_vars->buffer->buffer, NULL, 10)) != 0) {
        return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
      }
      buffer_reset(scanner_vars->buffer);
    } else if (type == DOUBLE) {
      if (token_set_double(*token,
                           strtod(scanner_vars->buffer->buffer, NULL)) != 0) {
        token_free(*token);
        return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
      }
      buffer_reset(scanner_vars->buffer);
    } else if (type == IDENTIFIER || type == STRING) {
      if (token_set_string(*token, scanner_vars->buffer->buffer) != 0) {
        token_free(*token);
        return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
      }
      buffer_reset(scanner_vars->buffer);
    }

    if (token_set_line_number(*token, scanner_vars->line_number) != 0) {
      token_free(*token);
      return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
    }

    if (token_set_nl_before(*token, scanner_vars->nl_seen) == 0) {
      scanner_vars->nl_seen = false;
      return SCANNER_SUCCESS;
    }
  }

  return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
}

static void newline_started(int *tmp_spaces, bool *has_tab) {
  scanner_vars->count_spaces = true;
  *tmp_spaces = 0;
  *has_tab = false;
}

int get_next_token(token_t **token) {
  scanner_state_t state = S_START;
  int star_count = 0;
  bool has_nl = false;
  int tmp_spaces = 0;
  bool has_tab = false;

  if (scanner_vars->pending_dedents > 0) {
    scanner_vars->pending_dedents--;
    return return_token(token, SP_DEDENT);
  }

  if (scanner_vars->pending_divide) {
    scanner_vars->pending_divide = false;
    return return_token(token, OP_DIVIDE);
  }

  if (scanner_vars->count_spaces) {
    state = S_INDENT;
  }

  while (true) {
    int c = getc(stdin);
    switch (state) {
    case S_START:
      if (c == ' ' || c == '\t')
        state = S_START; // eating whitespaces
      else if (c == '\n') {
        scanner_vars->line_number++;
        newline_started(&tmp_spaces, &has_tab);
        if (scanner_vars->parentheses == 0) {
          if (!scanner_vars->was_last_token_eol) {
            return return_token(token, SP_EOL);
          } else {
            state = S_INDENT;
          }
        } else if (scanner_vars->parentheses > 0) {
          scanner_vars->nl_seen = true;
          state = S_INDENT;
        } else
          state = S_START;
      } else if (c == EOF) {
        state = S_EOF;
      } else if (c == ';') {
        if (!scanner_vars->was_last_token_eol) {
          return return_token(token, SP_EOL);
        }
      } else if (c == '/')
        state = S_DIV_OR_CMNT_START;
      else {
        scanner_vars->was_last_token_eol = false;

        if (c == '+') {
          return return_token(token, OP_PLUS);
        } else if (c == '-') {
          return return_token(token, OP_MINUS);
        } else if (c == '*') {
          return return_token(token, OP_MULTIPLY);
        } else if (c == '(') {
          scanner_vars->parentheses++;
          return return_token(token, SP_OPENING_PARENTHESIS);
        } else if (c == ')') {
          if (scanner_vars->parentheses != 0) {
            scanner_vars->parentheses--;
          }
          return return_token(token, SP_CLOSING_PARENTHESIS);
        } else if (c == '!') {
          state = S_NOT_EQUAL;
        } else if (c == '>') {
          state = S_GREATER_OR_EQUAL;
        } else if (c == '<') {
          state = S_LESS_OR_EQUAL;
        } else if (c == '=') {
          state = S_ASSIGN_OR_EQUAL;
        } else if (c == ',') {
          return return_token(token, SP_COMMA);
        } else if (c == ':') {
          return return_token(token, SP_COLON);
        } else if (c == '@') {
          state = S_AT_MAIN_M;
        } else if (c == '_') {
          state = S_UNDERSCORE;
        } else if (isalpha(c)) {
          if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
            return SCANNER_INTERNAL_ERROR;
          }
          state = S_ID_OR_KW;
        } else if (c == '\"') {
          state = S_STRING_START;
        } else if (c == '0') {
          if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
            return SCANNER_INTERNAL_ERROR;
          }
          state = S_INT_ZERO;
        } else if (isdigit(c) && c != '0') {
          if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
            return SCANNER_INTERNAL_ERROR;
          }
          state = S_INT_CONTENT;
        } else if (c == '.') {
          state = S_DOT_OR_DOUBLE_START;
        } else {
          return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                              scanner_vars->line_number);
        }
      }
      break;
    case S_EOF:
      if (!scanner_vars->was_last_token_eol) {
        return return_token(token, SP_EOL);
      }
      while (get_indentation() > 0) {
        stack_pop(scanner_vars->indent_stack);
        scanner_vars->pending_dedents++;
      }
      if (scanner_vars->pending_dedents == 0) {
        return return_token(token, SP_EOF);
      } else if (scanner_vars->pending_dedents > 0) {
        scanner_vars->pending_dedents--;
        return return_token(token, SP_DEDENT);
      } else {
        return return_error(SCANNER_LEXICAL_ERROR, "inconsistent indentation",
                            scanner_vars->line_number);
      }
      break;
    case S_NOT_EQUAL:
      if (c == '=') {
        return return_token(token, OP_NOT_EQUAL);
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_GREATER_OR_EQUAL:
      if (c == '=') {
        return return_token(token, OP_GREATER_OR_EQUAL);
      } else {
        ungetc(c, stdin);
        return return_token(token, OP_GREATER);
      }
      break;
    case S_LESS_OR_EQUAL:
      if (c == '=') {
        return return_token(token, OP_LESS_OR_EQUAL);
      } else {
        ungetc(c, stdin);
        return return_token(token, OP_LESS);
      }
      break;
    case S_ASSIGN_OR_EQUAL:
      if (c == '=') {
        return return_token(token, OP_EQUAL);
      } else {
        ungetc(c, stdin);
        return return_token(token, OP_ASSIGN);
      }
      break;
    case S_AT_MAIN_M:
      if (c == 'm')
        state = S_AT_MAIN_MA;
      else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_AT_MAIN_MA:
      if (c == 'a')
        state = S_AT_MAIN_MAI;
      else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_AT_MAIN_MAI:
      if (c == 'i')
        state = S_AT_MAIN_MAIN;
      else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_AT_MAIN_MAIN:
      if (c == 'n') {
        return return_token(token, SP_MAIN);
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_UNDERSCORE:
      if (isalpha(c) || isdigit(c) || c == '_') {
        if (buffer_add_char(scanner_vars->buffer, '_') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_ID_OR_KW;
      } else {
        ungetc(c, stdin);
        return return_token(token, SP_UNDERSCORE);
      }
      break;
    case S_ID_OR_KW:
      if (isalpha(c) || isdigit(c) || c == '_') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_ID_OR_KW;
      } else {
        ungetc(c, stdin);
        return return_token(token, IDENTIFIER);
      }
      break;
    case S_STRING_START:
      if (c == '\"')
        state = S_STRING_END;
      else if (c == '\\')
        state = S_STRING_ESCAPE;
      else if (c != '\n' && c != EOF) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_STRING_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_STRING_CONTENT:
      if (c == '\\')
        state = S_STRING_ESCAPE;
      else if (c == '\"') {
        return return_token(token, STRING);
      } else if (c != '\n' && c != EOF) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_STRING_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_STRING_ESCAPE:
      if (c == '\\') {
        if (buffer_add_char(scanner_vars->buffer, '\\') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
      } else if (c == '\"') {
        if (buffer_add_char(scanner_vars->buffer, '\"') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
      } else if (c == 'n') {
        if (buffer_add_char(scanner_vars->buffer, '\n') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
      } else if (c == 'r') {
        if (buffer_add_char(scanner_vars->buffer, '\r') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
      } else if (c == 't') {
        if (buffer_add_char(scanner_vars->buffer, '\t') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      state = S_STRING_CONTENT;
      break;
    case S_STRING_END:
      if (c == '\"')
        state = S_MULTILINE_STRING_CONTENT;
      else {
        ungetc(c, stdin);
        return return_token(token, STRING);
      }
      break;
    case S_MULTILINE_STRING_CONTENT:
      if (c == '\"') {
        state = S_MULTILINE_STRING_END1;
      } else if (c != EOF) {
        if (c == '\n') {
          scanner_vars->line_number++;
        }
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_MULTILINE_STRING_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_MULTILINE_STRING_END1:
      if (c == '\"') {
        state = S_MULTILINE_STRING_END2;
      } else if (c != EOF) {
        ungetc(c, stdin);
        if (buffer_add_char(scanner_vars->buffer, '\"') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_MULTILINE_STRING_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_MULTILINE_STRING_END2:
      if (c == '\"') {
        state = S_MULTILINE_STRING_END3;
      } else if (c != EOF) {
        ungetc(c, stdin);
        if (buffer_add_char(scanner_vars->buffer, '\"') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        if (buffer_add_char(scanner_vars->buffer, '\"') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_MULTILINE_STRING_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_MULTILINE_STRING_END3:
      if (c == '\"') {
        if (buffer_add_char(scanner_vars->buffer, '\"') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_MULTILINE_STRING_END3;
      } else {
        ungetc(c, stdin);
        return return_token(token, STRING);
      }
      break;
    case S_INT_ZERO:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_INT_LEADING_ZEROS;
      } else if (c == '.') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_DEC_START;
      } else if (c == 'e' || c == 'E') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_START;
      } else if (c == '_') {
        state = S_INT_ZEROS_UNDERSCORE;
      } else {
        ungetc(c, stdin);
        return return_token(token, INTEGER);
      }
      break;
    case S_INT_LEADING_ZEROS:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_INT_LEADING_ZEROS;
      } else if (c == '.') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_DEC_START;
      } else if (c == 'e' || c == 'E') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_START;
      } else if (c == '_') {
        state = S_INT_ZEROS_UNDERSCORE;
      } else {
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      }
      break;
    case S_INT_ZEROS_UNDERSCORE:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_INT_LEADING_ZEROS;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_DOUBLE_EXP_START:
      if (c == '+' || c == '-') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_SIGN;
      } else if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_DOUBLE_EXP_SIGN:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_DOUBLE_EXP_CONTENT:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_CONTENT;
      } else if (c == '_')
        state = S_DOUBLE_EXP_UNDERSCORE;
      else {
        if (c != 'd' && c != 'D')
          ungetc(c, stdin);
        return return_token(token, DOUBLE);
      }
      break;
    case S_DOUBLE_EXP_UNDERSCORE:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_INT_CONTENT:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_INT_CONTENT;
      } else if (c == '_')
        state = S_INT_UNDERSCORE;
      else if (c == 'e' || c == 'E') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_START;
      } else if (c == '.') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_DEC_START;
      } else {
        ungetc(c, stdin);
        return return_token(token, INTEGER);
      }
      break;
    case S_INT_UNDERSCORE:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_INT_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_DOUBLE_DEC_START:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_DEC_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_DOUBLE_DEC_CONTENT:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_DEC_CONTENT;
      } else if (c == 'e' || c == 'E') {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_EXP_START;
      } else if (c == '_')
        state = S_DOUBLE_DEC_UNDERSCORE;
      else {
        if (c != 'd' && c != 'D') {
          ungetc(c, stdin);
        }
        return return_token(token, DOUBLE);
      }
      break;
    case S_DOUBLE_DEC_UNDERSCORE:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_DEC_CONTENT;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_DOT_OR_DOUBLE_START:
      if (isdigit(c)) {
        if (buffer_add_char(scanner_vars->buffer, '0') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        if (buffer_add_char(scanner_vars->buffer, '.') != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        if (buffer_add_char(scanner_vars->buffer, c) != SCANNER_SUCCESS) {
          return SCANNER_INTERNAL_ERROR;
        }
        state = S_DOUBLE_DEC_CONTENT;
      } else {
        ungetc(c, stdin);
        return return_token(token, SP_DOT);
      }
      break;
    case S_DIV_OR_CMNT_START:
      if (c == '/')
        state = S_LINE_CMNT;
      else if (c == '*') {
        star_count = 1;
        has_nl = false;
        state = S_MULTILINE_CMNT_START;
      } else if (scanner_vars->count_spaces) {
        ungetc(c, stdin);
        scanner_vars->count_spaces = false;

        if (has_tab)
          return return_error(SCANNER_LEXICAL_ERROR, "invalid indentation",
                              scanner_vars->line_number);

        if (scanner_vars->parentheses > 0) {
          if (tmp_spaces <= get_indentation()) {
            return return_error(SCANNER_LEXICAL_ERROR, "invalid indentation",
                                scanner_vars->line_number);
          } else {
            return return_token(token, OP_DIVIDE);
          }
        }

        if (tmp_spaces == get_indentation()) {
          return return_token(token, OP_DIVIDE);
        } else {
          token_type_t type;
          int to_return = handle_indents(tmp_spaces, &type);
          if (to_return == SCANNER_SUCCESS) {
            scanner_vars->pending_divide = true;
            return return_token(token, type);
          }
          return to_return;
        }
      } else if (!scanner_vars->count_spaces) {
        ungetc(c, stdin);
        return return_token(token, OP_DIVIDE);
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_LINE_CMNT:
      if (c == '\n') {
        if (scanner_vars->parentheses > 0)
          scanner_vars->nl_seen = true;
        scanner_vars->line_number++;
        newline_started(&tmp_spaces, &has_tab);
        if (scanner_vars->parentheses == 0) {
          if (!scanner_vars->was_last_token_eol) {
            return return_token(token, SP_EOL);
          } else {
            state = S_INDENT;
          }
        } else {
          state = S_INDENT;
        }
      } else if (c == EOF)
        state = S_EOF;
      else
        state = S_LINE_CMNT;
      break;
    case S_MULTILINE_CMNT_START:
      if (c == '/')
        state = S_MULTILINE_CMNT_NEST;
      else if (c == '*')
        state = S_MULTILINE_CMNT_END;
      else if (c == '\n') {
        has_nl = true;
        scanner_vars->line_number++;
        state = S_MULTILINE_CMNT_START;
      } else if (c != EOF)
        state = S_MULTILINE_CMNT_START;
      else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_MULTILINE_CMNT_NEST:
      if (c == '/') {
        state = S_MULTILINE_CMNT_NEST;
      } else if (c == '*') {
        star_count++;
        state = S_MULTILINE_CMNT_START;
      } else if (c == '\n') {
        has_nl = true;
        scanner_vars->line_number++;
        state = S_MULTILINE_CMNT_START;
      } else if (c != EOF) {
        state = S_MULTILINE_CMNT_START;
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_MULTILINE_CMNT_END:
      if (c == '*')
        state = S_MULTILINE_CMNT_END;
      else if (c == '/') {
        star_count--;
        if (star_count == 0) {
          if (scanner_vars->parentheses > 0) {
            if (has_nl) {
              scanner_vars->nl_seen = true;
              newline_started(&tmp_spaces, &has_tab);
              state = S_INDENT;
            } else {
              if (scanner_vars->count_spaces)
                state = S_INDENT;
              else
                state = S_START;
            }
          } else if (has_nl)
            state = S_MULTILINE_CMNT_EOL;
          else if (scanner_vars->count_spaces)
            state = S_INDENT;
          else
            state = S_START;
        } else
          state = S_MULTILINE_CMNT_START;
      } else if (c == '\n') {
        has_nl = true;
        scanner_vars->line_number++;
        state = S_MULTILINE_CMNT_START;
      } else if (c != EOF)
        state = S_MULTILINE_CMNT_START;
      else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_MULTILINE_CMNT_EOL:
      if (c == ' ')
        state = S_MULTILINE_CMNT_EOL;
      else if (c == '\n') {
        scanner_vars->line_number++;
        newline_started(&tmp_spaces, &has_tab);
        if (!scanner_vars->was_last_token_eol) {
          return return_token(token, SP_EOL);
        } else {
          state = S_INDENT;
        }
      } else if (c == EOF)
        state = S_EOF;
      else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid character",
                            scanner_vars->line_number);
      break;
    case S_INDENT:
      if (c == '/')
        state = S_DIV_OR_CMNT_START;
      else if (c == ' ') {
        tmp_spaces++;
        state = S_INDENT;
      } else if (c == '\t') {
        has_tab = true;
        state = S_INDENT;
      } else if (c == '\n') {
        scanner_vars->line_number++;
        if (scanner_vars->parentheses > 0)
          scanner_vars->nl_seen = true;
        newline_started(&tmp_spaces, &has_tab);
        if (scanner_vars->parentheses > 0) {
          state = S_INDENT;
        } else if (!scanner_vars->was_last_token_eol) {
          return return_token(token, SP_EOL);
        } else {
          state = S_INDENT;
        }
      } else if (c == EOF)
        state = S_EOF;
      else if (!has_tab) {
        ungetc(c, stdin);
        scanner_vars->count_spaces = false;
        if (scanner_vars->parentheses > 0) {
          if (tmp_spaces <= get_indentation()) {
            return return_error(SCANNER_LEXICAL_ERROR, "invalid indentation",
                                scanner_vars->line_number);
          }
          state = S_START;
        } else if (tmp_spaces == get_indentation()) {
          state = S_START;
        } else {
          token_type_t type;
          int to_return = handle_indents(tmp_spaces, &type);
          if (to_return == SCANNER_SUCCESS)
            return return_token(token, type);
          return to_return;
        }
      } else
        return return_error(SCANNER_LEXICAL_ERROR, "invalid indentation",
                            scanner_vars->line_number);
      break;
    }
  }
}

int scanner_init() {
  scanner_vars = malloc(sizeof(scanner_variables_t));
  if (scanner_vars == NULL) {
    return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
  }
  scanner_vars->parentheses = 0;
  scanner_vars->pending_dedents = 0;
  scanner_vars->pending_divide = false;
  scanner_vars->count_spaces = true;
  scanner_vars->was_last_token_eol = true;
  scanner_vars->nl_seen = false;
  scanner_vars->line_number = 1;

  if (stack_init(&scanner_vars->indent_stack, sizeof(int)) != 0) {
    free(scanner_vars);
    return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
  }

  int indent_base = INDENT_BASE;
  if (stack_push(scanner_vars->indent_stack, &indent_base) != 0) {
    stack_free(scanner_vars->indent_stack);
    free(scanner_vars);
    return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
  }

  scanner_vars->buffer = malloc(sizeof(buffer_t));
  if (scanner_vars->buffer == NULL) {
    stack_free(scanner_vars->indent_stack);
    free(scanner_vars);
    return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
  }

  scanner_vars->buffer->buffer = malloc(INITIAL_BUFFER_SIZE);
  if (scanner_vars->buffer->buffer == NULL) {
    stack_free(scanner_vars->indent_stack);
    free(scanner_vars->buffer);
    free(scanner_vars);
    return return_error(SCANNER_INTERNAL_ERROR, INTERNAL_ERROR_MSG, -1);
  }
  scanner_vars->buffer->size = INITIAL_BUFFER_SIZE;
  scanner_vars->buffer->index = 0;
  scanner_vars->buffer->buffer[0] = '\0';

  return SCANNER_SUCCESS;
}

void scanner_free() {
  if (scanner_vars != NULL) {
    if (scanner_vars->indent_stack != NULL) {
      stack_free(scanner_vars->indent_stack);
      scanner_vars->indent_stack = NULL;
    }

    if (scanner_vars->buffer != NULL) {
      free(scanner_vars->buffer->buffer);
      free(scanner_vars->buffer);
      scanner_vars->buffer = NULL;
    }

    free(scanner_vars);
    scanner_vars = NULL;
  }
}