/**
 * @file hashtable.c
 *
 * @brief FIT VUT | IFJ Project 2026 - hashtable implementation
 * @author Filip Obermajer (xobermf00)
 * @date 2026-10-19
 */

#include "hashtable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HASHTABLE_INITIAL_SIZE 512 // 9th power of 2
#define HASHTABLE_RETURN_FAILURE -1
#define HASHTABLE_RETURN_SUCCESS 0
#define HASHTABLE_RETURN_NOT_FOUND 1

typedef enum hashtable_state {
  HASHTABLE_EMPTY,
  HASHTABLE_OCCUPIED,
  HASHTABLE_DELETED
} hashtable_state_t;

typedef struct hashtable_item {
  char *key;
  void *value;
  hashtable_state_t state;
} hashtable_item_t;

struct hashtable {
  size_t size;
  size_t count;   // occupied item count
  size_t deleted; // deleted item count
  hashtable_item_t *items;
};

// djb2 hash function -- source: http://www.cse.yorku.ca/~oz/hash.html (edited)
uint64_t hashtable_hash1(const unsigned char *key) {
  uint64_t hash = 5381;
  int c;

  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }

  return hash;
}

// sdbm hash function -- source: http://www.cse.yorku.ca/~oz/hash.html (edited)
uint64_t hashtable_hash2(const unsigned char *key) {
  uint64_t hash = 0;
  int c;

  while ((c = *key++)) {
    hash = c + (hash << 6) + (hash << 16) - hash;
  }

  return hash;
}

/**
 * @brief Returns the index of a value in the hashtable by it's key
 *
 * @param[in] hashtable Pointer to the hashtable
 * @param[in] key Key of the value to find
 * @param[out] found Pointer to a boolean indicating if the key was found
 *
 * @return Index of the value in the hashtable, -1 on failure
 */
int hashtable_get_index(const hashtable_t *hashtable, const char *key,
                        bool *found) {
  int h2 = (hashtable_hash2((const unsigned char *)key) % hashtable->size) |
           1; // ensure h2 is odd to prevent collisions

  int idx = hashtable_hash1((const unsigned char *)key) % hashtable->size;
  int first_deleted_idx = -1;
  for (size_t i = 0; i < hashtable->size; i++) {
    if (hashtable->items[idx].state == HASHTABLE_OCCUPIED) {
      if (strcmp(hashtable->items[idx].key, key) == 0) {
        *found = true;
        return idx;
      } else {
        idx = (idx + h2) % hashtable->size; // double hashing
        continue;
      }
    }
    if (hashtable->items[idx].state == HASHTABLE_DELETED) {
      if (first_deleted_idx == -1) {
        first_deleted_idx = idx;
      }
      idx = (idx + h2) % hashtable->size; // double hashing
      continue;
    }
    if (hashtable->items[idx].state == HASHTABLE_EMPTY) {
      if (first_deleted_idx != -1) {
        idx = first_deleted_idx; // if we found a deleted item, we can reuse it
      }
      *found = false;
      return idx;
    }
  }

  return HASHTABLE_RETURN_FAILURE;
}

/**
 * @brief Rehashes the hashtable
 *
 * @param[in,out] hashtable The hashtable to rehash
 *
 * @return 0 on success, -1 on failure
 */
int hashtable_rehash(hashtable_t *hashtable) {
  int new_size = hashtable->size * 2; // size has to be power of 2 to prevent
                                      // collisions in double hashing
  hashtable_item_t *new_items = malloc(sizeof(hashtable_item_t) * new_size);
  if (new_items == NULL) {
    return HASHTABLE_RETURN_FAILURE;
  }

  // sets all newly created items to empty state:
  for (int i = 0; i < new_size; i++) {
    new_items[i].state = HASHTABLE_EMPTY;
  }

  // rehashes items from old hashtable to new hashtable:
  for (size_t i = 0; i < hashtable->size; i++) {
    if (hashtable->items[i].state == HASHTABLE_OCCUPIED) {
      hashtable_item_t cur_item = hashtable->items[i];
      int h1 = hashtable_hash1((const unsigned char *)cur_item.key) % new_size;
      int h2 =
          (hashtable_hash2((const unsigned char *)cur_item.key) % new_size) |
          1; // ensure h2 is odd to prevent collisions

      int jdx = h1;
      for (int j = 0; j < new_size; j++) {
        if (new_items[jdx].state == HASHTABLE_EMPTY) {
          new_items[jdx].key = cur_item.key;
          new_items[jdx].value = cur_item.value;
          new_items[jdx].state = HASHTABLE_OCCUPIED;
          break;
        } else {
          jdx = (jdx + h2) % new_size;
          continue;
        }
      }
    }
  }
  free(hashtable->items);
  hashtable->items = new_items;
  hashtable->size = new_size;
  hashtable->deleted = 0;
  return HASHTABLE_RETURN_SUCCESS;
}

int hashtable_init(hashtable_t **hashtable) {
  hashtable_t *new_hashtable = malloc(sizeof(hashtable_t));
  if (new_hashtable == NULL) {
    return HASHTABLE_RETURN_FAILURE;
  }

  hashtable_item_t *items =
      malloc(sizeof(hashtable_item_t) * HASHTABLE_INITIAL_SIZE);
  if (items == NULL) {
    free(new_hashtable);
    *hashtable = NULL;
    return HASHTABLE_RETURN_FAILURE;
  }

  new_hashtable->size = HASHTABLE_INITIAL_SIZE;
  new_hashtable->count = 0;
  new_hashtable->deleted = 0;
  new_hashtable->items = items;

  // sets all newly created items to empty state:
  for (int i = 0; i < HASHTABLE_INITIAL_SIZE; i++) {
    new_hashtable->items[i].state = HASHTABLE_EMPTY;
  }

  *hashtable = new_hashtable;
  return HASHTABLE_RETURN_SUCCESS;
}

int hashtable_count(const hashtable_t *hashtable) {
  if (hashtable != NULL) {
    return hashtable->count;
  }
  return HASHTABLE_RETURN_FAILURE;
}

int hashtable_find(const hashtable_t *hashtable, const char *key,
                   void **value) {
  if (hashtable == NULL || key == NULL) {
    return HASHTABLE_RETURN_FAILURE;
  }
  if (hashtable->count == 0) {
    return HASHTABLE_RETURN_NOT_FOUND;
  }

  bool found = false;
  int idx = hashtable_get_index(hashtable, key, &found);
  if (idx == -1) {
    return HASHTABLE_RETURN_FAILURE;
  }

  if (found) {
    *value = hashtable->items[idx].value;
    return HASHTABLE_RETURN_SUCCESS;
  }

  return HASHTABLE_RETURN_NOT_FOUND;
}

int hashtable_insert(hashtable_t *hashtable, const char *key, void *value) {
  if (hashtable == NULL || key == NULL || value == NULL) {
    return HASHTABLE_RETURN_FAILURE;
  }

  // rehashes the hashtable if more than 75% of the table is non-empty:
  if ((hashtable->count + hashtable->deleted) >= (hashtable->size * 3) / 4) {
    if (hashtable_rehash(hashtable) != 0) {
      return HASHTABLE_RETURN_FAILURE;
    }
  }

  bool found = false;
  int idx = hashtable_get_index(hashtable, key, &found);
  if (idx != -1 && !found) { // found == true is key duplicity - not allowed
    hashtable->items[idx].key = malloc(sizeof(char) * (strlen(key) + 1));
    if (hashtable->items[idx].key == NULL) {
      return HASHTABLE_RETURN_FAILURE;
    }

    strcpy(hashtable->items[idx].key, key);
    hashtable->items[idx].value = value;
    if (hashtable->items[idx].state == HASHTABLE_DELETED) {
      hashtable->deleted--;
    }
    hashtable->items[idx].state = HASHTABLE_OCCUPIED;
    hashtable->count++;
    return HASHTABLE_RETURN_SUCCESS;
  }

  return HASHTABLE_RETURN_FAILURE;
}

int hashtable_find_or_insert(hashtable_t *hashtable, const char *key,
                             bool *found, void **value) {
  // check is skipped as it's checked inside hashtable_find and hashtable_insert
  int found_int = hashtable_find(hashtable, key, value);
  if (found_int == 0) { // found
    *found = true;
    return HASHTABLE_RETURN_SUCCESS;
  }

  if (found_int == 1) { // not found
    *found = false;
    return hashtable_insert(hashtable, key,
                            *value); // returns error code from hashtable_insert
  }

  return HASHTABLE_RETURN_FAILURE;
}

int hashtable_remove(hashtable_t *hashtable, hashtable_item_t *item,
                     void (*free_func)(void *)) {
  if (hashtable == NULL || item == NULL) {
    return HASHTABLE_RETURN_FAILURE;
  }
  free(item->key);
  item->key = NULL;
  if (free_func != NULL && item->value != NULL) {
    free_func(item->value);
  }
  item->value = NULL;
  item->state = HASHTABLE_DELETED;
  hashtable->count--;
  hashtable->deleted++;
  return HASHTABLE_RETURN_SUCCESS;
}

int hashtable_find_and_remove(hashtable_t *hashtable, const char *key) {
  if (hashtable == NULL || key == NULL) {
    return HASHTABLE_RETURN_FAILURE;
  }
  if (hashtable->count == 0) {
    return HASHTABLE_RETURN_NOT_FOUND;
  }

  bool found = false;
  int idx = hashtable_get_index(hashtable, key, &found);
  if (idx != -1 && found) {
    hashtable_remove(hashtable, &hashtable->items[idx], NULL);
    return HASHTABLE_RETURN_SUCCESS;
  }

  return HASHTABLE_RETURN_NOT_FOUND;
}

void hashtable_clear(hashtable_t *hashtable, void (*free_func)(void *)) {
  if (hashtable == NULL) {
    return;
  }
  for (size_t i = 0; i < hashtable->size; i++) {
    if (hashtable->items[i].state == HASHTABLE_OCCUPIED) {
      hashtable_remove(hashtable, &hashtable->items[i], free_func);
    }
  }
  hashtable->count = 0;
  hashtable->deleted = 0;
}

void hashtable_free(hashtable_t *hashtable) {
  if (hashtable == NULL) {
    return;
  }
  for (size_t i = 0; i < hashtable->size; i++) {
    if (hashtable->items[i].state == HASHTABLE_OCCUPIED) {
      free(hashtable->items[i].key);
    }
  }
  free(hashtable->items);
  free(hashtable);
}