/*
 * Based on strlcpy
 *
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
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

/* $Id: strstcpy.h,v 1.1 2007/04/02 08:55:06 djm Exp $ */

#ifndef STRSTCPY_H
#include <sys/types.h>

/*
 * Copy up to "len" chars from "src" to "dst", stopping early if a '\0'
 * or a character from "stopchars" is encountered. Returns the number
 * of characters that would have been copied if "dst" was unlimited in
 * length (a la strlcpy)
 */
size_t
strstcpy(char *dst, const char *src, size_t len, const char *stopchars);

#endif /* STRSTCPY_H */
