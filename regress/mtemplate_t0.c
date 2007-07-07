/*
 * Regress test for mtemplate library
 * Public domain -- Damien Miller <djm@mindrot.org> 2007-04-16
 */

/* $Id$$ */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mobject.h"
#include "mtemplate.h"

int
main(int argc, char **argv)
{
	struct mobject *namespace;
	struct mtemplate *t;
	struct mobject *obj;
	char *o;

	/* Turn on all malloc debugging on OpenBSD */
	setenv("MALLOC_OPTIONS", "AFGJPRX", 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("mtemplate_t0:");

	assert((namespace = mdict_new()) != NULL);

	/* Case 1: Test trivial template compilation */
	t = mtemplate_parse("hello, world", NULL, 0);
	assert (t != NULL);
	printf(".");

	/* Case 2: Test trivial template execution */
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == 0);
	assert(strcmp(o, "hello, world") == 0);
	free(o);
	printf(".");

	/* Case 3: Free trivial template */
	mtemplate_free(t);
	printf(".");

	/* Case 4: Test compilation of template with escape sequence */
	t = mtemplate_parse("1{{{}} 2{{{{}} 3{{{{{}} 4{{{{{{}}", NULL, 0);
	assert(t != NULL);
	printf(".");

	/* Case 5: Test escape execution */
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == 0);
	assert(strcmp(o, "1{ 2{{ 3{{{ 4{{{{") == 0);
	mtemplate_free(t);
	free(o);
	printf(".");

	/* Case 6: Test bad syntax - escape prefix followed by junk */
	t = mtemplate_parse("{{{crap}}", NULL, 0);
	assert(t == NULL);
	printf(".");

	/* Case 7: Test compilation of template with basic substitutions */
	t = mtemplate_parse("ABC {{v1}} || {{v2}} XYZ", NULL, 0);
	assert(t != NULL);
	printf(".");

	/* Case 8: Test error on namespace lookup fail */
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == -1);
	assert(mdict_insert_ss(namespace, "v1", "happy") != NULL);
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == -1);
	printf(".");

	/* Case 9: Test basic substitution */
	assert(mdict_insert_ss(namespace, "v2", "days!") != NULL);
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == 0);
	assert(strcmp(o, "ABC happy || days! XYZ") == 0);
	free(o);
	printf(".");

	/* Case 10: Test free of template with substitution */
	mtemplate_free(t);
	printf(".");

	/* Case 11: Test compilation of template with conditional */
	t = mtemplate_parse("{{if v}}{{v1}}{{else}}{{v2}}{{endif}}",
	    NULL, 0);
	assert(t != NULL);
	printf(".");

	/* Case 12: Test failure with undefined conditional */
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == -1);
	printf(".");

	/* Case 13: Test conditional true */
	assert(mdict_insert_si(namespace, "v", 1) != NULL);
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == 0);
	assert(strcmp(o, "happy") == 0);
	free(o);
	printf(".");

	/* Case 14: Test conditional false */
	assert(mdict_replace_si(namespace, "v", 0) != NULL);
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == 0);
	assert(strcmp(o, "days!") == 0);
	free(o);
	printf(".");

	/* Case 15: Test free of template with conditional */
	mtemplate_free(t);
	printf(".");

	/* Case 16: Test bad syntax - double else */
	t = mtemplate_parse("{{if v}}{{v1}}{{else}}{{v2}}{{else}}x{{endif}}",
	    NULL, 0);
	assert(t == NULL);
	printf(".");

	/* Case 17: Test bad syntax - double endif */
	t = mtemplate_parse("{{if v}}{{v1}}{{else}}{{v2}}{{endif}}x{{endif}}",
	    NULL, 0);
	assert(t == NULL);
	printf(".");

	/* Case 18: Test bad syntax - unclosed if */
	t = mtemplate_parse("{{if v}}{{v1}}{{else}}{{v2}}",
	    NULL, 0);
	assert(t == NULL);
	printf(".");

	/* Case 19: Test compilation of iteration */
	t = mtemplate_parse("{{for x in v}}K:{{x.key}}V:{{x.value}} {{endfor}}",
	    NULL, 0);
	assert(t != NULL);
	printf(".");

	/* Case 20: Test failure when iterating over non-sequence */
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == -1);
	printf(".");

	/* Case 21: iteration over array */
	assert(mdict_replace_sa(namespace, "v") != NULL);
	assert((obj = mdict_item_s(namespace, "v")) != NULL);
	assert(marray_append_s(obj, "wow") != NULL);
	assert(marray_append_s(obj, "this") != NULL);
	assert(marray_append_s(obj, "worked") != NULL);
	assert(marray_append_i(obj, 765432) != NULL);
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == 0);
	assert(strcmp(o, "K:0V:wow K:1V:this K:2V:worked K:3V:765432 ") == 0);
	free(o);
	printf(".");

	/* Case 22: iteration over dictionary */
	assert(mdict_replace_sd(namespace, "v") != NULL);
	assert((obj = mdict_item_s(namespace, "v")) != NULL);
	assert(mdict_insert_ss(obj, "k1", "v1") != NULL);
	assert(mdict_insert_ss(obj, "k2", "v2") != NULL);
	assert(mtemplate_run_mbuf(t, namespace, &o, NULL, 0) == 0);
	/* Dictionary iteration doesn't guarantee order */
	assert(strstr(o, "K:k1V:v1 ") != 0);
	assert(strstr(o, "K:k2V:v2 ") != 0);
	assert(strlen(o) == strlen("K:k2V:v2 K:k1V:v1 "));
	free(o);
	mobject_free(namespace);
	printf(".");

	/* Case 23: Test free of template with loop */
	mtemplate_free(t);
	printf(".");

	/* Case 19: Test bad syntax - double endfor */
	t = mtemplate_parse("{{for x in v}}K:{{x.key}} {{endfor}}{{endfor}}",
	    NULL, 0);
	assert(t == NULL);
	printf(".");

	/* Case 20: Test bad syntax - unclosed for */
	t = mtemplate_parse("{{for x in v}}K:{{x.key}} ",
	    NULL, 0);
	assert(t == NULL);
	printf(".");

	/* test complex and deep template */
	/* test error messages */
	/* test line numbers in error */

	printf("\n");
	return 0;
}
