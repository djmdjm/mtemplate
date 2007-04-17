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

int
main(int argc, char **argv)
{
	struct mobject *ns, *xd, *xd2;
	struct mobject *xa, *xa2;
	struct mobject *xi;
	u_int i;
	char ebuf[8192], nbuf[8192];
	struct mobject *xo;
	struct mobject *hello, *there, *there42, *z24, *z10_1, *z10_2_wow;

	/* Turn on all malloc debugging on OpenBSD */
	setenv("MALLOC_OPTIONS", "AFGJPRX", 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("t1:");

	/* Prepare an namespace */
	assert((ns = xd = mdict_new()) != NULL);
	assert((xd2 = mdict_new()) != NULL);
	hello = xd2;
	assert(mdict_insert_s(xd, "hello", xd2) == 0);
	assert((xa = marray_new()) != NULL);
	there = xa;
	assert(mdict_insert_s(xd2, "there", xa) == 0);

	for (i = 0; i < 43; i++) {
		assert((xd2 = mdict_new()) != NULL);
		assert(marray_append(xa, xd2) == 0);
	}
	there42 = xd2;

	assert((xd = mdict_new()) != NULL);
	assert(mdict_insert_s(xd2, "x", xd) == 0);
	assert((xd2 = mdict_new()) != NULL);
	assert(mdict_insert_s(xd, "y", xd2) == 0);
	assert((xa = marray_new()) != NULL);
	assert(mdict_insert_s(xd2, "z", xa) == 0);

	for (i = 0; i < 30; i++) {
		if (i == 10) {
			assert((xa2 = marray_new()) != NULL);
			assert(marray_append(xa, xa2) == 0);
			assert((xi = mint_new(1234)) != NULL);
			assert(marray_append(xa2, xi) == 0);
			assert((xi = mint_new(5678)) != NULL);
			assert(marray_append(xa2, xi) == 0);
			z10_1 = xi;
			assert((xd = mdict_new()) != NULL);
			assert(marray_append(xa2, xd) == 0);
			assert(mdict_insert_ss(xd, "wow", "indeed") == 0);
			assert((z10_2_wow = mdict_item_s(xd, "wow")) != NULL);
			continue;
		}
		assert((xi = mint_new(i)) != NULL);
		assert(marray_append(xa, xi) == 0);
		if (i == 24)
			z24 = xi;
	}

	/* Case 1: Lookup null at root*/
	assert(xnamespace_lookup(ns, "", &xo, ebuf, sizeof(ebuf)) == -1);
	assert(strcmp(ebuf, "Empty location specified") == 0);
	printf(".");

	/* Case 2: Lookup one level down */
	assert(xnamespace_lookup(ns, "hello", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xo == hello);
	printf(".");

	/* Case 3: Lookup empty one level down */
	assert(xnamespace_lookup(ns, "hello.", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf, "Empty name at \"hello.\"") == 0);
	printf(".");

	/* Case 4: lookup under second-level dict */
	assert(xnamespace_lookup(ns, "hello.there", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xo == there);
	printf(".");

	/* Case 5: lookup array entry */
	assert(xnamespace_lookup(ns, "hello.there[42]", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xo == there42);
	printf(".");

	/* Case 6: Deep lookups */
	assert(xnamespace_lookup(ns, "hello.there[42].x", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xnamespace_lookup(ns, "hello.there[42].x.y", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xnamespace_lookup(ns, "hello.there[42].x.y.z", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xnamespace_lookup(ns, "hello.there[42].x.y.z[24]", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xo == z24);
	printf(".");

	/* Case 7: Bad name at root */
	assert(xnamespace_lookup(ns, "xhello", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf, "Name \"xhello\" not found") == 0);
	assert(xnamespace_lookup(ns, "xhello.there[42].x.y.z[24]", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf, "Name \"xhello\" not found") == 0);
	printf(".");

	/* Case 8: Bad name one down */
	assert(xnamespace_lookup(ns, "hello.therex", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf, "Name \"therex\" not found at \"hello.\"") == 0);
	assert(xnamespace_lookup(ns, "hello.therex[42].x.y.z[24]", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf, "Name \"therex\" not found at \"hello.\"") == 0);
	printf(".");

	/* Case 9: Array index out of bounds */
	assert(xnamespace_lookup(ns, "hello.there[43].x.y.z[24]", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf,
	    "Array index is out of bounds at \"hello.there[43]\"") == 0);
	printf(".");

	/* Case 10: Negative array index */
	assert(xnamespace_lookup(ns, "hello.there[-1].x.y.z[24]", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf,
	    "Array index is out of bounds at \"hello.there[-1]\"") == 0);
	printf(".");

	/* Case 11: Non numeric array index */
	assert(xnamespace_lookup(ns, "hello.there[blah].x.y.z[24]", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf,
	    "Array index is not a number at \"hello.there[blah]\"") == 0);
	printf(".");

	/* Case 12: Unterminated array index */
	assert(xnamespace_lookup(ns, "hello.there[123", &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf,
	    "Array index is not terminated at \"hello.there[123\"") == 0);
	printf(".");

	/* Case 13: Identifier too long */
	snprintf(nbuf, sizeof(nbuf), "hello.");
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /* 32*/
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /* 64*/
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /* 96*/
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /*128*/
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /*160*/
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /*192*/
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /*256*/
	strlcat(nbuf, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof(nbuf)); /*288*/
	assert(xnamespace_lookup(ns, nbuf, &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf,
	    "Name \"XXXXXXXX...\" too long at \"hello.\"") == 0);
	printf(".");

	/* Case 13: Array index too long */
	snprintf(nbuf, sizeof(nbuf), "hello.there[");
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /* 32*/
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /* 64*/
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /* 96*/
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /*128*/
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /*160*/
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /*192*/
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /*256*/
	strlcat(nbuf, "99999999999999999999999999999999", sizeof(nbuf)); /*288*/
	strlcat(nbuf, "]", sizeof(nbuf)); /*280*/
	assert(xnamespace_lookup(ns, nbuf, &xo, ebuf,
	    sizeof(ebuf)) == -1);
	assert(strcmp(ebuf,
	    "Array index is too long at \"hello.there[\"") == 0);
	printf(".");

	/* Case 14: Array then array */
	assert(xnamespace_lookup(ns, "hello.there[42].x.y.z[10][1]", &xo, ebuf,
	    sizeof(ebuf)) == 0);
	assert(xo == z10_1);
	printf(".");

	/* Case 14: Array then array then dict! */
	assert(xnamespace_lookup(ns, "hello.there[42].x.y.z[10][2].wow",
	    &xo, ebuf, sizeof(ebuf)) == 0);
	assert(xo == z10_2_wow);
	printf(".");

	mobject_free(ns);

	printf("\n");
	return 0;
}
