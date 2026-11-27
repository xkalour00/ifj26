/**
 * @file main.c
 *
 * @brief FIT VUT | IFJ Project 2026 - main program implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-11-27
 */

#include "ast.h"
#include "ast_tc.h"
#include "codegen.h"
#include "parser.h"
#include "scanner.h"
#include "symtable.h"

#define LEXICAL_ERROR 1
#define SYNTAX_ERROR 2
#define SEMANTIC_ERROR_UNDEFINED 3
#define SEMANTIC_ERROR_REDECLARATION 4
#define SEMANTIC_ERROR_ARGS 5
#define SEMANTIC_ERROR_TYPE 6
#define SEMANTIC_ERROR_CONST 7
#define SEMANTIC_ERROR_OTHER 10
#define INTERNAL_ERROR 99

int main() {
  if (scanner_init() != 0) {
    return INTERNAL_ERROR;
  }

  ast_node_t *root = parse();

  int return_code = check_ast(root, symtable);
  if (return_code != 0) {
    scanner_free(root);
    free_tree(root);
    symtable_free(symtable);
    return return_code;
  }

  generate_code(root, stdout);

  scanner_free();
  free_tree(root);
  symtable_free(symtable);
  return 0;
}
