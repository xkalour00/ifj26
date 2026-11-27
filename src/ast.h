/**
 * @file ast.h
 *
 * @author Radek Kalous (xkalour00)
 * @brief FIT VUT | IFJ Project 2026 - Abstract Syntax Tree interface header
 *
 * @date 2026-11-27
 */

#ifndef AST_H
#define AST_H

#include "stack.h"
#include "symtable.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enumeration of all types a node may become,
 * tells us what to expect in .data
 */
typedef enum {
  AST_INTEGER, // constant integer
  AST_DOUBLE,  // constant double
  AST_LEAF,    // leaf node (only ptr to symtable)
  AST_STRING,
  AST_BOOLEAN,
  AST_BINARY, // binary operation (+,-,=,< etc.)
  AST_LINKED_LIST,
  AST_LINKED_LIST_ITEM,
  AST_WHILE_LOOP,
  AST_FOR_LOOP,
  AST_IF_STATEMENT,
  AST_ELSE_IF,
  AST_FUNC_DEF,
  AST_FUNC_CALL,
  AST_ROOT,
  AST_RETURN,
  AST_DECLARATION,
  AST_UNIT,
} ast_node_type_t;

/**
 * @brief Enumeration of all binary operators
 */
typedef enum {
  AST_ADDITION,
  AST_SUBTRACTION,
  AST_MULTIPLICATION,
  AST_DIVISION,
  AST_ASSIGNMENT,
  AST_EQUAL,
  AST_NEQUAL,
  AST_LEQUAL,
  AST_GEQUAL,
  AST_LESS,
  AST_GREATER,
  AST_NOT,
  AST_AND,
  AST_OR,
} ast_binary_type_t;

typedef enum {
  AST_BIN_ARITH,
  AST_BIN_LOGIC,
  AST_BIN_ASSIGN,
} ast_binary_class_t;

typedef enum {
  RETURN_NONE,
  RETURN_DOUBLE,
  RETURN_INT,
  RETURN_STRING,
  RETURN_UNIT,
  RETURN_BOOLEAN,
  RETURN_UNKNOWN, // For TC, all significant returns must be known at codegen
} return_type_t;

/**
 * @brief Node connected to other nodes creates a tree,
 * nodes connect to exactly one parent
 */
typedef struct node {
  ast_node_type_t type;
  return_type_t ret_type;
  bool to_double; // This is only to be used when node has ret_type RETURN_INT

  union {
    double fp;
    symbol_t *leaf;
    char *str;
    int64_t integer;
    bool bool_value;
    struct {
      ast_binary_class_t binary_class;
      ast_binary_type_t binary_type;
      struct node *left;
      struct node *right;
    } binary;
    struct {
      struct node *begin;
      struct node *end;
    } llist;
    struct {
      struct node *node;
      struct node *next;
    } llist_item;
    struct {
      struct node *condition;
      struct node *while_block;
    } while_statement;
    struct {
      struct node *gen_start;
      struct node *gen_end;
      struct node *gen_step; // could be NULL
      bool gen_is_to;
      struct node *llist_guards;
      struct node *code_block;
    } for_loop;
    struct {
      struct node *condition;
      struct node *if_block;
      struct node *else_block;
      struct node *llist_else_if;
    } if_statement;
    struct {
      struct node *else_if_block;
      struct node *cond;
    } else_if;
    struct {
      // Set by parser
      return_type_t return_type;
      struct node *parameters;
      struct node *tree_list;
      char *name;
      // Helper params for TC
      bool possible_recursion;  // True if function can be recursive
      bool has_return_node;     // True if function has at least one return node
      stack_t *return_nodes;    // All return node the function contains
      stack_t *dir_reach_funcs; // Function names the function can directly call
      bool function_walked;
    } func_def;
    struct {
      struct node *parameters; // Expected to be a linked list
      char *name; // Used to find function definition through symtable
    } func_call;
    struct {
      struct node *main_func;
      struct node *func_def_list;
    } root;
    struct {
      struct node *value;
    } return_node;
    struct {
      struct node *value;
      char *name;
      char *codegen_name;
      return_type_t return_type;
    } declaration;
  } data;     // union
} ast_node_t; // typedef struct node

/**
 * @brief Create a node; Used for constant values and leaf nodes
 *
 * @param type Specifies type of node; UNIT, LEAF, INTEGER, DOUBLE, STRING,
 * RETURN are accepted
 * @param data Actual data the node contains, this depends on node type
 * @return ast_node_t* Pointer to newly created node, used for linking multiple
 * nodes together
 */
ast_node_t *make_node(ast_node_type_t type, void *data);

/**
 * @brief Create a AST root node
 *
 * @return ast_node_t* Pointer to the root node
 */
ast_node_t *make_root_node();

/**
 * @brief Create a binary tree with two children; Represents a binary
 * operation
 *
 * @param bin_op Type of binary operation the tree represents
 * @param left Left child, may be any node type, including other trees
 * @param right Right child, may be any node type, including other trees
 * @return ast_node_t* Pointer to parent node of this tree
 */
ast_node_t *make_binary_tree(ast_binary_type_t bin_op, ast_node_t *left,
                             ast_node_t *right);

/**
 * @brief Create a node representing if-else condition
 *
 * @param cond Node defining condition for this statement
 * @param if_block List of trees executed if condition is satisfied;
 * LINKED_LIST is expected here
 * @param elsifs Linked list of ELSE_IFs
 * @param else_block List of trees executed should not condition be satisfied;
 * LINKED_LIST is expected here
 * @return ast_node_t* Pointer to created if-else statement
 */
ast_node_t *make_if_else_tree(ast_node_t *cond, ast_node_t *if_block,
                              ast_node_t *elsifs, ast_node_t *else_block);

/**
 * @brief Create a node representing while loop
 *
 * @param cond Node defining condition for entering loop
 * @param while_block List of trees executed upon satisfying condition;
 * LINKED_LIST type is expected here
 * @return ast_node_t* Pointer to created while loop
 */
ast_node_t *make_while_tree(ast_node_t *cond, ast_node_t *while_block);

/**
 * @brief Initialize an empty linked list
 *
 * @return ast_node_t* Pointer to root of linked list
 */
ast_node_t *init_llist();

/**
 * @brief Append an item to the end of passed linked list
 *
 * @param llist Linked list to append to
 * @param item Node to append
 * @return int Return code; 0 on success, non-zero on failure
 */
int add_llist_item(ast_node_t *llist, ast_node_t *item);

/**
 * @brief Create a node representing function call with specific paramters
 *
 * @param func_name Function name
 * @param params List of parameters
 * @return ast_node_t* Pointer to function call node
 */
ast_node_t *make_func_call_tree(char *func_name, ast_node_t *params);

/**
 * @brief Add function definition to the AST root node
 *
 * @param root_node Root of the AST
 * @param return_type What the function returns
 * @param func_name Function name
 * @param params Parameters passed to the function
 * @param body Body of the function
 * @param is_main Whether the function is program's entry point (only one per
 * program)
 * @return int Return code; 0 on success, non-zero on failure
 */
int add_func_def_tree(ast_node_t *root_node, return_type_t return_type,
                      char *func_name, ast_node_t *params, ast_node_t *body,
                      bool is_main);

/**
 * @brief Create a new declaration
 *
 * @param name Declared variable/constant name
 * @param return_type Variable type
 * @param value Value represented as a tree
 * @return ast_node_t* Pointer to the declaration node
 */
ast_node_t *make_declaration_node(char *name, return_type_t return_type,
                                  ast_node_t *value);

/**
 * @brief Create else-if statement (for linking multiple ifs)
 *
 * @param cond Condition
 * @param block List of nodes that are executed
 * @return ast_node_t* Pointer to created node
 */
ast_node_t *make_else_if_node(ast_node_t *cond, ast_node_t *block);

/**
 * @brief Create for loop node
 *
 * @param i_node Index declaration node
 * @param end Maximum value of generator
 * @param step Optional parameter; defines by how much to increment index
 * @param is_to If true, generator goes to the end including;
 * else is "until" (omits last value)
 * @param llguards Linked list of guards
 * @param block Linked list of lines inside the loop
 * @return ast_node_t* Pointer to created node
 */
ast_node_t *make_for_loop_node(ast_node_t *i_node, ast_node_t *end,
                               ast_node_t *step, bool is_to,
                               ast_node_t *llguards, ast_node_t *block);

/**
 * @brief Deallocate memory used by passed node and its children
 *
 * @param root Root node from which to start deallocating children (including
 * root node itself)
 */
void free_tree(ast_node_t *root);

#endif // #ifndef AST_H