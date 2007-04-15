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

#include <sys/types.h>
#include <stddef.h>

#include "xobject.h"

/* XXX: this is all mechanical, move most of it to #defines */

int
xarray_append_s(struct xarray *array, const char *v)
{
	struct xobject *tmp;
	int r;

	if ((tmp = (struct xobject *)xstring_new(v)) == NULL)
		return -1;
	if ((r = xarray_append(array, tmp)) == -1)
		xobject_free(tmp);
	return r;
}

int
xarray_append_i(struct xarray *array, int64_t v)
{
	struct xobject *tmp;
	int r;

	if ((tmp = (struct xobject *)xint_new(v)) == NULL)
		return -1;
	if ((r = xarray_append(array, tmp)) == -1)
		xobject_free(tmp);
	return r;
}

struct xobject *
xdict_item_s(const struct xdict *dict, const char *key)
{
	struct xstring *tmp;
	struct xobject *r;

	if ((tmp = xstring_new(key)) == NULL)
		return NULL;
	r = xdict_item(dict, tmp);
	xobject_free((struct xobject *)tmp);
	return r;
}

struct xobject *
xdict_remove_s(struct xdict *dict, const char *key)
{
	struct xstring *tmp;
	struct xobject *r;

	if ((tmp = xstring_new(key)) == NULL)
		return NULL;
	r = xdict_remove(dict, tmp);
	xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_delete_s(struct xdict *dict, const char *key)
{
	struct xstring *tmp;
	int r;

	if ((tmp = xstring_new(key)) == NULL)
		return -1;
	r = xdict_delete(dict, tmp);
	xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_insert_s(struct xdict *dict, const char *key, struct xobject *value)
{
	struct xstring *tmp;
	int r;

	if ((tmp = xstring_new(key)) == NULL)
		return -1;
	if ((r = xdict_insert(dict, tmp, value)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_insert_ss(struct xdict *dict, const char *key, const char *value)
{
	struct xstring *tmp;
	int r;

	if ((tmp = xstring_new(value)) == NULL)
		return -1;
	if ((r = xdict_insert_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int 
xdict_insert_si(struct xdict *dict, const char *key, int64_t value)
{
	struct xint *tmp;
	int r;

	if ((tmp = xint_new(value)) == NULL)
		return -1;
	if ((r = xdict_insert_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_insert_sd(struct xdict *dict, const char *key)
{
	struct xdict *tmp;
	int r;

	if ((tmp = xdict_new()) == NULL)
		return -1;
	if ((r = xdict_insert_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_insert_sa(struct xdict *dict, const char *key)
{
	struct xarray *tmp;
	int r;

	if ((tmp = xarray_new()) == NULL)
		return -1;
	if ((r = xdict_insert_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_insert_sn(struct xdict *dict, const char *key)
{
	struct xnone *tmp;
	int r;

	if ((tmp = xnone_new()) == NULL)
		return -1;
	if ((r = xdict_insert_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_replace_s(struct xdict *dict, const char *key, struct xobject *value)
{
	struct xstring *tmp;
	int r;

	if ((tmp = xstring_new(key)) == NULL)
		return -1;
	if ((r = xdict_replace(dict, tmp, value)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_replace_ss(struct xdict *dict, const char *key, const char *value)
{
	struct xstring *tmp;
	int r;

	if ((tmp = xstring_new(value)) == NULL)
		return -1;
	if ((r = xdict_replace_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_replace_si(struct xdict *dict, const char *key, int64_t value)
{
	struct xint *tmp;
	int r;

	if ((tmp = xint_new(value)) == NULL)
		return -1;
	if ((r = xdict_replace_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_replace_sd(struct xdict *dict, const char *key)
{
	struct xdict *tmp;
	int r;

	if ((tmp = xdict_new()) == NULL)
		return -1;
	if ((r = xdict_replace_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_replace_sa(struct xdict *dict, const char *key)
{
	struct xarray *tmp;
	int r;

	if ((tmp = xarray_new()) == NULL)
		return -1;
	if ((r = xdict_replace_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}

int
xdict_replace_sn(struct xdict *dict, const char *key)
{
	struct xnone *tmp;
	int r;

	if ((tmp = xnone_new()) == NULL)
		return -1;
	if ((r = xdict_replace_s(dict, key, (struct xobject *)tmp)) == -1)
		xobject_free((struct xobject *)tmp);
	return r;
}
