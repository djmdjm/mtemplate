/*
 * Regress test for xnamespace functions
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

#include "mobject.h"
#include "strlcat.h"

#if 0
static void
mobject_dump(struct mobject *o)
{
	char buf[8192];
	struct miterator *iter;
	struct miteritem *item;

	switch (mobject_type(o)) {
	case TYPE_MNONE:
	case TYPE_MSTRING:
	case TYPE_MINT:
		mobject_to_string(o, buf, sizeof(buf));
		fputs(buf, stdout);
		break;
	case TYPE_MARRAY:
		assert((iter = mobject_getiter(o)) != NULL);
		fputs("array(", stdout);
		while ((item = miterator_next(iter)) != NULL) {
			fputs("{", stdout);
			mobject_dump(item->value);
			fputs("}, ", stdout);
		}
		fputs(")", stdout);
		miterator_free(iter);
		break;
	case TYPE_MDICT:
		assert((iter = mobject_getiter(o)) != NULL);
		fputs("dict(", stdout);
		while ((item = miterator_next(iter)) != NULL) {
			fputs("{", stdout);
			mobject_dump(item->key);
			fputs("} : {", stdout);
			mobject_dump(item->value);
			fputs("}, ", stdout);
		}
		fputs(")", stdout);
		miterator_free(iter);
		break;
	default:
		assert(0);
	}
}
#endif

int
main(int argc, char **argv)
{
	struct mobject *ns;
	struct mobject *xo;
	struct mobject *x_hello, *x_start_a2;
	struct mobject *x_start_a4_20, *x_start_a4_21_a, *x_start_a3_b;
	char ebuf[8192];

	/* Turn on all malloc debugging on OpenBSD */
	setenv("MALLOC_OPTIONS", "AFGJPRX", 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("mobject_t2:");

	/* Prepare an namespace */
	assert((ns = mdict_new()) != NULL);

	/* Case 1: Try an insert */
	assert((xo = mstring_new("x_hello")) != NULL);
	x_hello = xo;
	assert(xnamespace_set(ns, "hello", xo, ebuf, sizeof(ebuf)) == 0);
	assert(xnamespace_lookup(ns, "hello", &xo, ebuf, sizeof(ebuf)) == 0);
	assert(xo == x_hello);
	printf(".");

	/* Case 2: Don't clobber a scalar with an implicit */
	assert((xo = mstring_new("x_hello_a")) != NULL);
	assert(xnamespace_set(ns, "hello.a", xo,
	    ebuf, sizeof(ebuf)) == -1);
	printf(".");

	/* Case 3: Deeper inserts, incremental */
	assert((xo = mstring_new("x_start_a2")) != NULL);
	x_start_a2 = xo;
	assert(xnamespace_set(ns, "start.a[2]", xo,
	    ebuf, sizeof(ebuf)) == 0);
	assert((xo = mstring_new("x_start_a3_b")) != NULL);
	x_start_a3_b = xo;

	assert(xnamespace_set(ns, "start.a[3].b", xo,
	    ebuf, sizeof(ebuf)) == 0);
	assert((xo = mstring_new("x_start_a4_20")) != NULL);
	x_start_a4_20 = xo;

	assert(xnamespace_set(ns, "start.a[4][20]", xo,
	    ebuf, sizeof(ebuf)) == 0);
	assert((xo = mstring_new("x_start_a4_21_a")) != NULL);
	x_start_a4_21_a = xo;

	assert(xnamespace_set(ns, "start.a[4][21].a", xo,
	    ebuf, sizeof(ebuf)) == 0);

	assert(xnamespace_lookup(ns, "start.a[2]", &xo,
	    ebuf, sizeof(ebuf)) == 0);
	assert(xo == x_start_a2);
	assert(xnamespace_lookup(ns, "start.a[3].b", &xo,
	    ebuf, sizeof(ebuf)) == 0);
	assert(xo == x_start_a3_b);
	assert(xnamespace_lookup(ns, "start.a[4][20]", &xo,
	    ebuf, sizeof(ebuf)) == 0);
	assert(xo == x_start_a4_20);
	assert(xnamespace_lookup(ns, "start.a[4][21].a", &xo,
	    ebuf, sizeof(ebuf)) == 0);
	assert(xo == x_start_a4_21_a);
	printf(".");


	mobject_free(ns);

	printf("\n");
	return 0;
}
