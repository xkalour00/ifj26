/**
 * @file scanner.c
 *
 * @brief FIT VUT | IFJ Project 2026 - lexical scanner implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-28
 */

#include "token.h"
#include <stdbool.h>
#include <stdio.h>

#define SCANNER_RETURN_SUCCESS 0
#define SCANNER_RETURN_FAILURE -1

int parentheses = 0;

int get_next_token(token_t **token) {
  int state = 0;

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
    default:
      return SCANNER_RETURN_FAILURE;
    }
  }

  return 0;
}