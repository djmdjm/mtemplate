/*
 * Copyright (c) 2007 Damien Miller <djm@mindrot.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id$ */

/* Simple generic object system, loosely modelled on Python's */

#ifndef _MOBJECT_H
#define _MOBJECT_H

#include <sys/types.h>
#include <stdint.h>

enum mobject_type {
	TYPE_MNONE,
	TYPE_MSTRING,
	TYPE_MINT,
	TYPE_MARRAY,
	TYPE_MDICT,
};

struct mobject;

struct miteritem {
	struct mobject *key;
	struct mobject *value;
};

/*
 * Allocate a new "None" (empty/placeholder) object
 *
 * Returns: pointer to object or NULL on failure
 */
struct mobject *mnone_new(void);

/*
 * Allocate and populate a new integer object from the integer
 * specified by "v"
 *
 * Returns: pointer to object or NULL on failure
 */
struct mobject *mint_new(int64_t v);

/*
 * Allocate and populate a new string object from the memory buffer
 * specified by "value" and "len".
 *
 * The contents of the supplied memory buffer is _copied_ into object,
 * so caller is free to deallocate or modify it.
 *
 * Returns: pointer to object or NULL on failure
 */
struct mobject *mstring_new2(const u_int8_t *value, size_t len);

/*
 * Allocate and populate a new string object from the nul-terminated
 * string specified by "value"
 *
 * The contents of "value " is _copied_ into object, so caller is free
 * to deallocate or modify it.
 *
 * Returns: pointer to object or NULL on failure
 */
struct mobject *mstring_new(const char *value);

/*
 * Allocate an empty array object
 *
 * Returns: pointer to object or NULL on failure
 */
struct mobject *marray_new(void);

/*
 * Allocate an empty dictionary object
 *
 * Returns: pointer to object or NULL on failure
 */
struct mobject *mdict_new(void);

/*
 * Deallocate the object "o" and any objects it references.
 * I.e. if "o" is a dictionary or array, then any objects that it
 * holds are deallocated too including (recursively) other dictionaries
 * or arrays.
 */
void mobject_free(struct mobject *o);

/*
 * Returns the type of the specified object
 */
enum mobject_type mobject_type(const struct mobject *obj);

/*
 * Renders the specified object as a nul-terminated string into the
 * buffer "s". Up to "len" characters will be written, including the
 * terminating nul byte. Non-ASCII characters are rendered as octal
 * escape sequences using vis(3) and are safe for printing.
 *
 * Returns: the number of characters that would have been written to
 * "s" if it was of unlimited size, up to SIZE_T_MAX and not including
 * the terminating nul byte. (same as strlcpy, snprintf)
 */
size_t mobject_to_string(const struct mobject *o, char *s, size_t len);

/*
 * Makes "deep copy" copy of the specified object, recursively copying
 * arrays, dictionaries and their members.
 *
 * Returns a copy of the object(s) or NULL on failure
 */
struct mobject *mobject_deepcopy(struct mobject *o);

/*
 * Returns the value of the integer object "v"
 */
int64_t mint_value(const struct mobject *v);

/*
 * Adds the value 'n' to the mint 'v'
 *
 * Returns 0 on success, -1 on failure.
 */
int mint_add(struct mobject *v, int64_t n);

/*
 * Returns the length of a string object "s"
 */
size_t mstring_len(const struct mobject *s);

/*
 * Returns a pointer to the data in the string object "s"
 */
const u_int8_t *mstring_ptr(const struct mobject *s);

/*
 * Prepends the object "object" to the array "array". All existing entries
 * in the array are moved up, so "object" will occupy index 0, and
 * previously existing entries will start at index 1.
 *
 * NB. adding an object to the array transfers ownership of the object
 * to the array. The caller should not modify or deallocate the object
 * afterwards.
 *
 * Returns 0 on success, -1 on failure
 */
int marray_prepend(struct mobject *array, struct mobject *object);

/*
 * Appends the object "object" to the array "array". The object will
 * be accessible at index (marray_len(array) - 1).
 *
 * NB. adding an object to the array transfers ownership of the object
 * to the array. The caller should not modify or deallocate the object
 * afterwards.
 *
 * Returns 0 on success, -1 on failure
 */
int marray_append(struct mobject *array, struct mobject *object);
struct mobject *marray_append_s(struct mobject *array, const char *v);
struct mobject *marray_append_d(struct mobject *array, const char *v);
struct mobject *marray_append_i(struct mobject *array, int64_t v);

/*
 * Sets entry "ndx" of array "array" to object "object". Any existing object
 * at this location will be deallocated. If the "ndx" refers to a location
 * higher than the greatest currently allocated index, then the array will
 * be expanded and the lower numbered slots filled with None.
 *
 * NB. adding an object to the array transfers ownership of the object
 * to the array. The caller should not modify or deallocate the object
 * afterwards.
 *
 * Returns 0 on success, -1 on failure
 */
int marray_set(struct mobject *array, size_t ndx, struct mobject *object);

/*
 * Returns the number of entries in the array "array"
 */
size_t marray_len(struct mobject *array);

/*
 * Returns the last (highest index) entry in the array "array" or NULL if
 * the array is empty
 *
 * NB. the item is still "owned" by the array and should not be modified
 * by the caller
 */
struct mobject *marray_last(struct mobject *array);

/*
 * Returns the first (lowest index) entry in the array "array" or NULL if
 * the array is empty
 *
 * NB. the item is still "owned" by the array and should not be modified
 * by the caller
 */
struct mobject *marray_first(struct mobject *array);

/*
 * Remove and return the last (highest index) entry in the array "array"
 *
 * Returns the previously highest-numbered item in the array or NULL if the
 * array is empty
 *
 * NB. removing an item from the array transfers ownership for the item to
 * the caller, including the responsibility for deallocation
 */
struct mobject *marray_pop(struct mobject *array);

/*
 * Remove and return the first (lowest index) entry in the array "array".
 * Subsequent array elements are shifted down.
 *
 * Returns the previously lowest-numbered item in the array or NULL if the
 * array is empty
 *
 * NB. removing an item from the array transfers ownership for the item to
 * the caller, including the responsibility for deallocation
 */
struct mobject *marray_pull(struct mobject *array);

/*
 * Return the array element at the specified index "ndx"
 *
 * NB. the item is still "owned" by the array and should not be modified
 * by the caller
 */
struct mobject *marray_item(struct mobject *array, size_t ndx);

/*
 * Return the dictionary item referenced by the specified "key" or NULL
 * if no matching element is found.
 *
 * NB. the item is still "owned" by the dictionary and should not be
 * modified by the caller
 */
struct mobject *mdict_item(const struct mobject *dict,
    const struct mobject *key);
struct mobject *mdict_item_s(const struct mobject *dict, const char *key);

/*
 * Remove and return the dictionary item referenced by the specified "key"
 * or NULL if no matching element is found.
 *
 * NB. removing an item from the dictionary transfers ownership for the
 * item to the caller, including the responsibility for deallocation
 */
struct mobject *mdict_remove(struct mobject *dict, const struct mobject *key);
struct mobject *mdict_remove_s(struct mobject *dict, const char *key);

/*
 * Remove and deallocate a dictionary item referenced by the specified "key"
 * NB. deallocation will be recursive as described in mobject_free()
 *
 * Returns: 0 on success, -1 on failure
 */
int mdict_delete(struct mobject *dict, const struct mobject *key);
int mdict_delete_s(struct mobject *dict, const char *key);

/*
 * Insert an item identified by "key" of value "value" into
 * dictionary "dict". This operation will fail if a item with the same "key"
 * already exists in the dictionary.
 *
 * NB. successfully adding to the dictionary transfers ownership of both
 * the key and the value. The caller should not modify or deallocate the
 * object afterwards. 
 *
 * Returns: 0 on success, -1 on failure
 */
int mdict_insert(struct mobject *dict, struct mobject *key,
    struct mobject *value);
struct mobject *mdict_insert_s(struct mobject *dict, const char *key,
    struct mobject *value);
struct mobject *mdict_insert_ss(struct mobject *dict, const char *key,
    const char *value);
struct mobject *mdict_insert_si(struct mobject *dict, const char *key,
    int64_t value);
struct mobject *mdict_insert_sa(struct mobject *dict, const char *key);
struct mobject *mdict_insert_sd(struct mobject *dict, const char *key);
struct mobject *mdict_insert_sn(struct mobject *dict, const char *key);

/*
 * Insert an item identified by "key" of value "value" into
 * dictionary "dict". If an item with the same key already exists it
 * is removed and deallocated first. Note that deallocation will occur
 * recursively as described in mobject_free()
 *
 * NB. successfully adding to the dictionary transfers ownership of both
 * the key and the value. The caller should not modify or deallocate the
 * object afterwards. 
 *
 * Returns: 0 on success, -1 on failure
 */
int mdict_replace(struct mobject *dict, struct mobject *key,
    struct mobject *value);
struct mobject *mdict_replace_s(struct mobject *dict, const char *key,
    struct mobject *value);
struct mobject *mdict_replace_ss(struct mobject *dict, const char *key,
    const char *value);
struct mobject *mdict_replace_si(struct mobject *dict, const char *key,
    int64_t value);
struct mobject *mdict_replace_sa(struct mobject *dict, const char *key);
struct mobject *mdict_replace_sd(struct mobject *dict, const char *key);
struct mobject *mdict_replace_sn(struct mobject *dict, const char *key);

/*
 * Returns the number of items in a dictionary
 */
size_t mdict_len(const struct mobject *dict);

/*
 * Obtains an iterator over the object "obj". Iteration is
 * supported over arrays and dictionaries.
 *
 * Returns: pointer to iterator object or NULL on failure
 */
struct miterator *mobject_getiter(struct mobject *obj);

/*
 * Free an iterator
 */
void miterator_free(struct miterator *iter);

/*
 * Return the next value in an iteration or NULL if no more
 * values are available.
 */
struct miteritem *miterator_next(struct miterator *iter);

/*
 * Look up a name in a dictionary namespace. Names may combine dictionary
 * and array lookups through multiple levels of recursion. Dictionary lookup
 * is specified as "name.", array lookup as "array[index]".
 * For example the name "a.b[10].c" means, "return item 'c' from the 10th
 * element of the array stored as 'b' in dictionary 'a' found at the root of
 * namespace"
 *
 * "ns" is the namespace to search, "location" is the name as described above,
 * "obj" is a pointer in which the object will be returned, "ebuf" is an
 * error message buffer, "elen" is the length of the "ebuf" buffer.
 *
 * Returns 0 on success, -1 on failure. On failure, up to "elen" characters
 * will be written into "ebuf" describing the error.
 */
int mnamespace_lookup(struct mobject *ns, char *location, struct mobject **obj,
    char *ebuf, size_t elen);

/*
 * Sets the "location" in the namespace 'ns' to contain object "obj",
 * using the namespace syntax described for mnamespace_lookup. If no
 * path through the namespace already exists for the specified "location"
 * this function will create intervening arrays or dictionaries as
 * required. For example assigning an object to "a.b[10].c" will create
 * a dictionary 'a' at the root of the namespace, containing (at key 'b')
 * an array containing (at index 10) a dictionary, whose 'c' key will be
 * set to the specified object.
 *
 * NB. Assigning an object to a position in a namespace transfers ownership
 * of the object from the caller. The caller should not modify or 
 * deallocate the object afterwards.
 *
 * Returns 0 on success, -1 on failure. On failure, up to "elen" characters
 * will be written into "ebuf" describing the error.
 */ 
int mnamespace_set(struct mobject *ns, char *location, struct mobject *obj,
    char *ebuf, size_t elen);

#endif /* _MOBJECT_H */
