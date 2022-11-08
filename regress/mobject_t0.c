/*
 * Regress test for mobject library
 * Public domain -- Damien Miller <djm@mindrot.org> 2007-03-27
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mobject.h"

#include "t_macros.h"

int
main(int argc, char **argv)
{
	struct mobject *mnone_obj, *mnone_obj2;
	struct mobject *mint_obj;
	struct mobject *mstring_obj, *k;
	struct mobject *marray_obj;
	struct mobject *mdict_obj;
	const struct mobject *o;
	struct mobject *o2;
	struct miterator *it;
	u_int8_t bin[10] = {
		0x0, 0x1, 0xff, 'h', 'e', 'l', 'l', 'o', 0xf3, 0x0
	};
	u_int seen, n;

	/* Turn on all malloc debugging on OpenBSD */
	setenv("MALLOC_OPTIONS", "AFGJPRX", 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("mobject_t0:");

	/* Case 1: smoke test mnone_new() */
	mnone_obj = mnone_new();
	assert(mnone_obj != NULL);
	printf(".");

	/* Case 2: smoke test mint_new */
	mint_obj = mint_new(42);
	assert(mint_obj != NULL);
	printf(".");

	/* Case 3: smoke test mstring_new */
	mstring_obj = mstring_new("hello, world");
	assert(mstring_obj != NULL);
	printf(".");

	/* Case 4: smoke test marray_new */
	marray_obj = marray_new();
	assert(marray_obj != NULL);
	printf(".");

	/* Case 4: smoke test mdict_new */
	mdict_obj = mdict_new();
	assert(mdict_obj != NULL);
	printf(".");

	/* Case 5: smoke test mobject_free() */
	mobject_free(mnone_obj);
	mobject_free(mint_obj);
	mobject_free(mstring_obj);
	mobject_free(marray_obj);
	mobject_free(mdict_obj);
	printf(".");

	/* Case 6: mnone_new() is singleton */
	mnone_obj = mnone_new();
	mnone_obj2 = mnone_new();
	assert(mnone_obj != NULL);
	assert(mnone_obj2 != NULL);
	assert(mnone_obj == mnone_obj);
	printf(".");

	/* Case 7: get value out of mint */
	mint_obj = mint_new(42);
	assert(mint_obj != NULL);
	assert(mint_value(mint_obj) == 42);
	printf(".");

	/* Case 8: get value out of mstring */
	mstring_obj = mstring_new("hello, world");
	assert(mstring_obj != NULL);
	assert(mstring_len(mstring_obj) == strlen("hello, world"));
	assert(strcmp((char *)mstring_ptr(mstring_obj), "hello, world") == 0);
	printf(".");

	marray_obj = marray_new();
	assert(marray_obj != NULL);

	/* Case 9: marray_pull on empty array */
	assert(marray_pull(marray_obj) == NULL);
	printf(".");

	/* Case 10: marray_pop on empty array */
	assert(marray_pop(marray_obj) == NULL);
	printf(".");

	/* Case 11: marray_item on empty array */
	assert(marray_item(marray_obj, 0) == NULL);
	assert(marray_item(marray_obj, 1) == NULL);
	assert(marray_item(marray_obj, 4) == NULL);
	printf(".");

	/* Case 11: marray_len on empty array */
	assert(marray_len(marray_obj) == 0);
	printf(".");

	/* Case 12: marray_append */
	assert(marray_append(marray_obj, mnone_obj) == 0);
	assert(marray_append(marray_obj, mint_obj) == 0);
	assert(marray_append(marray_obj, mstring_obj) == 0);
	printf(".");

	/* Case 13: marray_len */
	assert(marray_len(marray_obj) == 3);
	printf(".");

	/* Case 14: marray_item */
	assert(marray_item(marray_obj, 0) == mnone_obj);
	assert(marray_item(marray_obj, 1) == mint_obj);
	assert(marray_item(marray_obj, 2) == mstring_obj);
	assert(marray_item(marray_obj, 3) == NULL);
	assert(marray_item(marray_obj, 4) == NULL);
	printf(".");

	/* Case 15: marray_first */
	assert(marray_first(marray_obj) == mnone_obj);
	printf(".");

	/* Case 16: marray_last */
	assert(marray_last(marray_obj) == mstring_obj);
	printf(".");

	/* Case 17: marray_pull */
	assert((o = marray_pull(marray_obj)) == mnone_obj);
	assert((o = marray_pull(marray_obj)) == mint_obj);
	assert((o = marray_pull(marray_obj)) == mstring_obj);
	assert((o = marray_pull(marray_obj)) == NULL);
	printf(".");

	/* Case 18: marray_len after emptying */
	assert(marray_len(marray_obj) == 0);
	printf(".");

	/* Case 19: marray_first after emptying */
	assert(marray_first(marray_obj) == NULL);
	printf(".");

	/* Case 20: marray_last after emptying */
	assert(marray_last(marray_obj) == NULL);
	printf(".");

	/* Case 21: marray_item after emptying */
	assert(marray_item(marray_obj, 0) == NULL);
	assert(marray_item(marray_obj, 1) == NULL);
	assert(marray_item(marray_obj, 2) == NULL);
	assert(marray_item(marray_obj, 64) == NULL);
	assert(marray_item(marray_obj, 666) == NULL);
	printf(".");

	/* Case 22: mobject_free after emptying */
	mobject_free(marray_obj);
	printf(".");

	marray_obj = marray_new();
	assert(marray_obj != NULL);

	/* Case 23: marray_prepend */
	assert(marray_prepend(marray_obj, mnone_obj) == 0);
	assert(marray_prepend(marray_obj, mint_obj) == 0);
	assert(marray_prepend(marray_obj, mstring_obj) == 0);
	assert(marray_item(marray_obj, 0) == mstring_obj);
	assert(marray_item(marray_obj, 1) == mint_obj);
	assert(marray_item(marray_obj, 2) == mnone_obj);
	assert(marray_item(marray_obj, 3) == NULL);
	assert(marray_item(marray_obj, 4) == NULL);
	printf(".");

	/* Case 24: mobject_free while full */
	mobject_free(marray_obj);
	printf(".");

	/* Case 25: mixed marray_append and marray_prepend */
	marray_obj = marray_new();
	assert(marray_prepend(marray_obj, mnone_new()) == 0);
	/* { None } */
	assert(marray_prepend(marray_obj, mint_new(1)) == 0);
	/* { 1, None } */
	assert(marray_append(marray_obj, mint_new(2)) == 0);
	/* { 1, None, 2 } */
	assert(marray_prepend(marray_obj, mstring_new("abc")) == 0);
	/* { "abc", 1, None, 2 } */
	assert(marray_append(marray_obj, mstring_new("def")) == 0);
	/* { "abc", 1, None, 2, "def" } */
	assert(marray_append(marray_obj, mstring_new("hij")) == 0);
	/* { "abc", 1, None, 2, "def", "hij" } */
	assert(marray_prepend(marray_obj, mstring_new("klm")) == 0);
	/* { "klm", "abc", 1, None, 2, "def", "hij" } */
	assert(marray_prepend(marray_obj, mint_new(3)) == 0);
	/* { 3, "klm", "abc", 1, None, 2, "def", "hij" } */
	assert(marray_prepend(marray_obj, mint_new(4)) == 0);
	/* { 4, 3, "klm", "abc", 1, None, 2, "def", "hij" } */
	assert(marray_append(marray_obj, mnone_new()) == 0);
	/* { 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None } */
	assert(marray_prepend(marray_obj, mstring_new("pqr")) == 0);
	/* { "pqr", 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None } */

	X_STRING_AT_ARRAY(marray_obj, 0, "pqr");
	X_INT_AT_ARRAY(marray_obj, 1, 4);
	X_INT_AT_ARRAY(marray_obj, 2, 3);
	X_STRING_AT_ARRAY(marray_obj, 3, "klm");
	X_STRING_AT_ARRAY(marray_obj, 4, "abc");
	X_INT_AT_ARRAY(marray_obj, 5, 1);
	X_NONE_AT_ARRAY(marray_obj, 6);
	X_INT_AT_ARRAY(marray_obj, 7, 2);
	X_STRING_AT_ARRAY(marray_obj, 8, "def");
	X_STRING_AT_ARRAY(marray_obj, 9, "hij");
	X_NONE_AT_ARRAY(marray_obj, 10);
	assert((o = marray_item(marray_obj, 11)) == NULL);
	assert((o = marray_item(marray_obj, 12)) == NULL);
	assert((o = marray_item(marray_obj, 666)) == NULL);
	printf(".");

	/* Case 26: marray_pop */
	/* Note - order is reversed */
	X_NONE_POP_ARRAY(marray_obj);
	X_STRING_POP_ARRAY(marray_obj, "hij");
	X_STRING_POP_ARRAY(marray_obj, "def");
	X_INT_POP_ARRAY(marray_obj, 2);
	X_NONE_POP_ARRAY(marray_obj);
	X_INT_POP_ARRAY(marray_obj, 1);
	X_STRING_POP_ARRAY(marray_obj, "abc");
	X_STRING_POP_ARRAY(marray_obj, "klm");
	X_INT_POP_ARRAY(marray_obj, 3);
	X_INT_POP_ARRAY(marray_obj, 4);
	X_STRING_POP_ARRAY(marray_obj, "pqr");
	assert((o = marray_pop(marray_obj)) == NULL);
	assert((o = marray_pop(marray_obj)) == NULL);
	printf(".");

	mobject_free(marray_obj);

	/* Case 27: store into dict */
	mdict_obj = mdict_new();
	assert(mdict_insert(mdict_obj, mstring_new("a"), mnone_new()) == 0);
	assert(mdict_insert(mdict_obj, mstring_new("b"),
	    mstring_new("abc")) == 0);
	assert(mdict_insert(mdict_obj, mstring_new("c"),
	    mstring_new("def")) == 0);
	assert(mdict_insert(mdict_obj, mstring_new("d"),
	    mstring_new("ghi")) == 0);
	assert(mdict_insert(mdict_obj, mstring_new("e"), mint_new(-64)) == 0);
	assert(mdict_insert(mdict_obj, mstring_new("f"), mint_new(42)) == 0);
	assert(mdict_insert(mdict_obj, (k = mstring_new("a")),
	    (o2 = mint_new(666))) == -1);
	mobject_free(k);
	mobject_free(o2);
	printf(".");

	/* Case 28: mdict_len after insert */
	assert(mdict_len(mdict_obj) == 6);
	printf(".");

	/* Case 29: retrieve from dict */
	X_NONE_AT_DICT(mdict_obj, "a");
	X_STRING_AT_DICT(mdict_obj, "b", "abc");
	X_STRING_AT_DICT(mdict_obj, "c", "def");
	X_STRING_AT_DICT(mdict_obj, "d", "ghi");
	X_INT_AT_DICT(mdict_obj, "e", -64);
	X_INT_AT_DICT(mdict_obj, "f", 42);
	X_NULL_AT_DICT(mdict_obj, "x");
	printf(".");

	/* Case 30: replace in dict */
	assert(mdict_replace(mdict_obj, mstring_new("a"), mint_new(666)) == 0);
	assert(mdict_replace(mdict_obj, mstring_new("x"), mnone_new()) == 0);
	X_INT_AT_DICT(mdict_obj, "a", 666);
	X_NONE_AT_DICT(mdict_obj, "x");
	printf(".");

	/* Case 31: mdict_len after replace */
	assert(mdict_len(mdict_obj) == 7);
	printf(".");

	/* Case 32: remove from dict */
	assert((o2 = mdict_remove(mdict_obj,
	    (k = mstring_new("c")))) != NULL);
	mobject_free(k);
	assert(mobject_type(o2) == TYPE_MSTRING);
	assert(strcmp((char *)mstring_ptr(o2), "def") == 0);
	mobject_free(o2);
	assert((o2 = mdict_remove(mdict_obj,
	    (k = mstring_new("e")))) != NULL);
	mobject_free(k);
	assert(mint_value(o2) == -64);
	mobject_free(o2);
	assert((o2 = mdict_remove(mdict_obj, 
	    (k = mstring_new("z")))) == NULL);
	mobject_free(k);
	X_NULL_AT_DICT(mdict_obj, "c");
	X_NULL_AT_DICT(mdict_obj, "e");
	X_NULL_AT_DICT(mdict_obj, "z");
	printf(".");

	/* Case 33: mdict_len after remove */
	assert(mdict_len(mdict_obj) == 5);
	printf(".");

	/* Case 34: free populated dict */
	mobject_free(mdict_obj);
	printf(".");

	/* Case 35: mobject_to_string */
	mstring_obj = mstring_new("hello, world");
	mint_obj = mint_new(42);
	mnone_obj = mnone_new();
	marray_obj = marray_new();
	mdict_obj = mdict_new();
	T_XOTS(mstring_obj, "hello, world");
	T_XOTS(mint_obj, "42");
	T_XOTS(mnone_obj, "None");
	T_XOTS_PRE(marray_obj, "marray(");
	T_XOTS_PRE(mdict_obj, "mdict(");
	mobject_free(mstring_obj);
	mobject_free(mint_obj);
	mobject_free(mnone_obj);
	mobject_free(marray_obj);
	mobject_free(mdict_obj);
	printf(".");

	/* Case 36: mobject_to_string with non-ASCII chars */
	mstring_obj = mstring_new2(bin, sizeof(bin));
	T_XOTS(mstring_obj, "\\000\\001\\377hello\\363\\000");
	mobject_free(mstring_obj);
	printf(".");

	/* Case 37: array iterator */
	X_ARRAY_FILL(marray_obj);
	/* { "pqr", 4, 3, "klm", "abc", 1, None, 2, "def", "hij", None } */

	assert((it = mobject_getiter(marray_obj)) != NULL);
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
	miterator_free(it);
	printf(".");

	/* Case 39: free iterator midway through array traversal */
	assert((it = mobject_getiter(marray_obj)) != NULL);
	X_STRING_ARRAY_ITER(it, 0, "pqr");
	X_INT_ARRAY_ITER(it, 1, 4);
	X_INT_ARRAY_ITER(it, 2, 3);
	X_STRING_ARRAY_ITER(it, 3, "klm");
	X_STRING_ARRAY_ITER(it, 4, "abc");
	miterator_free(it);
	printf(".");

	mobject_free(marray_obj);

	/* Case 40: dict iterator */
	X_DICT_FILL(mdict_obj);
	/* { "a" : None, "b" : "abc", c : "def", d : -64, e : 42 } */
	/*
	 * This is a little more painful, as dict iteration doesn't
	 * guarantee an order
	 */
	seen = 31;
	assert((it = mobject_getiter(mdict_obj)) != NULL);
	for (n = 0; n < 5; n++) {
		const struct miteritem *ii;
		int kn;
		assert((ii = miterator_next(it)) != NULL);
		assert(mobject_type(ii->key) == TYPE_MSTRING);
		assert(mstring_len(ii->key) == 1);
		kn = mstring_ptr(ii->key)[0];
		assert((seen & (1 << (kn - 'a'))) != 0);
		seen &= ~(1 << (kn - 'a'));
		switch (kn) {
		case 'a':
			assert(mobject_type(ii->value) == TYPE_MNONE);
			break;
		case 'b':
			assert(mobject_type(ii->value) == TYPE_MSTRING);
			assert(strcmp((const char *)mstring_ptr(ii->value),
			    "abc") == 0);
			break;
		case 'c':
			assert(mobject_type(ii->value) == TYPE_MSTRING);
			assert(strcmp((const char *)mstring_ptr(ii->value),
			    "def") == 0);
			break;
		case 'd':
			assert(mobject_type(ii->value) == TYPE_MINT);
			assert(mint_value(ii->value) == -64);
			break;
		case 'e':
			assert(mobject_type(ii->value) == TYPE_MINT);
			assert(mint_value(ii->value) == 42);
			break;
		default:
			assert(0);
		}
	}
	assert(seen == 0);

	/* Case 41: free iterator after dict traversal */
	miterator_free(it);
	printf(".");

	mobject_free(mdict_obj);

	/* Case 42: marray_set and implicit filling */
	assert((marray_obj = marray_new()) != NULL);
	assert((mstring_obj = mstring_new2((u_int8_t *)"happy", 3)) != NULL);
	assert(marray_set(marray_obj, 5, mstring_obj) != -1);
	assert((mstring_obj = mstring_new2((u_int8_t *)"joyjoy", 2)) != NULL);
	assert(marray_set(marray_obj, 3, mstring_obj) != -1);
	assert((mstring_obj = mstring_new2((u_int8_t *)"merry!", 5)) != NULL);
	assert(marray_set(marray_obj, 3, mstring_obj) != -1);
	assert(marray_len(marray_obj) == 6);
	X_NONE_AT_ARRAY(marray_obj, 0);
	X_NONE_AT_ARRAY(marray_obj, 1);
	X_NONE_AT_ARRAY(marray_obj, 2);
	X_STRING_AT_ARRAY(marray_obj, 3, "merry");
	X_NONE_AT_ARRAY(marray_obj, 4);
	X_STRING_AT_ARRAY(marray_obj, 5, "hap");
	mobject_free(marray_obj);
	printf(".");

	/* XXX check that functions do not accept inappropriate objects */
	/* XXX concurrent iterations */
	/* XXX mdict_*_s function */

	printf("\n");
	return 0;
}
