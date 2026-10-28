/**
 * @file scanner.c
 *
 * @brief FIT VUT | IFJ Project 2026 - lexical scanner implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-28
 */

#include "stack.h"
#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCANNER_RETURN_SUCCESS 0
#define SCANNER_RETURN_FAILURE -1

int parentheses = 0;
int pending_dedents = 0;
bool pending_divide = false;
bool count_spaces = false;
stack_t *indent_stack = NULL;

bool is_keyword(const char *str, token_type_t *type) {
  char *keywords[] = {"def", "do",     "Double",  "else", "if",    "import",
                      "Int", "return", "String",  "then", "Unit",  "val",
                      "var", "while",  "Boolean", "case", "class", "extension",
                      "for", "List",   "match",   "type", NULL};

  token_type_t types[] = {
      KW_DEF, KW_DO,     KW_DOUBLE,  KW_ELSE, KW_IF,    KW_IMPORT,
      KW_INT, KW_RETURN, KW_STRING,  KW_THEN, KW_UNIT,  KW_VAL,
      KW_VAR, KW_WHILE,  KW_BOOLEAN, KW_CASE, KW_CLASS, KW_EXTENSION,
      KW_FOR, KW_LIST,   KW_MATCH,   KW_TYPE};

  for (int i = 0; keywords[i] != NULL; i++) {
    if (strcmp(str, keywords[i]) == 0) {
      *type = types[i];
      return true;
    }
  }
  return false;
}

int get_next_token(token_t **token) {
  int state = 0;
  char buffer[256];
  int buffer_index = 0;
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
    case 0:
      if (c == ' ' || c == '\t')
        state = 0; // eating whitespaces
      else if (c == '\n') {
        if (parentheses == 0) {
          count_spaces = true;
          tmp_spaces = 0;
          has_tab = false;
          token_init(token, SP_EOL);
          return SCANNER_RETURN_SUCCESS;
        } else
          state = 0;
      } else if (c == EOF) {
        state = 1;
      } else if (c == '+') {
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
      } else if (c == ';') {
        token_init(token, SP_SEMICOLON);
        return SCANNER_RETURN_SUCCESS;
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
        buffer[buffer_index++] = c;
        state = 11;
      } else if (c == '\"') {
        state = 12;
      } else if (c == '0') {
        buffer[buffer_index++] = c;
        state = 19;
      } else if (isdigit(c) && c != '0') {
        buffer[buffer_index++] = c;
        state = 24;
      } else if (c == '.') {
        state = 29;
      } else if (c == '/') {
        state = 30;
      } else {
        return SCANNER_RETURN_FAILURE;
      }
      break;
    case 1: // EOF handling
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
    case 2: // NOT EQUAL
      if (c == '=') {
        token_init(token, OP_NOT_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 3: // GREATER OR EQUAL
      if (c == '=') {
        token_init(token, OP_GREATER_OR_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else {
        ungetc(c, stdin);
        token_init(token, OP_GREATER);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 4: // LESS OR EQUAL
      if (c == '=') {
        token_init(token, OP_LESS_OR_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else {
        ungetc(c, stdin);
        token_init(token, OP_LESS);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 5: // ASSIGN OR EQUAL
      if (c == '=') {
        token_init(token, OP_EQUAL);
        return SCANNER_RETURN_SUCCESS;
      } else {
        ungetc(c, stdin);
        token_init(token, OP_ASSIGN);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 6:
      if (c == 'm')
        state = 7;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 7:
      if (c == 'a')
        state = 8;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 8:
      if (c == 'i')
        state = 9;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 9:
      if (c == 'n') {
        token_init(token, SP_MAIN);
        return SCANNER_RETURN_SUCCESS;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 10:
      if (isalpha(c) || isdigit(c) || c == '_') {
        buffer[buffer_index++] = '_';
        buffer[buffer_index++] = c;
        state = 11;
      } else {
        ungetc(c, stdin);
        token_init(token, SP_UNDERSCORE);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 11:
      if (isalpha(c) || isdigit(c) || c == '_') {
        buffer[buffer_index++] = c;
        state = 11;
      } else {
        ungetc(c, stdin);
        buffer[buffer_index] = '\0';
        token_type_t type;
        if (is_keyword(buffer, &type))
          token_init(token, type);
        else {
          token_init(token, IDENTIFIER);
          token_set_string(*token, buffer);
        }
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 12:
      if (c == '\"')
        state = 15;
      else if (c == '\\')
        state = 14;
      else if (c != '\n' && c != EOF) {
        buffer[buffer_index++] = c;
        state = 13;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 13:
      if (c != '\n' && c != EOF) {
        buffer[buffer_index++] = c;
        state = 13;
      } else if (c == '\\')
        state = 14;
      else if (c == '\"') {
        buffer[buffer_index] = '\0';
        token_init(token, STRING);
        token_set_string(*token, buffer);
        return SCANNER_RETURN_SUCCESS;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 14:
      if (c == '\\') {
        buffer[buffer_index++] = '\\';
        state = 13;
      } else if (c == '\"') {
        buffer[buffer_index++] = '\"';
        state = 13;
      } else if (c == 'n') {
        buffer[buffer_index++] = '\n';
        state = 13;
      } else if (c == 'r') {
        buffer[buffer_index++] = '\r';
        state = 13;
      } else if (c == 't') {
        buffer[buffer_index++] = '\t';
        state = 13;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 15:
      if (c == '\"')
        state = 16;
      else {
        ungetc(c, stdin);
        buffer[buffer_index] = '\0';
        token_init(token, STRING);
        token_set_string(*token, buffer);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 16:
      if (c == '\"') {
        state = 17;
      } else if (c != EOF) {
        buffer[buffer_index++] = c;
        state = 16;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 17:
      if (c == '\"') {
        state = 18;
      } else if (c != EOF) {
        ungetc(c, stdin);
        buffer[buffer_index++] = '\"';
        state = 16;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 18:
      if (c == '\"') {
        buffer[buffer_index] = '\0';
        token_init(token, STRING);
        token_set_string(*token, buffer);
        return SCANNER_RETURN_SUCCESS;
      } else if (c != EOF) {
        ungetc(c, stdin);
        buffer[buffer_index++] = '\"';
        buffer[buffer_index++] = '\"';
        state = 16;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 19:
      if (c == '.') {
        buffer[buffer_index++] = c;
        state = 26;
      } else if (c == 'e' || c == 'E') {
        buffer[buffer_index++] = c;
        state = 20;
      } else {
        ungetc(c, stdin);
        buffer[buffer_index] = '\0';
        token_init(token, INTEGER);
        token_set_int(*token, strtoll(buffer, NULL, 10));
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 20:
      if (c == '+' || c == '-') {
        buffer[buffer_index++] = c;
        state = 21;
      } else if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 22;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 21:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 22;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 22:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 22;
      } else if (c == '_')
        state = 23;
      else {
        if (c != 'd' && c != 'D')
          ungetc(c, stdin);
        buffer[buffer_index] = '\0';
        token_init(token, DOUBLE);
        token_set_double(*token, strtod(buffer, NULL));
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 23:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 22;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
      //----
    case 24:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 24;
      } else if (c == '_')
        state = 25;
      else if (c == 'e' || c == 'E') {
        buffer[buffer_index++] = c;
        state = 20;
      } else if (c == '.') {
        buffer[buffer_index++] = c;
        state = 26;
      } else {
        ungetc(c, stdin);
        buffer[buffer_index] = '\0';
        token_init(token, INTEGER);
        token_set_int(*token, strtoll(buffer, NULL, 10));
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 25:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 24;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 26:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 27;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 27:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 27;
      } else if (c == '_')
        state = 28;
      else {
        if (c != 'd' && c != 'D')
          ungetc(c, stdin);
        buffer[buffer_index] = '\0';
        token_init(token, DOUBLE);
        token_set_double(*token, strtod(buffer, NULL));
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 28:
      if (isdigit(c)) {
        buffer[buffer_index++] = c;
        state = 27;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 29:
      if (isdigit(c)) {
        buffer[buffer_index++] = '0';
        buffer[buffer_index++] = '.';
        buffer[buffer_index++] = c;
        state = 27;
      } else {
        ungetc(c, stdin);
        token_init(token, SP_DOT);
        return SCANNER_RETURN_SUCCESS;
      }
      break;
    case 30:
      if (c == '/')
        state = 31;
      else if (c == '*') {
        star_count = 1;
        has_nl = false;
        state = 32;
      } else if (count_spaces) {
        ungetc(c, stdin);
        pending_divide = true;
        state = 36;
      } else if (!count_spaces) {
        ungetc(c, stdin);
        token_init(token, OP_DIVIDE);
        return SCANNER_RETURN_SUCCESS;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 31:
      if (c == '\n') {
        count_spaces = true;
        tmp_spaces = 0;
        has_tab = false;
        token_init(token, SP_EOL);
        return SCANNER_RETURN_SUCCESS;
      } else if (c == EOF)
        state = 1;
      else
        state = 31;
      break;
    case 32:
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
    case 33:
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
    case 34:
      if (c == '*')
        state = 34;
      else if (c == '/') {
        star_count--;
        if (star_count == 0) {
          if (has_nl)
            state = 35;
          else if (count_spaces)
            state = 36;
          else if (!count_spaces)
            state = 0;
        } else
          state = 32;
      } else if (c != EOF)
        state = 32;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 35:
      if (c == ' ')
        state = 35;
      else if (c == '\n') {
        count_spaces = true;
        tmp_spaces = 0;
        has_tab = false;
        token_init(token, SP_EOL);
        return SCANNER_RETURN_SUCCESS;
      } else if (c == EOF)
        state = 1;
      else
        return SCANNER_RETURN_FAILURE;
      break;
    case 36:
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
        token_init(token, SP_EOL);
        return SCANNER_RETURN_SUCCESS;
      } else if (c == EOF)
        state = 1;
      else if (!has_tab) {
        ungetc(c, stdin);
        count_spaces = false;
        state = 37;
      } else
        return SCANNER_RETURN_FAILURE;
      break;
    case 37:
      if (tmp_spaces == *(int *)stack_top(indent_stack))
        state = 0;
      else if (tmp_spaces > *(int *)stack_top(indent_stack)) {
        stack_push(indent_stack, &tmp_spaces);
        token_init(token, SP_INDENT);
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
          token_init(token, SP_DEDENT);
          return SCANNER_RETURN_SUCCESS;
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

  return SCANNER_RETURN_SUCCESS;
}

void scanner_free() {
  stack_free(indent_stack);
  indent_stack = NULL;
}