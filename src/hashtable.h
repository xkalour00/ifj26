/**
 * @file hashtable.h
 *
 * @brief FIT VUT | IFJ Project 2026 - hashtable header
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-19
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

typedef struct hashtable hashtable_t;

/**
 * @brief Initializes a hashtable
 *
 * @param[out] hashtable Pointer to the initialized hashtable
 *
 * @return 0 on success, non-zero on failure
 */
int hashtable_init(hashtable_t **hashtable);

/**
 * @brief Returns number of occupied items in the hashtable
 *
 * @param[in] hashtable Pointer to the hashtable
 *
 * @return Number of occupied items in the hashtable. -1 on failure. (invalid
 * hashtable, etc.)
 */
int hashtable_count(const hashtable_t *hashtable);

/**
 * @brief Finds a value in the hashtable by it's key
 *
 * @param[in] hashtable Pointer to the hashtable
 * @param[in] key Key of the value to find
 * @param[out] value Pointer to the found value
 *
 * @return 0 on success, 1 on not found, others on failure
 */
int hashtable_find(const hashtable_t *hashtable, const char *key, void **value);

/**
 * @brief Inserts a value into the hashtable
 *
 * @param[in] hashtable Pointer to the hashtable
 * @param[in] key Key of the value to insert
 * @param[in] value Value to insert
 *
 * @return 0 on success, non-zero on failure
 */
int hashtable_insert(hashtable_t *hashtable, const char *key, void *value);

/**
 * @brief Finds a value in the hashtable by it's key, or adds it if not found
 *
 * @param[in] hashtable Pointer to the hashtable
 * @param[in] key Key of the value to find or add
 * @param[out] found Boolean indicating if the value was found
 * @param[in,out] value Pointer to the found or added value
 *
 * @return 0 on success, non-zero on failure
 */
int hashtable_find_or_insert(hashtable_t *hashtable, const char *key,
                             bool *found, void **value);

/**
 * @brief Removes a value from the hashtable by it's key
 *
 * @param[in] hashtable Pointer to the hashtable
 * @param[in] key Key of the value to remove
 *
 * @return 0 on success, 1 on not found, others on failure
 */
int hashtable_remove(hashtable_t *hashtable, const char *key);

/**
 * @brief Frees the memory allocated for the hashtable
 *
 * @param[in] hashtable Pointer to the hashtable to free
 */
void hashtable_free(hashtable_t *hashtable);

#endif // HASHTABLE_H