/*
 * Copyright (c) Damien Miller <djm@mindrot.org>
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

#ifndef _XOBJECT_H
#define _XOBJECT_H

#include <sys/types.h>
#include <stdint.h>

enum xobject_type {
	TYPE_XNONE,
	TYPE_XSTRING,
	TYPE_XINT,
	TYPE_XARRAY,
	TYPE_XDICT,
};

struct xobject;
struct xnone;
struct xstring;
struct xint;
struct xarray;
struct xdict;
struct xiterator;

struct xiteritem {
	const struct xobject *key;
	const struct xobject *value;
};

/*
 * Allocate a new "None" (empty/placeholder) object
 *
 * Returns: pointer to object or NULL on failure
 */
struct xnone *xnone_new(void);

/*
 * Allocate and populate a new integer object from the integer
 * specified by "v"
 *
 * Returns: pointer to object or NULL on failure
 */
struct xint *xint_new(int64_t v);

/*
 * Allocate and populate a new string object from the memory buffer
 * specified by "value" and "len".
 *
 * The contents of the supplied memory buffer is _copied_ into object,
 * so caller is free to deallocate or modify it.
 *
 * Returns: pointer to object or NULL on failure
 */
struct xstring *xstring_new2(const u_char *value, size_t len);

/*
 * Allocate and populate a new string object from the nul-terminated
 * string specified by "value"
 *
 * The contents of "value " is _copied_ into object, so caller is free
 * to deallocate or modify it.
 *
 * Returns: pointer to object or NULL on failure
 */
struct xstring *xstring_new(const u_char *value);

/*
 * Allocate an empty array object
 *
 * Returns: pointer to object or NULL on failure
 */
struct xarray *xarray_new(void);

/*
 * Allocate an empty dictionary object
 *
 * Returns: pointer to object or NULL on failure
 */
struct xdict *xdict_new(void);

/*
 * Deallocate the object "o" and any objects it references.
 * I.e. if "o" is a dictionary or array, then any objects that it
 * holds are deallocated too including (recursively) other dictionaries
 * or arrays.
 */
void xobject_free(struct xobject *o);

/*
 * Returns the type of the specified object
 */
enum xobject_type xobject_type(const struct xobject *obj);

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
size_t xobject_to_string(const struct xobject *o, u_char *s, size_t len);

/*
 * Returns the value of the integer object "v"
 */
int64_t xint_value(const struct xint *v);

/*
 * Returns the length of a string object "s"
 */
size_t xstring_len(const struct xstring *s);

/*
 * Returns a pointer to the data in the string object "s"
 */
const u_char *xstring_ptr(const struct xstring *s);

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
int xarray_prepend(struct xarray *array, struct xobject *object);

/*
 * Appends the object "object" to the array "array". The object will
 * be accessible at index (xarray_len(array) - 1).
 *
 * NB. adding an object to the array transfers ownership of the object
 * to the array. The caller should not modify or deallocate the object
 * afterwards.
 *
 * Returns 0 on success, -1 on failure
 */
int xarray_append(struct xarray *array, struct xobject *object);

/*
 * Returns the number of entries in the array "array"
 */
size_t xarray_len(struct xarray *array);

/*
 * Returns the last (highest index) entry in the array "array" or NULL if
 * the array is empty
 *
 * NB. the item is still "owned" by the array and should not be modified
 * by the caller
 */
const struct xobject *xarray_last(struct xarray *array);

/*
 * Returns the first (lowest index) entry in the array "array" or NULL if
 * the array is empty
 *
 * NB. the item is still "owned" by the array and should not be modified
 * by the caller
 */
const struct xobject *xarray_first(struct xarray *array);

/*
 * Remove and return the last (highest index) entry in the array "array"
 *
 * Returns the previously highest-numbered item in the array or NULL if the
 * array is empty
 *
 * NB. removing an item from the array transfers ownership for the item to
 * the caller, including the responsibility for deallocation
 */
struct xobject *xarray_pop(struct xarray *array);

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
struct xobject *xarray_pull(struct xarray *array);

/*
 * Return the array element at the specified index "ndx"
 *
 * NB. the item is still "owned" by the array and should not be modified
 * by the caller
 */
const struct xobject *xarray_item(struct xarray *array, size_t ndx);

/*
 * Return the dictionary item referenced by the specified "key" or NULL
 * if no matching element is found.
 *
 * NB. the item is still "owned" by the dictionary and should not be
 * modified by the caller
 */
const struct xobject *xdict_item(struct xdict *dict,
    const struct xstring *key);

/*
 * Remove and return the dictionary item referenced by the specified "key"
 * or NULL if no matching element is found.
 *
 * NB. removing an item from the dictionary transfers ownership for the
 * item to the caller, including the responsibility for deallocation
 */
struct xobject *xdict_remove(struct xdict *dict, const struct xstring *key);

/*
 * Remove and deallocate a dictionary item referenced by the specified "key"
 * NB. deallocation will be recursive as described in xobject_free()
 *
 * Returns: 0 on success, -1 on failure
 */
int xdict_delete(struct xdict *dict, const struct xstring *key);

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
int xdict_insert(struct xdict *dict, struct xstring *key,
    struct xobject *value);

/*
 * Insert an item identified by "key" of value "value" into
 * dictionary "dict". If an item with the same key already exists it
 * is removed and deallocated first. Note that deallocation will occur
 * recursively as described in xobject_free()
 *
 * NB. successfully adding to the dictionary transfers ownership of both
 * the key and the value. The caller should not modify or deallocate the
 * object afterwards. 
 *
 * Returns: 0 on success, -1 on failure
 */
int xdict_replace(struct xdict *dict, struct xstring *key,
    struct xobject *value);

/*
 * Obtains an iterator over the object "obj". Iteration is
 * supported over arrays and dictionaries.
 *
 * Returns: pointer to iterator object or NULL on failure
 */
struct xiterator *xobject_getiter(struct xobject *obj);

/*
 * Free an iterator
 */
void xiterator_free(struct xiterator *iter);

/*
 * Return the next value in an iteration or NULL if no more
 * values are available.
 */
const struct xiteritem *xiterator_next(struct xiterator *iter);

#endif /* _XOBJECT_H */
