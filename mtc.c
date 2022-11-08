/*
 * Copyright (c) 2007 Damien Miller <djm@mindrot.org>
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

/* Simple mtemplate command line processor */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>

#include "mtemplate.h"
#include "mobject.h"

static void
usage(void)
{
	fprintf(stderr,
	    "Usage: xtc [-h] [-D key=value] [-o output-file] template-file\n");
}

static void
define(struct mobject *namespace, const char *kv)
{
	const char *cp;
	char kbuf[256], ebuf[512];
	struct mobject *v;

	if ((cp = strchr(kv, '=')) == NULL || cp == kv || *(cp + 1) == '\0') {
		warnx("Invalid define");
		usage();
		exit(1);
	}
	if ((size_t)(cp - kv) >= sizeof(kbuf))
		errx(1, "Define key too long");
	memcpy(kbuf, kv, cp - kv);
	kbuf[cp - kv] = '\0';
	if ((v = mstring_new(cp + 1)) == NULL)
		errx(1, "mstring_new failed");

	if (mnamespace_set(namespace, kbuf, v, ebuf, sizeof(ebuf)) != 0)
		errx(1, "mnamespace_set: %s", ebuf);
}

int
main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int len, ch, tfd;
	char *template, buf[8192];
	const char *out_path = "-";
	size_t tlen;
	struct mtemplate *t;
	struct mobject *namespace;
	FILE *out;

	if ((namespace = mdict_new()) == NULL)
		errx(1, "mdict_new failed");
	while ((ch = getopt(argc, argv, "hD:o:")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			exit(0);
		case 'D':
			define(namespace, optarg);
			break;
		case 'o':
			out_path = optarg;
			break;
		default:
			warnx("Unrecognised command line option");
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1) {
		warnx("No template file specified");
		usage();
		exit(1);
	}

	if (strcmp(argv[0], "-") == 0)
		tfd = STDIN_FILENO;
	else if ((tfd = open(argv[0], O_RDONLY)) == -1)
		err(1, "open(\"%s\", O_RDONLY)", argv[0]);

	template = NULL;
	tlen = 0;
	for (;;) {
		if ((len = read(tfd, buf, sizeof(buf))) == -1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			err(1, "read");
		}
		if (len == 0)
			break;
		if (tlen + len + 1 < tlen)
			errx(1, "template length exceeds SIZE_T_MAX");
		if ((template = realloc(template, tlen + len + 1)) == NULL)
			errx(1, "realloc(template, %zu) failed",
			    tlen + len + 1);
		memcpy(template + tlen, buf, len);
		*(template + tlen + len) = '\0';
		tlen += len;
	}
	close(tfd);

	if ((t = mtemplate_parse(template, buf, sizeof(buf))) == NULL)
		errx(1, "mtemplate_parse: %s", buf);

	if (strcmp(out_path, "-") == 0)
		out = stdout;
	else if ((out = fopen(out_path, "w")) == NULL)
		err(1, "fopen(\"%s\")", out_path);

	if (mtemplate_run_stdio(t, namespace, out, buf, sizeof(buf)) == -1)
		errx(1, "mtemplate_run: %s", buf);

	fclose(out);

	return 0;
}
