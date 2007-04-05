/*
 * Regress helper macros for xobject library
 * Public domain -- Damien Miller <djm@mindrot.org> 2007-03-27
 */

/* $Id$ */

#ifndef _T_MACROS_H
#define _T_MACROS_H

/* Expect object at position in array */
#define X_STRING_AT_ARRAY(a, loc, expect) do { \
		const struct xobject *xo; \
		assert((xo = xarray_item(a, loc)) != NULL); \
		assert(xobject_type(xo) == TYPE_XSTRING); \
		assert(strcmp(xstring_ptr((struct xstring *)xo), \
		    expect) == 0); \
	} while (0)
#define X_NONE_AT_ARRAY(a, loc) do { \
		const struct xobject *xo; \
		assert((xo = xarray_item(a, loc)) != NULL); \
		assert(xobject_type(xo) == TYPE_XNONE); \
	} while (0)
#define X_INT_AT_ARRAY(a, loc, expect) do { \
		const struct xobject *xo; \
		assert((xo = xarray_item(a, loc)) != NULL); \
		assert(xobject_type(xo) == TYPE_XINT); \
		assert(xint_value((struct xint *)xo) == expect); \
	} while (0)

/* Expect object at popped off array */
#define X_STRING_POP_ARRAY(a, expect) do { \
		struct xobject *xo; \
		assert((xo = xarray_pop(a)) != NULL); \
		assert(xobject_type(xo) == TYPE_XSTRING); \
		assert(strcmp(xstring_ptr((struct xstring *)xo), \
		    expect) == 0); \
		xobject_free(xo); \
	} while (0)
#define X_NONE_POP_ARRAY(a) do { \
		struct xobject *xo; \
		assert((xo = xarray_pop(a)) != NULL); \
		assert(xobject_type(xo) == TYPE_XNONE); \
		xobject_free(xo); \
	} while (0)
#define X_INT_POP_ARRAY(a, expect) do { \
		struct xobject *xo; \
		assert((xo = xarray_pop(a)) != NULL); \
		assert(xobject_type(xo) == TYPE_XINT); \
		assert(xint_value((struct xint *)xo) == expect); \
		xobject_free(xo); \
	} while (0)

/* Expect object in dict */
#define X_STRING_AT_DICT(d, loc, expect) do { \
		const struct xobject *xo; \
		struct xstring *xs; \
		assert((xs = xstring_new(loc)) != NULL); \
		assert((xo = xdict_item(d, xs)) != NULL); \
		assert(xobject_type(xo) == TYPE_XSTRING); \
		assert(strcmp(xstring_ptr((struct xstring *)xo), \
		    expect) == 0); \
		xobject_free((struct xobject *)xs); \
	} while (0)
#define X_NONE_AT_DICT(d, loc) do { \
		const struct xobject *xo; \
		struct xstring *xs; \
		assert((xs = xstring_new(loc)) != NULL); \
		assert((xo = xdict_item(d, xs)) != NULL); \
		assert(xobject_type(xo) == TYPE_XNONE); \
		xobject_free((struct xobject *)xs); \
	} while (0)
#define X_INT_AT_DICT(d, loc, expect) do { \
		const struct xobject *xo; \
		struct xstring *xs; \
		assert((xs = xstring_new(loc)) != NULL); \
		assert((xo = xdict_item(d, xs)) != NULL); \
		assert(xobject_type(xo) == TYPE_XINT); \
		assert(xint_value((struct xint *)xo) == expect); \
		xobject_free((struct xobject *)xs); \
	} while (0)
#define X_NULL_AT_DICT(d, loc) do { \
		const struct xobject *xo; \
		struct xstring *xs; \
		assert((xs = xstring_new(loc)) != NULL); \
		assert((xo = xdict_item(d, xs)) == NULL); \
		xobject_free((struct xobject *)xs); \
	} while (0)

/* Expect object to string translation */
#define T_XOTS(o, s) do { \
		char buf1[8192], buf2[64]; \
		size_t l1, l2; \
		memset(buf2, '*', sizeof(buf2)); \
		l1 = xobject_to_string((struct xobject *)o, \
		    buf1, sizeof(buf1)); \
		l2 = xobject_to_string((struct xobject *)o, buf2, 5); \
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
		l1 = xobject_to_string((struct xobject *)o, \
		    buf1, sizeof(buf1)); \
		l2 = xobject_to_string((struct xobject *)o, buf2, 5); \
		assert(l1 = strlen(s)); \
		assert(l2 = strlen(s)); \
		assert(strncmp(buf1, s, strlen(s)) == 0); \
		assert(strncmp(buf2, s, MIN(strlen(s), 4)) == 0); \
		assert(buf2[5] == '*'); \
	} while (0)

/* Expect object at next iteration point */
#define X_STRING_ARRAY_ITER(i, k, v) do { \
		const struct xiteritem *ii; \
		assert((ii = xiterator_next(i)) != NULL); \
		assert(xobject_type(ii->key) == TYPE_XINT); \
		assert(xobject_type(ii->value) == TYPE_XSTRING); \
		assert(xint_value((const struct xint *)ii->key) == k); \
		assert(strcmp(xstring_ptr((const struct xstring *)ii->value), \
		    v) == 0); \
	} while (0)
#define X_NONE_ARRAY_ITER(i, k) do { \
		const struct xiteritem *ii; \
		assert((ii = xiterator_next(i)) != NULL); \
		assert(xobject_type(ii->key) == TYPE_XINT); \
		assert(xobject_type(ii->value) == TYPE_XNONE); \
		assert(xint_value((const struct xint *)ii->key) == k); \
	} while (0)
#define X_NULL_ARRAY_ITER(i) do { \
		const struct xiteritem *ii; \
		assert((ii = xiterator_next(i)) == NULL); \
	} while (0)
#define X_INT_ARRAY_ITER(i, k, v) do { \
		const struct xiteritem *ii; \
		assert((ii = xiterator_next(i)) != NULL); \
		assert(xobject_type(ii->key) == TYPE_XINT); \
		assert(xobject_type(ii->value) == TYPE_XINT); \
		assert(xint_value((const struct xint *)ii->key) == k); \
		assert(xint_value((const struct xint *)ii->value) == v); \
	} while (0)

/* Fill: [ "pqr", 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None ] */
#define X_ARRAY_FILL(x) do { \
		x = xarray_new(); \
		xarray_prepend(x, (struct xobject *)xnone_new()); \
		xarray_prepend(x, (struct xobject *)xint_new(1)); \
		xarray_append(x, (struct xobject *)xint_new(2)); \
		xarray_prepend(x, (struct xobject *)xstring_new("abc")); \
		xarray_append(x, (struct xobject *)xstring_new("def")); \
		xarray_append(x, (struct xobject *)xstring_new("hij")); \
		xarray_prepend(x, (struct xobject *)xstring_new("klm")); \
		xarray_prepend(x, (struct xobject *)xint_new(3)); \
		xarray_prepend(x, (struct xobject *)xint_new(4)); \
		xarray_append(x, (struct xobject *)xnone_new()); \
		xarray_prepend(x, (struct xobject *)xstring_new("pqr")); \
	} while (0)

/* Fill: { "a" : None, "b" : "abc", c : "def", d : -64, e : 42 } */
#define X_DICT_FILL(x) do { \
		x = xdict_new(); \
		xdict_insert(x, xstring_new("a"), \
		    (struct xobject *)xnone_new()); \
		xdict_insert(x, xstring_new("b"), \
		    (struct xobject *)xstring_new("abc")); \
		xdict_insert(x, xstring_new("c"), \
		    (struct xobject *)xstring_new("def")); \
		xdict_insert(x, xstring_new("d"), \
		    (struct xobject *)xint_new(-64)); \
		xdict_insert(x, xstring_new("e"), \
		    (struct xobject *)xint_new(42)); \
	} while (0)

/*
{char b[64]; \
 xobject_to_string((const struct xobject *)ii->key, b, 64); printf("XXX key '%s'\n", b); \
 xobject_to_string((const struct xobject *)ii->value, b, 64); printf("XXX value '%s'\n", b); \
} \
*/

#endif /* _T_MACROS_H */