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
      } else {
        return SCANNER_RETURN_FAILURE;
      }
      break;

    case 1: // EOF handling
      // TODO: pop indent/dedent stack and set relevant pending variables
      return SCANNER_RETURN_SUCCESS;
      break;
    default:
      return SCANNER_RETURN_FAILURE;
    }
  }

  return 0;
}