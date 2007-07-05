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

#include <sys/types.h>
#include <stddef.h>

#include "mobject.h"

/* XXX: this is all mechanical, move most of it to #defines */

int
marray_append_s(struct mobject *array, const char *v)
{
	struct mobject *tmp;
	int r;

	if ((tmp = (struct mobject *)mstring_new(v)) == NULL)
		return -1;
	if ((r = marray_append(array, tmp)) == -1)
		mobject_free(tmp);
	return r;
}

int
marray_append_i(struct mobject *array, int64_t v)
{
	struct mobject *tmp;
	int r;

	if ((tmp = (struct mobject *)mint_new(v)) == NULL)
		return -1;
	if ((r = marray_append(array, tmp)) == -1)
		mobject_free(tmp);
	return r;
}

struct mobject *
mdict_item_s(const struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	struct mobject *r;

	if ((tmp = mstring_new(key)) == NULL)
		return NULL;
	r = mdict_item(dict, tmp);
	mobject_free((struct mobject *)tmp);
	return r;
}

struct mobject *
mdict_remove_s(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	struct mobject *r;

	if ((tmp = mstring_new(key)) == NULL)
		return NULL;
	r = mdict_remove(dict, tmp);
	mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_delete_s(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mstring_new(key)) == NULL)
		return -1;
	r = mdict_delete(dict, tmp);
	mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_insert_s(struct mobject *dict, const char *key, struct mobject *value)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mstring_new(key)) == NULL)
		return -1;
	if ((r = mdict_insert(dict, tmp, value)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_insert_ss(struct mobject *dict, const char *key, const char *value)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mstring_new(value)) == NULL)
		return -1;
	if ((r = mdict_insert_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int 
mdict_insert_si(struct mobject *dict, const char *key, int64_t value)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mint_new(value)) == NULL)
		return -1;
	if ((r = mdict_insert_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_insert_sd(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mdict_new()) == NULL)
		return -1;
	if ((r = mdict_insert_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_insert_sa(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	int r;

	if ((tmp = marray_new()) == NULL)
		return -1;
	if ((r = mdict_insert_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_insert_sn(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mnone_new()) == NULL)
		return -1;
	if ((r = mdict_insert_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_replace_s(struct mobject *dict, const char *key, struct mobject *value)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mstring_new(key)) == NULL)
		return -1;
	if ((r = mdict_replace(dict, tmp, value)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_replace_ss(struct mobject *dict, const char *key, const char *value)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mstring_new(value)) == NULL)
		return -1;
	if ((r = mdict_replace_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_replace_si(struct mobject *dict, const char *key, int64_t value)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mint_new(value)) == NULL)
		return -1;
	if ((r = mdict_replace_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_replace_sd(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mdict_new()) == NULL)
		return -1;
	if ((r = mdict_replace_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_replace_sa(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	int r;

	if ((tmp = marray_new()) == NULL)
		return -1;
	if ((r = mdict_replace_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}

int
mdict_replace_sn(struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	int r;

	if ((tmp = mnone_new()) == NULL)
		return -1;
	if ((r = mdict_replace_s(dict, key, (struct mobject *)tmp)) == -1)
		mobject_free((struct mobject *)tmp);
	return r;
}
