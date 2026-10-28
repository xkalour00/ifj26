/**
 * @file scanner.c
 *
 * @brief FIT VUT | IFJ Project 2026 - lexical scanner implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-28
 */

#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SCANNER_RETURN_SUCCESS 0
#define SCANNER_RETURN_FAILURE -1

int parentheses = 0;

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

  while (true) {
    int c = getc(stdin);
    switch (state) {
    case 0:
      if (c == ' ' || c == '\t')
        state = 0; // eating whitespaces
      else if (c == '\n') {
        // TODO: toggle count_spaces and relevant variables for indendation
        token_init(token, SP_EOL);
        return SCANNER_RETURN_SUCCESS;
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
      } else {
        return SCANNER_RETURN_FAILURE;
      }
      break;

    case 1: // EOF handling
      // TODO: pop indent/dedent stack and set relevant pending variables
      return SCANNER_RETURN_SUCCESS;
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
    default:
      return SCANNER_RETURN_FAILURE;
    }
  }

  return 0;
}