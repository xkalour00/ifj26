/**
 * @file stack.h
 *
 * @brief FIT VUT | IFJ Project 2026 - stack structure definition
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-17
 */

#ifndef STACK_H
#define STACK_H

#include <stdlib.h>

typedef struct stack stack_t;

/**
 * @brief Initialize a new stack
 *
 * @param[out] stack Pointer to the stack to initialize
 * @param[in] element_size Size of each element in the stack
 *
 * @return 0 on success, non-zero on failure
 *
 * @warning stack must be freed using stack_free() at the end
 */
int stack_init(stack_t **stack, size_t element_size);

/**
 * @brief Push an element onto the stack
 *
 * @param[in,out] stack The stack to push the element onto
 * @param[in] data The data to push onto the stack
 *
 * @return 0 on success, non-zero on failure
 *
 * @warning data is copied into the stack (the original data can be freed after
 * this call) -- stack must be freed using stack_free() at the end
 */
int stack_push(stack_t *stack, const void *data);

/**
 * @brief Pop an element from the stack
 *
 * @param[in,out] stack The stack to pop the element from
 *
 * @return 0 on success, non-zero on failure
 */
int stack_pop(stack_t *stack);

/**
 * @brief Get the top element of the stack without removing it
 *
 * @param[in] stack The stack to get the top element from
 *
 * @return Pointer to the top element on success, NULL on failure
 */
void *stack_top(const stack_t *stack);

/**
 * @brief Get the element at the specified index in the stack
 *
 *@param[in] stack The stack to get the element from
 *@param[in] index The index of the element to get (0 is the top of the stack)
 *
 * @return Pointer to the element at the specified index on success, NULL on
 * error
 */
void *stack_get(const stack_t *stack, size_t index);

/**
 * @brief Check if the stack is empty
 *
 * @param[in] stack The stack to check
 *
 * @return 1 if the stack is empty, 0 if the stack is not empty, -1 on error
 */
int stack_is_empty(const stack_t *stack);

/**
 * @brief Free the memory allocated for the stack
 *
 * @param stack The stack to free
 */
void stack_free(stack_t *stack);

#endif // STACK_H