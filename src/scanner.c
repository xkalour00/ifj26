/**
 * @file scanner.c
 *
 * @brief FIT VUT | IFJ Project 2026 - lexical scanner implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-28
 */

#include "scanner.h"
#include "stack.h"
#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCANNER_RETURN_SUCCESS 0
#define SCANNER_RETURN_FAILURE -1

#define INITIAL_BUFFER_SIZE 256

typedef struct {
  char *buffer;
  size_t size;
  size_t index;
} buffer_t;

typedef struct {
  const char *keyword;
  token_type_t keyword_type;
} keyword_mapping_t;

static int parentheses = 0;     // used to track parentheses
static int pending_dedents = 0; // used for tracking dedents that need to be
                                // returned from previous runs of the scanner
static bool pending_divide =
    false; // used to track if a divide token needs to be returned from previous
           // run of the scanner
static bool count_spaces = true; // used to track if the scanner is currently
                                 // counting spaces for indentation
static stack_t *indent_stack = NULL; // indendation level stack
static buffer_t *buffer = NULL; // buffer for filling token values (strings,
                                // ints/doubles, identifiers, etc.)
static bool was_last_token_eol =
    true; // used to prevent multiple EOL tokens in a row

/**
 * @brief Add a character to the buffer
 * @param buffer The buffer to add the character to
 * @param c The character to add
 * @return SCANNER_RETURN_SUCCESS if the character was added,
 * SCANNER_RETURN_FAILURE otherwise
 */
int buffer_add_char(buffer_t *buffer, char c) {
  if (buffer->index >=
      buffer->size - 1) { // doubles the buffer size if it's full
    size_t new_size = buffer->size * 2;
    char *new_buffer = realloc(buffer->buffer, new_size);
    if (new_buffer == NULL) {
      return SCANNER_RETURN_FAILURE;
    }
    buffer->buffer = new_buffer;
    buffer->size = new_size;
  }
  buffer->buffer[buffer->index] = c;
  buffer->index++;
  buffer->buffer[buffer->index] = '\0';
  return SCANNER_RETURN_SUCCESS;
}

/**
 * @brief Reset the buffer
 * @param buffer The buffer to reset
 */
void buffer_reset(buffer_t *buffer) {
  buffer->index = 0;
  if (buffer->buffer != NULL) {
    buffer->buffer[0] = '\0';
  }
}

/**
 * @brief Check if a string is a keyword and return its type
 * @param str The string to check
 * @param type Pointer to the token type to be set
 * @return true if the string is a keyword, false otherwise
 */
bool is_keyword(const char *str, token_type_t *type) {
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
 * @return SCANNER_RETURN_SUCCESS if the indentation is correct,
 * SCANNER_RETURN_FAILURE otherwise
 */
int handle_indents(int tmp_spaces, token_type_t *type) {
  if (tmp_spaces > *(int *)stack_top(indent_stack)) {
    stack_push(indent_stack, &tmp_spaces);
    *type = SP_INDENT;
    return SCANNER_RETURN_SUCCESS;
  } else if (tmp_spaces < *(int *)stack_top(indent_stack)) {
    while (tmp_spaces < *(int *)stack_top(indent_stack)) {
      if (*(int *)stack_top(indent_stack) == 0)
        return SCANNER_RETURN_FAILURE;
      stack_pop(indent_stack);
      pending_dedents++;
    }
    if (tmp_spaces == *(int *)stack_top(indent_stack)) {
      pending_dedents--;
      *type = SP_DEDENT;
      return SCANNER_RETURN_SUCCESS;
    }
  }

  return SCANNER_RETURN_FAILURE; // should not be reachable
}

int get_next_token(token_t **token) {
  int state = 0;
  int star_count = 0;
  bool has_nl = false;
  int tmp_spaces = 0;
  bool has_tab = false;

  if (pending_dedents > 0) {
    pending_dedents--;
    token_init(token, SP_DEDENT);
    return SCANNER_RETURN_SUCCESS;
  }

  if (pending_divide) {
    pending_divide = false;
    token_init(token, OP_DIVIDE);
    return SCANNER_RETURN_SUCCESS;
  }

  if (count_spaces) {
    state = 36;
  }

  while (true) {
    int c = getc(stdin);
    switch (state) {
    case 0: // -- START
      if (c == ' ' || c == '\t')
        state = 0; // eating whitespaces
      else if (c == '\n') {
        count_spaces = true;
        tmp_spaces = 0;
        has_tab = false;
        if (parentheses == 0) {
          if (!was_last_token_eol) {
            was_last_token_eol = true;
            token_init(token, SP_EOL);
            return SCANNER_RETURN_SUCCESS;
          } else {
            count_spaces = true;
            tmp_spaces = 0;
            has_tab = false;
            state = 36;
          }
        } else if (parentheses > 0) {
          state = 36;
        } else
          state = 0;
      } else if (c == EOF) {
        state = 1;
      } else if (c == ';') {
        if (!was_last_token_eol) {
          was_last_token_eol = true;
          token_init(token, SP_EOL);
          return SCANNER_RETURN_SUCCESS;
        }
      } else if (c == '/')
        state = 30;
      else {
        was_last_token_eol = false;

        if (c == '+') {
          token_init(token, OP_PLUS);
          return SCANNER_RETURN_SUCCESS;
        } else if (c == '-') {
          token_init(token, OP_MINUS);
          return SCANNER_RETURN_SUCCESS;
        } else if (c == '*') {
          token_init(token, OP_MULTIPLY);
          return SCANNER_RETURN_SUCCESS;
        } else if (c == '(') {
          parentheses++;
          token_init(token, SP_OPENING_PARENTHESIS);
          return SCANNER_RETURN_SUCCESS;
        } else if (c == ')') {
          parentheses--;
          token_init(token, SP_CLOSING_PARENTHESIS);
          return SCANNER_RETURN_SUCCESS;
        } else if (c == '!') {
          state = 2;
        } else if (c == '>') {
          state = 3;
        } else if (c == '<') {
          state = 4;
        } else if (c == '=') {
          state = 5;
        } else if (c == ',') {
          token_init(token, SP_COMMA);
          return SCANNER_RETURN_SUCCESS;
        } else if (c == ':') {
          token_init(token, SP_COLON);
          return SCANNER_RETURN_SUCCESS;
        } else if (c == '@') {
          state = 6;
        } else if (c == '_') {
          state = 10;
        } else if (isalpha(c)) {
          buffer_add_char(buffer, c);
          state = 11;
        } else if (c == '\"') {
          state = 12;
        } else if (c == '0') {
          buffer_add_char(buffer, c);
          state = 19;
        } else if (isdigit(c) && c != '0') {
          buffer_add_char(buffer, c);
          state = 24;
        } else if (c == '.') {
          state = 29;
        } else {
          return SCANNER_RETURN_FAILURE;
        }
      }
      break;
    case 1: // -- EOF
      if (!was_last_token_eol) {
        was_last_token_eol = true;
        token_init(token, SP_EOL);
        return SCANNER_RETURN_SUCCESS;
      }
      while (*(int *)stack_top(indent_stack) > 0) {
        stack_pop(indent_stack);
        pending_dedents++;
      }
      if (pending_dedents == 0) {
        token_init(token, SP_EOF);
        return SCANNER_RETURN_SUCCESS;
      } else if (pending_dedents > 0) {
        pending_dedents--;
        token_init(token, SP_DEDENT);
        return SCANNER_RETURN_SUCCESS;
      } else {
        return SCANNER_RETURN_FAILURE;
      }
      break;
    case 2: // -- NOT EQUAL
      if (c == '=') {
        token_init(token, OP_NOT_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 3: // -- GREATER OR EQUAL
      if (c == '=') {
        token_init(token, OP_GREATER_OR_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else {
        ungetc(c, stdin);
        token_init(token, OP_GREATER);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 4: // -- LESS OR EQUAL
      if (c == '=') {
        token_init(token, OP_LESS_OR_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else {
        ungetc(c, stdin);
        token_init(token, OP_LESS);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 5: // -- ASSIGN OR EQUAL
      if (c == '=') {
        token_init(token, OP_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else {
        ungetc(c, stdin);
        token_init(token, OP_ASSIGN);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 6: // -- @main (m)
      if (c == 'm')
        state = 7;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 7: // -- @main (ma)
      if (c == 'a')
        state = 8;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 8: // -- @main (mai)
      if (c == 'i')
        state = 9;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 9: // -- @main (main)
      if (c == 'n') {
        token_init(token, SP_MAIN);
        return SCANNER_RETURN_SUCCESS;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 10: // -- UNDERSCORE
      if (isalpha(c) || isdigit(c) || c == '_') {
        buffer_add_char(buffer, '_');
        buffer_add_char(buffer, c);
        state = 11;
      } else {
        ungetc(c, stdin);
        token_init(token, SP_UNDERSCORE);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 11: // -- IDENTIFIER
      if (isalpha(c) || isdigit(c) || c == '_') {
        buffer_add_char(buffer, c);
        state = 11;
      } else {
        ungetc(c, stdin);
        token_type_t type;
        if (is_keyword(buffer->buffer, &type))
          token_init(token, type);
        else {
          token_init(token, IDENTIFIER);
          token_set_string(*token, buffer->buffer);
        }
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 12: // -- STRING (opening ")
      if (c == '\"')
        state = 15;
      else if (c == '\\')
        state = 14;
      else if (c != '\n' && c != EOF) {
        buffer_add_char(buffer, c);
        state = 13;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 13: // -- STRING (content)
      if (c == '\\')
        state = 14;
      else if (c == '\"') {
        token_init(token, STRING);
        token_set_string(*token, buffer->buffer);
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      } else if (c != '\n' && c != EOF) {
        buffer_add_char(buffer, c);
        state = 13;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 14: // -- STRING (escape sequences)
      if (c == '\\') {
        buffer_add_char(buffer, '\\');
      } else if (c == '\"') {
        buffer_add_char(buffer, '\"');
      } else if (c == 'n') {
        buffer_add_char(buffer, '\n');
      } else if (c == 'r') {
        buffer_add_char(buffer, '\r');
      } else if (c == 't') {
        buffer_add_char(buffer, '\t');
      } else
        return SCANNER_RETURN_FAILURE;
      state = 13;
      break;
    case 15: // -- STRING (closing " or multi-line entrance)
      if (c == '\"')
        state = 16;
      else {
        ungetc(c, stdin);
        token_init(token, STRING);
        token_set_string(*token, buffer->buffer);
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 16: // -- MULTI-LINE STRING (content)
      if (c == '\"') {
        state = 17;
      } else if (c != EOF) {
        buffer_add_char(buffer, c);
        state = 16;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 17: // -- MULTI-LINE STRING (closing p1 or content)
      if (c == '\"') {
        state = 18;
      } else if (c != EOF) {
        ungetc(c, stdin);
        buffer_add_char(buffer, '\"');
        state = 16;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 18: // -- MULTI-LINE STRING (closing p2 or content)
      if (c == '\"') {
        token_init(token, STRING);
        token_set_string(*token, buffer->buffer);
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      } else if (c != EOF) {
        ungetc(c, stdin);
        buffer_add_char(buffer, '\"');
        buffer_add_char(buffer, '\"');
        state = 16;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 19: // -- INTEGER (starting 0)
      if (isdigit(c) || c == '_') {
        return SCANNER_RETURN_FAILURE;
      } else if (c == '.') {
        buffer_add_char(buffer, c);
        state = 26;
      } else if (c == 'e' || c == 'E') {
        buffer_add_char(buffer, c);
        state = 20;
      } else {
        ungetc(c, stdin);
        token_init(token, INTEGER);
        token_set_int(*token, strtoll(buffer->buffer, NULL, 10));
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 20: // -- DOUBLE (exponent)
      if (c == '+' || c == '-') {
        buffer_add_char(buffer, c);
        state = 21;
      } else if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 22;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 21: // -- DOUBLE (exponent sign)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 22;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 22: // -- DOUBLE (exponent content)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 22;
      } else if (c == '_')
        state = 23;
      else {
        if (c != 'd' && c != 'D')
          ungetc(c, stdin);
        token_init(token, DOUBLE);
        token_set_double(*token, strtod(buffer->buffer, NULL));
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 23: // -- DOUBLE (exponent underscore)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 22;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 24: // -- INTEGER (content)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 24;
      } else if (c == '_')
        state = 25;
      else if (c == 'e' || c == 'E') {
        buffer_add_char(buffer, c);
        state = 20;
      } else if (c == '.') {
        buffer_add_char(buffer, c);
        state = 26;
      } else {
        ungetc(c, stdin);
        token_init(token, INTEGER);
        token_set_int(*token, strtoll(buffer->buffer, NULL, 10));
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 25: // -- INTEGER (underscore)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 24;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 26: // -- DOUBLE (decimal point)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 27;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 27: // -- DOUBLE (decimal content)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 27;
      } else if (c == 'e' || c == 'E') {
        buffer_add_char(buffer, c);
        state = 20;
      } else if (c == '_')
        state = 28;
      else {
        if (c != 'd' && c != 'D')
          ungetc(c, stdin);
        token_init(token, DOUBLE);
        token_set_double(*token, strtod(buffer->buffer, NULL));
        buffer_reset(buffer);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 28: // -- DOUBLE (decimal underscore)
      if (isdigit(c)) {
        buffer_add_char(buffer, c);
        state = 27;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 29: // -- DOT OR DOUBLE START
      if (isdigit(c)) {
        buffer_add_char(buffer, '0');
        buffer_add_char(buffer, '.');
        buffer_add_char(buffer, c);
        state = 27;
      } else {
        ungetc(c, stdin);
        token_init(token, SP_DOT);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 30: // -- DIVIDE OR COMMENT
      if (c == '/')
        state = 31;
      else if (c == '*') {
        star_count = 1;
        has_nl = false;
        state = 32;
      } else if (count_spaces) {
        ungetc(c, stdin);
        count_spaces = false;

        if (has_tab)
          return SCANNER_RETURN_FAILURE;

        if (parentheses > 0) {
          if (tmp_spaces <= *(int *)stack_top(indent_stack)) {
            return SCANNER_RETURN_FAILURE;
          } else {
            token_init(token, OP_DIVIDE);
            return SCANNER_RETURN_SUCCESS;
          }
        }

        if (tmp_spaces == *(int *)stack_top(indent_stack)) {
          token_init(token, OP_DIVIDE);
          return SCANNER_RETURN_SUCCESS;
        } else {
          token_type_t type;
          int to_return = handle_indents(tmp_spaces, &type);
          if (to_return == SCANNER_RETURN_SUCCESS) {
            token_init(token, type);
            pending_divide = true;
          }
          return to_return;
        }
      } else if (!count_spaces) {
        ungetc(c, stdin);
        token_init(token, OP_DIVIDE);
        return SCANNER_RETURN_SUCCESS;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 31: // -- COMMENT (single-line)
      if (c == '\n') {
        count_spaces = true;
        tmp_spaces = 0;
        has_tab = false;
        if (parentheses == 0) {
          if (!was_last_token_eol) {
            was_last_token_eol = true;
            token_init(token, SP_EOL);
            return SCANNER_RETURN_SUCCESS;
          } else {
            count_spaces = true;
            tmp_spaces = 0;
            has_tab = false;
            state = 36;
          }
        } else {
          state = 36;
        }
      } else if (c == EOF)
        state = 1;
      else
        state = 31;
      break;
    case 32: // -- MULTI-LINE COMMENT (start)
      if (c == '/')
        state = 33;
      else if (c == '*')
        state = 34;
      else if (c == '\n') {
        has_nl = true;
        state = 32;
      } else if (c != EOF)
        state = 32;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 33: // -- MULTI-LINE COMMENT (nesting)
      if (c == '*') {
        star_count++;
        state = 32;
      } else if (c == '\n') {
        has_nl = true;
        state = 32;
      } else if (c != EOF) {
        state = 32;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 34: // -- MULTI-LINE COMMENT (ending)
      if (c == '*')
        state = 34;
      else if (c == '/') {
        star_count--;
        if (star_count == 0) {
          if (parentheses > 0) {
            if (has_nl) {
              count_spaces = true;
              tmp_spaces = 0;
              has_tab = false;
              state = 36;
            } else {
              if (count_spaces)
                state = 36;
              else
                state = 0;
            }
          } else if (has_nl)
            state = 35;
          else if (count_spaces)
            state = 36;
          else
            state = 0;
        } else
          state = 32;
      } else if (c != EOF)
        state = 32;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 35: // -- MULTI-LINE COMMENT (new-line after comment handling)
      if (c == ' ')
        state = 35;
      else if (c == '\n') {
        count_spaces = true;
        tmp_spaces = 0;
        has_tab = false;
        if (!was_last_token_eol) {
          was_last_token_eol = true;
          token_init(token, SP_EOL);
          return SCANNER_RETURN_SUCCESS;
        } else {
          count_spaces = true;
          tmp_spaces = 0;
          has_tab = false;
          state = 36;
        }
      } else if (c == EOF)
        state = 1;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 36: // -- INDENT HANDLING
      if (c == '/')
        state = 30;
      else if (c == ' ') {
        tmp_spaces++;
        state = 36;
      } else if (c == '\t') {
        has_tab = true;
        state = 36;
      } else if (c == '\n') {
        count_spaces = true;
        tmp_spaces = 0;
        has_tab = false;
        if (parentheses > 0) {
          state = 36;
        } else if (!was_last_token_eol) {
          was_last_token_eol = true;
          token_init(token, SP_EOL);
          return SCANNER_RETURN_SUCCESS;
        } else {
          tmp_spaces = 0;
          has_tab = false;
          state = 36;
        }
      } else if (c == EOF)
        state = 1;
      else if (!has_tab) {
        ungetc(c, stdin);
        count_spaces = false;
        if (parentheses > 0) {
          if (tmp_spaces <= *(int *)stack_top(indent_stack)) {
            return SCANNER_RETURN_FAILURE;
          }
          state = 0;
        } else if (tmp_spaces == *(int *)stack_top(indent_stack)) {
          state = 0;
        } else {
          token_type_t type;
          int to_return = handle_indents(tmp_spaces, &type);
          if (to_return == SCANNER_RETURN_SUCCESS)
            token_init(token, type);
          return to_return;
        }
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    default:
      return SCANNER_RETURN_FAILURE;
    }
  }

  return SCANNER_RETURN_FAILURE; // should not be reachable
}

int scanner_init() {
  if (stack_init(&indent_stack, sizeof(int)) != 0)
    return SCANNER_RETURN_FAILURE;

  int indent_base = 0;
  stack_push(indent_stack, &indent_base);

  buffer = malloc(sizeof(buffer_t));
  if (buffer == NULL)
    return SCANNER_RETURN_FAILURE;

  buffer->buffer = malloc(INITIAL_BUFFER_SIZE);
  if (buffer->buffer == NULL) {
    free(buffer);
    return SCANNER_RETURN_FAILURE;
  }
  buffer->size = INITIAL_BUFFER_SIZE;
  buffer->index = 0;

  return SCANNER_RETURN_SUCCESS;
}

void scanner_free() {
  stack_free(indent_stack);
  indent_stack = NULL;

  if (buffer != NULL) {
    free(buffer->buffer);
    free(buffer);
    buffer = NULL;
  }
}