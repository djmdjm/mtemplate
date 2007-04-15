/*
 * Regress test for xobject library
 * Public domain -- Damien Miller <djm@mindrot.org> 2007-03-27
 */

/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "xobject.h"

#include "t_macros.h"

int
main(int argc, char **argv)
{
	struct xnone *xnone_obj, *xnone_obj2;
	struct xint *xint_obj;
	struct xstring *xstring_obj, *k;
	struct xarray *xarray_obj;
	struct xdict *xdict_obj;
	const struct xobject *o;
	struct xobject *o2;
	struct xiterator *it;
	char bin[10] = { 0x0, 0x1, 0xff, 'h', 'e', 'l', 'l', 'o', 0xf3, 0x0 };
	u_int seen, n;

	/* Turn on all malloc debugging on OpenBSD */
	setenv("MALLOC_OPTIONS", "AFGJPRX", 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("t0:");

	/* Case 1: smoke test xnone_new() */
	xnone_obj = xnone_new();
	assert(xnone_obj != NULL);
	printf(".");

	/* Case 2: smoke test xint_new */
	xint_obj = xint_new(42);
	assert(xint_obj != NULL);
	printf(".");

	/* Case 3: smoke test xstring_new */
	xstring_obj = xstring_new("hello, world");
	assert(xstring_obj != NULL);
	printf(".");

	/* Case 4: smoke test xarray_new */
	xarray_obj = xarray_new();
	assert(xarray_obj != NULL);
	printf(".");

	/* Case 4: smoke test xdict_new */
	xdict_obj = xdict_new();
	assert(xdict_obj != NULL);
	printf(".");

	/* Case 5: smoke test xobject_free() */
	xobject_free((struct xobject *)xnone_obj);
	xobject_free((struct xobject *)xint_obj);
	xobject_free((struct xobject *)xstring_obj);
	xobject_free((struct xobject *)xarray_obj);
	xobject_free((struct xobject *)xdict_obj);
	printf(".");

	/* Case 6: xnone_new() is singleton */
	xnone_obj = xnone_new();
	xnone_obj2 = xnone_new();
	assert(xnone_obj != NULL);
	assert(xnone_obj2 != NULL);
	assert(xnone_obj == xnone_obj);
	printf(".");

	/* Case 7: get value out of xint */
	xint_obj = xint_new(42);
	assert(xint_obj != NULL);
	assert(xint_value(xint_obj) == 42);
	printf(".");

	/* Case 8: get value out of xstring */
	xstring_obj = xstring_new("hello, world");
	assert(xstring_obj != NULL);
	assert(xstring_len(xstring_obj) == strlen("hello, world"));
	assert(strcmp(xstring_ptr(xstring_obj), "hello, world") == 0);
	printf(".");

	xarray_obj = xarray_new();
	assert(xarray_obj != NULL);

	/* Case 9: xarray_pull on empty array */
	assert(xarray_pull(xarray_obj) == NULL);
	printf(".");

	/* Case 10: xarray_pop on empty array */
	assert(xarray_pop(xarray_obj) == NULL);
	printf(".");

	/* Case 11: xarray_item on empty array */
	assert(xarray_item(xarray_obj, 0) == NULL);
	assert(xarray_item(xarray_obj, 1) == NULL);
	assert(xarray_item(xarray_obj, 4) == NULL);
	printf(".");

	/* Case 11: xarray_len on empty array */
	assert(xarray_len(xarray_obj) == 0);
	printf(".");

	/* Case 12: xarray_append */
	assert(xarray_append(xarray_obj, (struct xobject *)xnone_obj) == 0);
	assert(xarray_append(xarray_obj, (struct xobject *)xint_obj) == 0);
	assert(xarray_append(xarray_obj, (struct xobject *)xstring_obj) == 0);
	printf(".");

	/* Case 13: xarray_len */
	assert(xarray_len(xarray_obj) == 3);
	printf(".");

	/* Case 14: xarray_item */
	assert(xarray_item(xarray_obj, 0) == (struct xobject *)xnone_obj);
	assert(xarray_item(xarray_obj, 1) == (struct xobject *)xint_obj);
	assert(xarray_item(xarray_obj, 2) == (struct xobject *)xstring_obj);
	assert(xarray_item(xarray_obj, 3) == NULL);
	assert(xarray_item(xarray_obj, 4) == NULL);
	printf(".");

	/* Case 15: xarray_first */
	assert(xarray_first(xarray_obj) == (struct xobject *)xnone_obj);
	printf(".");

	/* Case 16: xarray_last */
	assert(xarray_last(xarray_obj) == (struct xobject *)xstring_obj);
	printf(".");

	/* Case 17: xarray_pull */
	assert((o = xarray_pull(xarray_obj)) == (struct xobject *)xnone_obj);
	assert((o = xarray_pull(xarray_obj)) == (struct xobject *)xint_obj);
	assert((o = xarray_pull(xarray_obj)) == 
	    (struct xobject *)xstring_obj);
	assert((o = xarray_pull(xarray_obj)) == NULL);
	printf(".");

	/* Case 18: xarray_len after emptying */
	assert(xarray_len(xarray_obj) == 0);
	printf(".");

	/* Case 19: xarray_first after emptying */
	assert(xarray_first(xarray_obj) == NULL);
	printf(".");

	/* Case 20: xarray_last after emptying */
	assert(xarray_last(xarray_obj) == NULL);
	printf(".");

	/* Case 21: xarray_item after emptying */
	assert(xarray_item(xarray_obj, 0) == NULL);
	assert(xarray_item(xarray_obj, 1) == NULL);
	assert(xarray_item(xarray_obj, 2) == NULL);
	assert(xarray_item(xarray_obj, 64) == NULL);
	assert(xarray_item(xarray_obj, 666) == NULL);
	printf(".");

	/* Case 22: xobject_free after emptying */
	xobject_free((struct xobject *)xarray_obj);
	printf(".");

	xarray_obj = xarray_new();
	assert(xarray_obj != NULL);

	/* Case 23: xarray_prepend */
	assert(xarray_prepend(xarray_obj, (struct xobject *)xnone_obj) == 0);
	assert(xarray_prepend(xarray_obj, (struct xobject *)xint_obj) == 0);
	assert(xarray_prepend(xarray_obj, 
	    (struct xobject *)xstring_obj) == 0);
	assert(xarray_item(xarray_obj, 0) == (struct xobject *)xstring_obj);
	assert(xarray_item(xarray_obj, 1) == (struct xobject *)xint_obj);
	assert(xarray_item(xarray_obj, 2) == (struct xobject *)xnone_obj);
	assert(xarray_item(xarray_obj, 3) == NULL);
	assert(xarray_item(xarray_obj, 4) == NULL);
	printf(".");

	/* Case 24: xobject_free while full */
	xobject_free((struct xobject *)xarray_obj);
	printf(".");

	/* Case 25: mixed xarray_append and xarray_prepend */
	xarray_obj = xarray_new();
	assert(xarray_prepend(xarray_obj,
	    (struct xobject *)xnone_new()) == 0);
	/* { None } */
	assert(xarray_prepend(xarray_obj,
	    (struct xobject *)xint_new(1)) == 0);
	/* { 1, None } */
	assert(xarray_append(xarray_obj,
	    (struct xobject *)xint_new(2)) == 0);
	/* { 1, None, 2 } */
	assert(xarray_prepend(xarray_obj,
	    (struct xobject *)xstring_new("abc")) == 0);
	/* { "abc", 1, None, 2 } */
	assert(xarray_append(xarray_obj,
	    (struct xobject *)xstring_new("def")) == 0);
	/* { "abc", 1, None, 2, "def" } */
	assert(xarray_append(xarray_obj,
	    (struct xobject *)xstring_new("hij")) == 0);
	/* { "abc", 1, None, 2, "def", "hij" } */
	assert(xarray_prepend(xarray_obj,
	    (struct xobject *)xstring_new("klm")) == 0);
	/* { "klm", "abc", 1, None, 2, "def", "hij" } */
	assert(xarray_prepend(xarray_obj,
	    (struct xobject *)xint_new(3)) == 0);
	/* { 3, "klm", "abc", 1, None, 2, "def", "hij" } */
	assert(xarray_prepend(xarray_obj,
	    (struct xobject *)xint_new(4)) == 0);
	/* { 4, 3, "klm", "abc", 1, None, 2, "def", "hij" } */
	assert(xarray_append(xarray_obj,
	    (struct xobject *)xnone_new()) == 0);
	/* { 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None } */
	assert(xarray_prepend(xarray_obj,
	    (struct xobject *)xstring_new("pqr")) == 0);
	/* { "pqr", 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None } */

	X_STRING_AT_ARRAY(xarray_obj, 0, "pqr");
	X_INT_AT_ARRAY(xarray_obj, 1, 4);
	X_INT_AT_ARRAY(xarray_obj, 2, 3);
	X_STRING_AT_ARRAY(xarray_obj, 3, "klm");
	X_STRING_AT_ARRAY(xarray_obj, 4, "abc");
	X_INT_AT_ARRAY(xarray_obj, 5, 1);
	X_NONE_AT_ARRAY(xarray_obj, 6);
	X_INT_AT_ARRAY(xarray_obj, 7, 2);
	X_STRING_AT_ARRAY(xarray_obj, 8, "def");
	X_STRING_AT_ARRAY(xarray_obj, 9, "hij");
	X_NONE_AT_ARRAY(xarray_obj, 10);
	assert((o = xarray_item(xarray_obj, 11)) == NULL);
	assert((o = xarray_item(xarray_obj, 12)) == NULL);
	assert((o = xarray_item(xarray_obj, 666)) == NULL);
	printf(".");

	/* Case 26: xarray_pop */
	/* Note - order is reversed */
	X_NONE_POP_ARRAY(xarray_obj);
	X_STRING_POP_ARRAY(xarray_obj, "hij");
	X_STRING_POP_ARRAY(xarray_obj, "def");
	X_INT_POP_ARRAY(xarray_obj, 2);
	X_NONE_POP_ARRAY(xarray_obj);
	X_INT_POP_ARRAY(xarray_obj, 1);
	X_STRING_POP_ARRAY(xarray_obj, "abc");
	X_STRING_POP_ARRAY(xarray_obj, "klm");
	X_INT_POP_ARRAY(xarray_obj, 3);
	X_INT_POP_ARRAY(xarray_obj, 4);
	X_STRING_POP_ARRAY(xarray_obj, "pqr");
	assert((o = xarray_pop(xarray_obj)) == NULL);
	assert((o = xarray_pop(xarray_obj)) == NULL);
	printf(".");

	xobject_free((struct xobject *)xarray_obj);

	/* Case 27: store into dict */
	xdict_obj = xdict_new();
	assert(xdict_insert(xdict_obj, xstring_new("a"),
	    (struct xobject *)xnone_new()) == 0);
	assert(xdict_insert(xdict_obj, xstring_new("b"),
	    (struct xobject *)xstring_new("abc")) == 0);
	assert(xdict_insert(xdict_obj, xstring_new("c"),
	    (struct xobject *)xstring_new("def")) == 0);
	assert(xdict_insert(xdict_obj, xstring_new("d"),
	    (struct xobject *)xstring_new("ghi")) == 0);
	assert(xdict_insert(xdict_obj, xstring_new("e"),
	    (struct xobject *)xint_new(-64)) == 0);
	assert(xdict_insert(xdict_obj, xstring_new("f"),
	    (struct xobject *)xint_new(42)) == 0);
	assert(xdict_insert(xdict_obj, (k = xstring_new("a")),
	    (o2 = (struct xobject *)xint_new(666))) == -1);
	xobject_free((struct xobject *)k);
	xobject_free(o2);
	printf(".");

	/* Case 28: xdict_len after insert */
	assert(xdict_len(xdict_obj) == 6);
	printf(".");

	/* Case 29: retrieve from dict */
	X_NONE_AT_DICT(xdict_obj, "a");
	X_STRING_AT_DICT(xdict_obj, "b", "abc");
	X_STRING_AT_DICT(xdict_obj, "c", "def");
	X_STRING_AT_DICT(xdict_obj, "d", "ghi");
	X_INT_AT_DICT(xdict_obj, "e", -64);
	X_INT_AT_DICT(xdict_obj, "f", 42);
	X_NULL_AT_DICT(xdict_obj, "x");
	printf(".");

	/* Case 30: replace in dict */
	assert(xdict_replace(xdict_obj, xstring_new("a"),
	    (struct xobject *)xint_new(666)) == 0);
	assert(xdict_replace(xdict_obj, xstring_new("x"),
	    (struct xobject *)xnone_new()) == 0);
	X_INT_AT_DICT(xdict_obj, "a", 666);
	X_NONE_AT_DICT(xdict_obj, "x");
	printf(".");

	/* Case 31: xdict_len after replace */
	assert(xdict_len(xdict_obj) == 7);
	printf(".");

	/* Case 32: remove from dict */
	assert((o2 = xdict_remove(xdict_obj,
	    (k = xstring_new("c")))) != NULL);
	xobject_free((struct xobject *)k);
	assert(xobject_type(o2) == TYPE_XSTRING);
	assert(strcmp(xstring_ptr((struct xstring *)o2), "def") == 0);
	xobject_free(o2);
	assert((o2 = xdict_remove(xdict_obj,
	    (k = xstring_new("e")))) != NULL);
	xobject_free((struct xobject *)k);
	assert(xint_value((struct xint *)o2) == -64);
	xobject_free(o2);
	assert((o2 = xdict_remove(xdict_obj, 
	    (k = xstring_new("z")))) == NULL);
	xobject_free((struct xobject *)k);
	X_NULL_AT_DICT(xdict_obj, "c");
	X_NULL_AT_DICT(xdict_obj, "e");
	X_NULL_AT_DICT(xdict_obj, "z");
	printf(".");

	/* Case 33: xdict_len after remove */
	assert(xdict_len(xdict_obj) == 5);
	printf(".");

	/* Case 34: free populated dict */
	xobject_free((struct xobject *)xdict_obj);
	printf(".");

	/* Case 35: xobject_to_string */
	xstring_obj = xstring_new("hello, world");
	xint_obj = xint_new(42);
	xnone_obj = xnone_new();
	xarray_obj = xarray_new();
	xdict_obj = xdict_new();
	T_XOTS(xstring_obj, "hello, world");
	T_XOTS(xint_obj, "42");
	T_XOTS(xnone_obj, "None");
	T_XOTS_PRE(xarray_obj, "xarray(");
	T_XOTS_PRE(xdict_obj, "xdict(");
	xobject_free((struct xobject *)xstring_obj);
	xobject_free((struct xobject *)xint_obj);
	xobject_free((struct xobject *)xnone_obj);
	xobject_free((struct xobject *)xarray_obj);
	xobject_free((struct xobject *)xdict_obj);
	printf(".");

	/* Case 36: xobject_to_string with non-ASCII chars */
	xstring_obj = xstring_new2(bin, sizeof(bin));
	T_XOTS(xstring_obj, "\\000\\001\\377hello\\363\\000");
	xobject_free((struct xobject *)xstring_obj);
	printf(".");

	/* Case 37: array iterator */
	X_ARRAY_FILL(xarray_obj);
	/* { "pqr", 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None } */

	assert((it = xobject_getiter((struct xobject *)xarray_obj)) != NULL);
	X_STRING_ARRAY_ITER(it, 0, "pqr");
	X_INT_ARRAY_ITER(it, 1, 4);
	X_INT_ARRAY_ITER(it, 2, 3);
	X_STRING_ARRAY_ITER(it, 3, "klm");
	X_STRING_ARRAY_ITER(it, 4, "abc");
	X_INT_ARRAY_ITER(it, 5, 1);
	X_NONE_ARRAY_ITER(it, 6);
	X_INT_ARRAY_ITER(it, 7, 2);
	X_STRING_ARRAY_ITER(it, 8, "def");
	X_STRING_ARRAY_ITER(it, 9, "hij");
	X_NONE_ARRAY_ITER(it, 10);
	X_NULL_ARRAY_ITER(it);
	X_NULL_ARRAY_ITER(it);
	printf(".");

	/* Case 38: free iterator after array traversal */
	xiterator_free(it);
	printf(".");

	/* Case 39: free iterator midway through array traversal */
	assert((it = xobject_getiter((struct xobject *)xarray_obj)) != NULL);
	X_STRING_ARRAY_ITER(it, 0, "pqr");
	X_INT_ARRAY_ITER(it, 1, 4);
	X_INT_ARRAY_ITER(it, 2, 3);
	X_STRING_ARRAY_ITER(it, 3, "klm");
	X_STRING_ARRAY_ITER(it, 4, "abc");
	xiterator_free(it);
	printf(".");

	xobject_free((struct xobject *)xarray_obj);

	/* Case 40: dict iterator */
	X_DICT_FILL(xdict_obj);
	/* { "a" : None, "b" : "abc", c : "def", d : -64, e : 42 } */
	/*
	 * This is a little more painful, as dict iteration doesn't
	 * guarantee an order
	 */
	seen = 31;
	assert((it = xobject_getiter((struct xobject *)xdict_obj)) != NULL);
	for (n = 0; n < 5; n++) {
		const struct xiteritem *ii;
		int kn;
		assert((ii = xiterator_next(it)) != NULL);
		assert(xobject_type(ii->key) == TYPE_XSTRING);
		assert(xstring_len((struct xstring *)ii->key) == 1);
		kn = xstring_ptr((struct xstring *)ii->key)[0];
		assert((seen & (1 << (kn - 'a'))) != 0);
		seen &= ~(1 << (kn - 'a'));
		switch (kn) {
		case 'a':
			assert(xobject_type(ii->value) == TYPE_XNONE);
			break;
		case 'b':
			assert(xobject_type(ii->value) == TYPE_XSTRING);
			assert(strcmp(xstring_ptr((struct xstring *)ii->value),
			    "abc") == 0);
			break;
		case 'c':
			assert(xobject_type(ii->value) == TYPE_XSTRING);
			assert(strcmp(xstring_ptr((struct xstring *)ii->value),
			    "def") == 0);
			break;
		case 'd':
			assert(xobject_type(ii->value) == TYPE_XINT);
			assert(xint_value((struct xint *)ii->value) == -64);
			break;
		case 'e':
			assert(xobject_type(ii->value) == TYPE_XINT);
			assert(xint_value((struct xint *)ii->value) == 42);
			break;
		default:
			assert(0);
		}
	}
	assert(seen == 0);

	/* Case 41: free iterator after dict traversal */
	xiterator_free(it);
	printf(".");

	xobject_free((struct xobject *)xdict_obj);

	/* Case 42: xarray_set and implicit filling */
	assert((xarray_obj = xarray_new()) != NULL);
	assert((xstring_obj = xstring_new2("happy", 3)) != NULL);
	assert(xarray_set(xarray_obj, 5, (struct xobject *)xstring_obj) != -1);
	assert((xstring_obj = xstring_new2("joyjoy", 2)) != NULL);
	assert(xarray_set(xarray_obj, 3, (struct xobject *)xstring_obj) != -1);
	assert((xstring_obj = xstring_new2("merry!", 5)) != NULL);
	assert(xarray_set(xarray_obj, 3, (struct xobject *)xstring_obj) != -1);
	assert(xarray_len(xarray_obj) == 6);
	X_NONE_AT_ARRAY(xarray_obj, 0);
	X_NONE_AT_ARRAY(xarray_obj, 1);
	X_NONE_AT_ARRAY(xarray_obj, 2);
	X_STRING_AT_ARRAY(xarray_obj, 3, "merry");
	X_NONE_AT_ARRAY(xarray_obj, 4);
	X_STRING_AT_ARRAY(xarray_obj, 5, "hap");
	xobject_free((struct xobject *)xarray_obj);
	printf(".");

	/* XXX check that functions do not accept inappropriate objects */
	/* XXX concurrent iterations */
	/* XXX xdict_*_s function */

	printf("\n");
	return 0;
}
