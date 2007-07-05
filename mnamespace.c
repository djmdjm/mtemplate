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

/* $Id$ */

/* Namespace resolution */

#include <sys/types.h>
#include <sys/param.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "mobject.h"
#include "compat.h"
#include "strstcpy.h"

#define MNAMESPACE_MAX_NAME_LENGTH		4096
#define MNAMESPACE_MAX_ID_LENGTH		256

static void
format_err(size_t o, const char *loc, char *ebuf, size_t elen,
    const char *fmt, ...)
{
	char at[MNAMESPACE_MAX_NAME_LENGTH];
	va_list args;

	if (ebuf == NULL || elen == 0)
		return;

	*at = '\0';
	if (o > 0 && o <= strlen(loc))
		snprintf(at, sizeof(at), " at \"%.*s\"", (int)o, loc);

	va_start(args, fmt);
	vsnprintf(ebuf, elen, fmt, args);
	va_end(args);

	strlcat(ebuf, at, elen);
}

static int
array_ndx(char *s, size_t *alen, size_t *skip, char **errp)
{
	char buf[32], *ep;
	long long llval;

	/* Extract array length */
	if ((*skip = strstcpy(buf, s, sizeof(buf), "]")) >= sizeof(buf)) {
		*skip = 0;
		*errp = "Array index is too long";
		return -1;
	}
	if (*skip == 0) {
		*errp = "Array index is empty";
		return -1;
	}
	if (*(s + *skip) != ']') {
		*errp = "Array index is not terminated";
		return -1;
	}
	(*skip)++;

	llval = strtol(buf, &ep, 0);
	if (s[0] == '\0' || *ep != '\0') {
		*errp = "Array index is not a number";
		return -1;
	}
	if (llval < 0 || llval > INT_MAX) {
		*errp = "Array index is out of bounds";
		return -1;
	}
	*alen = (size_t)llval;
	return 0;
}

/* XXX this really needs meaningful return codes */
int
mnamespace_lookup(struct mobject *ns, char *location, struct mobject **obj,
    char *ebuf, size_t elen)
{
	struct mobject *current = ns;
	struct mobject *next;
	char name[MNAMESPACE_MAX_ID_LENGTH];
	char type, *cp;
	size_t l, o = 0, ndx;

	if (mobject_type(ns) != TYPE_MDICT) {
		format_err(o, location, ebuf, elen,
		    "Namespace is not of dictionary type");
		return -1;
	}

	if (obj != NULL)
		*obj = NULL;
	if (*location == '\0') {
		format_err(o, location, ebuf, elen,
		    "Empty location specified");
		return -1;
	}

	for (;;) {
		l = strstcpy(name, location + o, sizeof(name), ".[");
		if (l == 0) {
			format_err(o, location, ebuf, elen, "Empty name");
			return -1;
		}
		if (l >= sizeof(name)) {
			format_err(o, location, ebuf, elen,
			    "Name \"%.8s...\" too long", name);
			return -1;
		}

		if ((next = mdict_item_s(current, name)) == NULL) {
			format_err(o, location, ebuf, elen,
			    "Name \"%s\" not found", name);
			return -1;
		}
		o += l;
 resolve_next:
		type = *(location + o++);
		if (type == '\0') {
			break;
		} else if (type == '.') {
			if (mobject_type(next) != TYPE_MDICT) {
				format_err(o, location, ebuf, elen,
				    "Name \"%s\" is not a dictionary", name);
				return -1;
			}
			current = next;
		} else if (type == '[') {
			if (mobject_type(next) != TYPE_MARRAY) {
				format_err(o, location, ebuf, elen,
				    "Name \"%s\" is not an array", name);
				return -1;
			}
			/* Extract array index */
			if ((array_ndx(location + o, &ndx, &l, &cp)) != 0) {
				o += l;
				format_err(o, location, ebuf, elen, "%s", cp);
				return -1;
			}
			o += l;
			if (ndx >= marray_len(next)) {
				format_err(o, location, ebuf, elen,
				    "Array index is out of bounds");
				return -1;
			}			
			next = marray_item(next, ndx);
			goto resolve_next;
		} else {
			format_err(o, location, ebuf, elen, "Parse error");
			return -1;
		}
	}

	if (obj != NULL)
		*obj = next;
	return 0;
}

static int
xalloc_by_typechar(int typechar, struct mobject **xop, char **namep)
{
	switch (typechar) {
	case '.':
		if ((*xop = mdict_new()) == NULL) {
			*namep = "Allocation of dictionary failed";
			return -1;
		}
		return 0;
	case '[':
		if ((*xop = marray_new()) == NULL) {
			*namep = "Allocation of array failed";
			return -1;
		}
		return 0;
	default:
		*namep = "Parse error";
		return -1;
	}
}

/* XXX this really needs meaningful return codes */
/* XXX O gorgon! this is ugly code */
int
mnamespace_set(struct mobject *ns, char *location, struct mobject *obj,
    char *ebuf, size_t elen)
{
	struct mobject *current = ns;
	struct mobject *next, *n2;
	char name[MNAMESPACE_MAX_ID_LENGTH];
	char type, *cp;
	size_t ndx, l, o = 0;

	if (mobject_type(ns) != TYPE_MDICT) {
		format_err(o, location, ebuf, elen,
		    "Namespace is not of dictionary type");
		return -1;
	}

	if (*location == '\0') {
		format_err(o, location, ebuf, elen,
		    "Empty location specified");
		return -1;
	}

	for (;;) {
		l = strstcpy(name, location + o, sizeof(name), ".[");
		if (l == 0) {
			format_err(o, location, ebuf, elen, "Empty name");
			return -1;
		}
		if (l >= sizeof(name)) {
			format_err(o, location, ebuf, elen,
			    "Name \"%.8s...\" too long", name);
			return -1;
		}
		next = mdict_item_s(current, name);
		o += l;
		type = *(location + o++);
 next_array:

		/* We are at the end of the string, insert the object here */
		if (type == '\0') {
			if (mdict_replace_s(current, name, obj) == -1) {
				format_err(o, location, ebuf, elen,
					"mdict_insert_s failed");
				return -1;
			}
			return 0;
		}

		/* If no entry exists, create one */
		if (next == NULL) {
			if (xalloc_by_typechar(type, &next, &cp) == -1) {
				format_err(o, location, ebuf, elen, "%s", cp);
				return -1;
			}
			if (mdict_replace_s(current, name, next) == -1) {
				format_err(o, location, ebuf, elen,
				    "mdict_insert_s failed");
				return -1;
			}
			/* Continue processing below */
		}

 next_is_dict:
		if (type == '.') {
			if (mobject_type(next) != TYPE_MDICT) {
				format_err(o, location, ebuf, elen,
				    "\"%s\" is not a dictionary", name);
				return -1;
			}
			current = next;
			continue;
		}

		if (type != '[') {
			format_err(o, location, ebuf, elen, "Parse error");
			return -1;
		}

		/* Next entry is an array */
		if (array_ndx(location + o, &ndx, &l, &cp) == -1) {
			format_err(o + l, location, ebuf, elen, "%s", cp);
			return -1;
		}
		o += l;
		if (mobject_type(next) != TYPE_MARRAY) {
			format_err(o, location, ebuf, elen,
			    "\"%s\" is not an array", name);
			return -1;
		}
		type = *(location + o++);
		if (type == '\0') {
			/*
			 * We are at the end of the string, so
			 * insert the object here.
			 */
			if (marray_set(next, ndx, obj) == -1) {
				format_err(o, location, ebuf, elen,
				    "marray_set failed");
				return -1;
			}
			return 0;
		}
		if ((n2 = marray_item(next, ndx)) == NULL) {
			/*
			 * If the entry doesn't exist yet in the array,
			 * create and insert it. We need to look ahead to
			 * determine what type of entry to create.
			 */
			if (xalloc_by_typechar(type, &n2, &cp) == -1) {
				format_err(o, location, ebuf, elen, "%s", cp);
				return -1;
			}
			if (marray_set(next, ndx, n2) == -1) {
				format_err(o, location, ebuf, elen,
				"mdict_insert_s failed");
				return -1;
			}
		}
		next = n2;
		/* If next reference is an array, don't try to read a name */
		if (type == '[')
			goto next_array;

		/*
		 * Otherwise, next reference is a dict - check it
		 * and update current pointer before continuing.
		 */
		goto next_is_dict;
	}		
}
