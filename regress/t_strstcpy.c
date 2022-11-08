/*
 * Regress test for strstcpy function
 * Public domain -- Damien Miller <djm@mindrot.org> 2007-03-27
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "strstcpy.h"
#include "compat.h"

int
main(int argc, char **argv)
{
	char buf[256];

	/* Turn on all malloc debugging on OpenBSD */
	setenv("MALLOC_OPTIONS", "AFGJPRX", 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("t_strstcpy:");

	/* Case 1: simple copy, no stop chars, no trunction */
	assert(strstcpy(buf, "hello.there", sizeof(buf), NULL) == 11);
	assert(strcmp(buf, "hello.there") == 0);
	printf(".");

	/* Case 2: simple copy, no stop chars, with trunction */
	assert(strstcpy(buf, "hello.there", 6, NULL) == 11);
	assert(strcmp(buf, "hello") == 0);
	printf(".");

	/* Case 3: stopped copy, no trunction */
	assert(strstcpy(buf, "hello.there", sizeof(buf), ".") == 5);
	assert(strcmp(buf, "hello") == 0);
	printf(".");

	/* Case 4: stopped copy, with trunction */
	assert(strstcpy(buf, "hello.there", 6, ".") == 5);
	assert(strcmp(buf, "hello") == 0);
	printf(".");

	printf("\n");
	return 0;
}
