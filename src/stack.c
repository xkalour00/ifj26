/**
 * @file stack.c
 *
 * @brief FIT VUT | IFJ Project 2026 - stack structure implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-17
 */

#include "stack.h"
#include <stdlib.h>
#include <string.h>

#define STACK_RETURN_SUCCESS 0
#define STACK_RETURN_FAILURE -1
#define STACK_RETURN_EMPTY 1

typedef struct stack_element {
  void *data;
  struct stack_element *next;
} stack_element_t;

struct stack {
  stack_element_t *top;
  size_t element_size;
};

int stack_init(stack_t **stack, size_t element_size) {
  if (stack == NULL || element_size == 0) {
    return STACK_RETURN_FAILURE;
  }
  stack_t *new_stack = malloc(sizeof(stack_t));
  if (new_stack == NULL) {
    return STACK_RETURN_FAILURE;
  }
  new_stack->top = NULL;
  new_stack->element_size = element_size;
  *stack = new_stack;
  return STACK_RETURN_SUCCESS;
}

int stack_push(stack_t *stack, const void *data) {
  if (stack == NULL || data == NULL) {
    return STACK_RETURN_FAILURE;
  }
  stack_element_t *new_element = malloc(sizeof(stack_element_t));
  if (new_element == NULL) {
    return STACK_RETURN_FAILURE;
  }
  new_element->data = malloc(stack->element_size);
  if (new_element->data == NULL) {
    free(new_element);
    return STACK_RETURN_FAILURE;
  }
  memcpy(new_element->data, data, stack->element_size);
  new_element->next = stack->top;
  stack->top = new_element;
  return STACK_RETURN_SUCCESS;
}

int stack_pop(stack_t *stack) {
  if (stack == NULL || stack->top == NULL) {
    return STACK_RETURN_FAILURE;
  }
  stack_element_t *element_to_pop = stack->top;
  stack->top = element_to_pop->next;
  free(element_to_pop->data);
  free(element_to_pop);
  return STACK_RETURN_SUCCESS;
}

void *stack_top(const stack_t *stack) {
  if (stack == NULL || stack->top == NULL) {
    return NULL;
  }
  return stack->top->data;
}

int stack_is_empty(const stack_t *stack) {
  if (stack == NULL) {
    return STACK_RETURN_FAILURE;
  }
  if (stack->top == NULL) {
    return STACK_RETURN_EMPTY;
  }
  return STACK_RETURN_SUCCESS;
}

void stack_free(stack_t *stack) {
  if (stack == NULL) {
    return;
  }
  stack_element_t *current = stack->top;
  stack_element_t *next;
  while (current != NULL) {
    next = current->next;
    free(current->data);
    free(current);
    current = next;
  }
  free(stack);
}
