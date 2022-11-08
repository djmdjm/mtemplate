/*
 * Regress test for mobject_cmp
 * Public domain -- Damien Miller <djm@mindrot.org> 2007-07-07
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mobject.h"
#include "compat.h"

#define CMP_TESTS do { \
		assert(mobject_cmp(o1, o1) == 0);	\
		assert(mobject_cmp(o1, o2) == -1);	\
		assert(mobject_cmp(o1, o3) == -1);	\
		assert(mobject_cmp(o2, o1) == 1);	\
		assert(mobject_cmp(o3, o1) == 1);	\
		assert(mobject_cmp(o2, o3) == 0);	\
		printf(".");				\
	} while (0)

int
main(int argc, char **argv)
{
	struct mobject *o1, *o2, *o3;
	struct mobject *a, *d, *i, *s, *n;

	/* Turn on all malloc debugging on OpenBSD */
	setenv("MALLOC_OPTIONS", "AFGJPRX", 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("mobject_t3:");

	assert((o1 = mint_new(100)) != NULL);
	assert((o2 = mint_new(200)) != NULL);
	assert((o3 = mint_new(200)) != NULL);

	CMP_TESTS;

	mobject_free(o1);
	mobject_free(o2);
	mobject_free(o3);

	assert((o1 = mstring_new("abc")) != NULL);
	assert((o2 = mstring_new("def")) != NULL);
	assert((o3 = mstring_new("def")) != NULL);

	CMP_TESTS;

	mobject_free(o1);
	mobject_free(o2);
	mobject_free(o3);

	assert((o1 = marray_new()) != NULL);
	assert((o2 = marray_new()) != NULL);
	assert((o3 = marray_new()) != NULL);

	assert(marray_append_s(o1, "hello") != NULL);
	assert(marray_append_s(o1, "there") != NULL);
	assert(marray_append_s(o2, "hi") != NULL);
	assert(marray_append_s(o2, "there") != NULL);
	assert(marray_append_i(o2, 456) != NULL);
	assert(marray_append_s(o3, "hi") != NULL);
	assert(marray_append_s(o3, "there") != NULL);
	assert(marray_append_i(o3, 456) != NULL);

	CMP_TESTS;
	assert(marray_append_i(o1, 123) != NULL);
	CMP_TESTS;
	assert(marray_set_s(o1, 0, "hi") != NULL);
	CMP_TESTS;
	assert(marray_prepend_n(o1) != NULL);
	assert(marray_append_n(o2) != NULL);
	assert(marray_append_n(o3) != NULL);
	CMP_TESTS;

	mobject_free(o1);
	mobject_free(o2);
	mobject_free(o3);

	assert((o1 = mnone_new()) != NULL);
	assert((o2 = mnone_new()) != NULL);
	assert(mobject_cmp(o1, o1) == 0);
	assert(mobject_cmp(o1, o2) == 0);
	printf(".");

	assert((o1 = mdict_new()) != NULL);
	assert((o2 = mdict_new()) != NULL);
	assert(mobject_cmp(o1, o1) == 0);
	assert(mobject_cmp(o1, o2) != 0);
	printf(".");

	mobject_free(o1);
	mobject_free(o2);

	/* Precedence */
	assert((n = mnone_new()) != NULL);
	assert((i = mint_new(123)) != NULL);
	assert((s = mstring_new("hello")) != NULL);
	assert((a = marray_new()) != NULL);
	assert((d = mdict_new()) != NULL);

	assert(mobject_cmp(n, n) == 0);
	assert(mobject_cmp(n, i) == -1);
	assert(mobject_cmp(n, s) == -1);
	assert(mobject_cmp(n, a) == -1);
	assert(mobject_cmp(n, d) == -1);
	printf(".");

	assert(mobject_cmp(i, n) == 1);
	assert(mobject_cmp(i, i) == 0);
	assert(mobject_cmp(i, s) == -1);
	assert(mobject_cmp(i, a) == -1);
	assert(mobject_cmp(i, d) == -1);
	printf(".");

	assert(mobject_cmp(s, n) == 1);
	assert(mobject_cmp(s, i) == 1);
	assert(mobject_cmp(s, s) == 0);
	assert(mobject_cmp(s, a) == -1);
	assert(mobject_cmp(s, d) == -1);
	printf(".");

	assert(mobject_cmp(a, n) == 1);
	assert(mobject_cmp(a, i) == 1);
	assert(mobject_cmp(a, s) == 1);
	assert(mobject_cmp(a, a) == 0);
	assert(mobject_cmp(a, d) == -1);
	printf(".");

	assert(mobject_cmp(d, n) == 1);
	assert(mobject_cmp(d, i) == 1);
	assert(mobject_cmp(d, s) == 1);
	assert(mobject_cmp(d, a) == 1);
	assert(mobject_cmp(d, d) == 0);
	printf(".");

	mobject_free(n);
	mobject_free(i);
	mobject_free(a);
	mobject_free(s);
	mobject_free(d);

	printf("\n");
	return 0;
}
