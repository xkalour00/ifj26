/**
 * @file ast_test_nodes.c
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - Test program for AST
 *
 * @date 2026-11-17
 */

#include "../src/ast.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ast_node_t *make_list() {
  static int64_t i = 13;

  ast_node_t *llist = init_llist();

  if (!llist) {
    fprintf(stderr, "Linked list allocation error");
    return NULL;
  }

  for (int j = 0; j < i; ++j) {
    int64_t x = i * j;
    ast_node_t *node = make_node(AST_INTEGER, &x);

    if (!node) {
      fprintf(stderr, "Linked list item allocation error");
      free_tree(llist);
      return NULL;
    }

    int res = add_llist_item(llist, node);

    if (res) {
      fprintf(stderr, "Linked list addition error");
      free_tree(llist);
      free_tree(node);
      return NULL;
    }
  } // for (int j = 0; j < i; ++j)

  ++i;

  return llist;
} // ast_node_t *make_list()

void test(const char *description, void func(ast_node_t *root)) {
  static uint32_t counter = 1;

  ast_node_t *root = make_root_node();

  if (!root) {
    fprintf(stderr, "Root allocation error\n");
    exit(1);
  }

  printf("TEST %d: %s\n", counter++, description);

  func(root);
}

void test_free_empty_root(ast_node_t *root) {
  // Test freeing empty root tree

  free_tree(root);
}

void test_leaf_node(ast_node_t *root) {
  // Not necessarry in this test
  free_tree(root);

  char *str = malloc(sizeof(char) * 142);

  if (!str) {
    fprintf(stderr, "String allocation error");
    exit(1);
  }

  strcpy(str, "lorem_ipsum");

  ast_node_t *node = make_node(AST_LEAF, str);

  if (!node) {
    fprintf(stderr, "Leaf node allocation error");
    exit(1);
  }

  free_tree(node);
}
void test_int_node(ast_node_t *root) {
  free_tree(root);

  int64_t i = 42123456;

  ast_node_t *node = make_node(AST_INTEGER, &i);

  if (!node) {
    fprintf(stderr, "Integer node allocation error");
    exit(1);
  }

  free_tree(node);
}
void test_double_node(ast_node_t *root) { free_tree(root); }
void test_string_node(ast_node_t *root) { free_tree(root); }
void test_unit_node(ast_node_t *root) { free_tree(root); }
void test_empty_llist_node(ast_node_t *root) { free_tree(root); }
void test_filled_llist_node(ast_node_t *root) { free_tree(root); }

void test_free_main_other_root(ast_node_t *root) {
  // Allocate some chars
  char *func_name1 = malloc(sizeof(char) * 142);

  if (!func_name1) {
    fprintf(stderr, "First string allocation error");
    free_tree(root);
    exit(1);
  }

  strcpy(func_name1, "lorem_ipsum_dolor_sit_amet");

  ast_node_t *param = make_list();

  if (!param) {
    free(func_name1);
    free_tree(root);
  }

  ast_node_t *body = make_list();

  if (!body) {
    free(func_name1);
    free_tree(param);
    free_tree(root);
  }

  add_func_def_tree(root, RETURN_UNIT, func_name1, param, body, false);

  char *func_name2 = malloc(sizeof(char) * 20);

  if (!func_name2) {
    fprintf(stderr, "Second string allocation error");
    free_tree(root);
    exit(1);
  }

  strcpy(func_name2, "dolor_ipsum");

  ast_node_t *param2 = make_list();

  if (!param2) {
    free(func_name2);
    free_tree(root);
  }

  ast_node_t *body2 = make_list();

  if (!body2) {
    free(func_name2);
    free_tree(param2);
    free_tree(root);
  }

  add_func_def_tree(root, RETURN_DOUBLE, func_name2, param2, body2, true);

  free_tree(root);
}

int main() {
  test("Free empty root node", test_free_empty_root);
  test("Create leaf node", test_leaf_node);
  test("Create integer node", test_int_node);
  // test("Create double node", test_double_node);
  // test("Create string node", test_string_node);
  // test("Create unit node", test_unit_node);
  // test("Create empty llist node", test_empty_llist_node);
  // test("Create filled llist node", test_filled_llist_node);
  test("Free root with main and 1 other function", test_free_main_other_root);

  return 0;
}
