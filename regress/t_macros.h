/*
 * Regress helper macros for mobject library
 * Public domain -- Damien Miller <djm@mindrot.org> 2007-03-27
 */

/* $Id$ */

#ifndef _T_MACROS_H
#define _T_MACROS_H

/* Expect object at position in array */
#define X_STRING_AT_ARRAY(a, loc, expect) do { \
		const struct mobject *xo; \
		assert((xo = marray_item(a, loc)) != NULL); \
		assert(mobject_type(xo) == TYPE_MSTRING); \
		assert(strcmp((const char *)mstring_ptr(xo), \
		    expect) == 0); \
	} while (0)
#define X_NONE_AT_ARRAY(a, loc) do { \
		const struct mobject *xo; \
		assert((xo = marray_item(a, loc)) != NULL); \
		assert(mobject_type(xo) == TYPE_MNONE); \
	} while (0)
#define X_INT_AT_ARRAY(a, loc, expect) do { \
		const struct mobject *xo; \
		assert((xo = marray_item(a, loc)) != NULL); \
		assert(mobject_type(xo) == TYPE_MINT); \
		assert(mint_value(xo) == expect); \
	} while (0)

/* Expect object at popped off array */
#define X_STRING_POP_ARRAY(a, expect) do { \
		struct mobject *xo; \
		assert((xo = marray_pop(a)) != NULL); \
		assert(mobject_type(xo) == TYPE_MSTRING); \
		assert(strcmp((const char *)mstring_ptr(xo), \
		    expect) == 0); \
		mobject_free(xo); \
	} while (0)
#define X_NONE_POP_ARRAY(a) do { \
		struct mobject *xo; \
		assert((xo = marray_pop(a)) != NULL); \
		assert(mobject_type(xo) == TYPE_MNONE); \
		mobject_free(xo); \
	} while (0)
#define X_INT_POP_ARRAY(a, expect) do { \
		struct mobject *xo; \
		assert((xo = marray_pop(a)) != NULL); \
		assert(mobject_type(xo) == TYPE_MINT); \
		assert(mint_value(xo) == expect); \
		mobject_free(xo); \
	} while (0)

/* Expect object in dict */
#define X_STRING_AT_DICT(d, loc, expect) do { \
		const struct mobject *xo; \
		struct mobject *xs; \
		assert((xs = mstring_new(loc)) != NULL); \
		assert((xo = mdict_item(d, xs)) != NULL); \
		assert(mobject_type(xo) == TYPE_MSTRING); \
		assert(strcmp((const char *)mstring_ptr(xo), \
		    expect) == 0); \
		mobject_free(xs); \
	} while (0)
#define X_NONE_AT_DICT(d, loc) do { \
		const struct mobject *xo; \
		struct mobject *xs; \
		assert((xs = mstring_new(loc)) != NULL); \
		assert((xo = mdict_item(d, xs)) != NULL); \
		assert(mobject_type(xo) == TYPE_MNONE); \
		mobject_free(xs); \
	} while (0)
#define X_INT_AT_DICT(d, loc, expect) do { \
		const struct mobject *xo; \
		struct mobject *xs; \
		assert((xs = mstring_new(loc)) != NULL); \
		assert((xo = mdict_item(d, xs)) != NULL); \
		assert(mobject_type(xo) == TYPE_MINT); \
		assert(mint_value(xo) == expect); \
		mobject_free(xs); \
	} while (0)
#define X_NULL_AT_DICT(d, loc) do { \
		const struct mobject *xo; \
		struct mobject *xs; \
		assert((xs = mstring_new(loc)) != NULL); \
		assert((xo = mdict_item(d, xs)) == NULL); \
		mobject_free(xs); \
	} while (0)

/* Expect object to string translation */
#define T_XOTS(o, s) do { \
		char buf1[8192], buf2[64]; \
		size_t l1, l2; \
		memset(buf2, '*', sizeof(buf2)); \
		l1 = mobject_to_string(o, \
		    buf1, sizeof(buf1)); \
		l2 = mobject_to_string(o, buf2, 5); \
		assert(l1 = strlen(s)); \
		assert(l2 = strlen(s)); \
		assert(strcmp(buf1, s) == 0); \
		assert(strncmp(buf2, s, 4) == 0); \
		assert(buf2[5] == '*'); \
	} while (0)
#define T_XOTS_PRE(o, s) do { \
		char buf1[8192], buf2[64]; \
		size_t l1, l2; \
		memset(buf2, '*', sizeof(buf2)); \
		l1 = mobject_to_string(o, \
		    buf1, sizeof(buf1)); \
		l2 = mobject_to_string(o, buf2, 5); \
		assert(l1 = strlen(s)); \
		assert(l2 = strlen(s)); \
		assert(strncmp(buf1, s, strlen(s)) == 0); \
		assert(strncmp(buf2, s, MIN(strlen(s), 4)) == 0); \
		assert(buf2[5] == '*'); \
	} while (0)

/* Expect object at next iteration point */
#define X_STRING_ARRAY_ITER(i, k, v) do { \
		const struct miteritem *ii; \
		assert((ii = miterator_next(i)) != NULL); \
		assert(mobject_type(ii->key) == TYPE_MINT); \
		assert(mobject_type(ii->value) == TYPE_MSTRING); \
		assert(mint_value(ii->key) == k); \
		assert(strcmp((char *)mstring_ptr(ii->value), \
		    v) == 0); \
	} while (0)
#define X_NONE_ARRAY_ITER(i, k) do { \
		const struct miteritem *ii; \
		assert((ii = miterator_next(i)) != NULL); \
		assert(mobject_type(ii->key) == TYPE_MINT); \
		assert(mobject_type(ii->value) == TYPE_MNONE); \
		assert(mint_value(ii->key) == k); \
	} while (0)
#define X_NULL_ARRAY_ITER(i) do { \
		const struct miteritem *ii; \
		assert((ii = miterator_next(i)) == NULL); \
	} while (0)
#define X_INT_ARRAY_ITER(i, k, v) do { \
		const struct miteritem *ii; \
		assert((ii = miterator_next(i)) != NULL); \
		assert(mobject_type(ii->key) == TYPE_MINT); \
		assert(mobject_type(ii->value) == TYPE_MINT); \
		assert(mint_value(ii->key) == k); \
		assert(mint_value(ii->value) == v); \
	} while (0)

/* Fill: [ "pqr", 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None ] */
#define X_ARRAY_FILL(x) do { \
		x = marray_new(); \
		marray_prepend(x, mnone_new()); \
		marray_prepend(x, mint_new(1)); \
		marray_append(x, mint_new(2)); \
		marray_prepend(x, mstring_new("abc")); \
		marray_append(x, mstring_new("def")); \
		marray_append(x, mstring_new("hij")); \
		marray_prepend(x, mstring_new("klm")); \
		marray_prepend(x, mint_new(3)); \
		marray_prepend(x, mint_new(4)); \
		marray_append(x, mnone_new()); \
		marray_prepend(x, mstring_new("pqr")); \
	} while (0)

/* Fill: { "a" : None, "b" : "abc", c : "def", d : -64, e : 42 } */
#define X_DICT_FILL(x) do { \
		x = mdict_new(); \
		mdict_insert(x, mstring_new("a"), mnone_new()); \
		mdict_insert(x, mstring_new("b"), mstring_new("abc")); \
		mdict_insert(x, mstring_new("c"), mstring_new("def")); \
		mdict_insert(x, mstring_new("d"), mint_new(-64)); \
		mdict_insert(x, mstring_new("e"), mint_new(42)); \
	} while (0)

#endif /* _T_MACROS_H */
