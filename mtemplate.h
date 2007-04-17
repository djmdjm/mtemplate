/*
 * Copyright (c) Damien Miller <djm@mindrot.org>
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

/* $Id$ */

/* Simple template system */

#ifndef _XTEMPLATE_H
#define _XTEMPLATE_H

#include <sys/types.h>
#include <stdio.h>

#include "xobject.h"

struct xtemplate;

/*
 * Parse the text 'template', returning a compiled representation suitable
 * for later use by xtemplate_run().
 *
 * Returns a compiled template on success, or NULL if parsing.failed. On
 * error, up to 'elen' bytes of error message will be written to 'ebuf'.
 */
struct xtemplate *xtemplate_parse(const char *template,
    char *ebuf, size_t elen);

/*
 * Run the pre-compiled template 't', with an libxobject dictionary
 * 'namespace'. Output will be written to 'out'.
 *
 * Returns a 0 on success, or -1 on failure. On failue, up to 'elen' bytes of
 * error message will be written to 'ebuf'.
 */
int xtemplate_run_stdio(struct xtemplate *t, struct xobject *namespace,
    FILE *out, char *ebuf, size_t elen);

/*
 * Run the pre-compiled template 't', with an libxobject dictionary
 * 'namespace'. Output will be returned in a string buffer pointed to by
 * 'outp'. It is the caller's responsibility to deallocate this buffer.
 *
 * Returns a 0 on success, or -1 on failure. On failue, up to 'elen' bytes of
 * error message will be written to 'ebuf'.
 */
int
xtemplate_run_mbuf(struct xtemplate *t, struct xobject *namespace, char **outp,
    char *ebuf, size_t elen);

/*
 * Run the pre-compiled template 't', with an libxobject dictionary
 * 'namespace'. Output will occur via the 'out_cb' callback, which will be
 * called for each hunk of text that is generated. The 'out_ctx' argument is
 * passed through to the callback for context.
 *
 * The first argument to the callback is a pointer to the nul-terminated
 * string to be written, the second argument is the 'out_ctx'. On success,
 * the callback must return 0. If the callback returns any other value,
 * template generation will be terminated.
 *
 * Returns a 0 on success, or -1 on failure. On failue, up to 'elen' bytes of
 * error message will be written to 'ebuf'.
 */
int
xtemplate_run_cb(struct xtemplate *t, struct xobject *namespace, char *ebuf,
    size_t elen, int (*out_cb)(const char *, void *), void *out_ctx);

#endif /* _XTEMPLATE_H */
