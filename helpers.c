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

#include <sys/types.h>
#include <stddef.h>

#include "mobject.h"

/* XXX: this is all mechanical, move most of it to #defines */

struct mobject *
marray_append_s(struct mobject *array, const char *v)
{
	struct mobject *tmp;

	if ((tmp = mstring_new(v)) == NULL)
		return NULL;
	if (marray_append(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_append_d(struct mobject *array)
{
	struct mobject *tmp;

	if ((tmp = mdict_new()) == NULL)
		return NULL;
	if (marray_append(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_append_a(struct mobject *array)
{
	struct mobject *tmp;

	if ((tmp = marray_new()) == NULL)
		return NULL;
	if (marray_append(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_append_n(struct mobject *array)
{
	struct mobject *tmp;

	if ((tmp = mnone_new()) == NULL)
		return NULL;
	if (marray_append(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_append_i(struct mobject *array, int64_t v)
{
	struct mobject *tmp;

	if ((tmp = mint_new(v)) == NULL)
		return NULL;
	if (marray_append(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_prepend_s(struct mobject *array, const char *v)
{
	struct mobject *tmp;

	if ((tmp = mstring_new(v)) == NULL)
		return NULL;
	if (marray_prepend(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_prepend_d(struct mobject *array)
{
	struct mobject *tmp;

	if ((tmp = mdict_new()) == NULL)
		return NULL;
	if (marray_prepend(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_prepend_a(struct mobject *array)
{
	struct mobject *tmp;

	if ((tmp = marray_new()) == NULL)
		return NULL;
	if (marray_prepend(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_prepend_n(struct mobject *array)
{
	struct mobject *tmp;

	if ((tmp = mnone_new()) == NULL)
		return NULL;
	if (marray_prepend(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_prepend_i(struct mobject *array, int64_t v)
{
	struct mobject *tmp;

	if ((tmp = mint_new(v)) == NULL)
		return NULL;
	if (marray_prepend(array, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_item_s(const struct mobject *dict, const char *key)
{
	struct mobject *tmp;
	struct mobject *r;

	if ((tmp = mstring_new(key)) == NULL)
		return NULL;
	r = mdict_item(dict, tmp);
	mobject_free(tmp);
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
	mobject_free(tmp);
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
	mobject_free(tmp);
	return r;
}

struct mobject *
mdict_insert_s(struct mobject *dict, const char *key, struct mobject *value)
{
	struct mobject *tmp;

	if ((tmp = mstring_new(key)) == NULL)
		return NULL;
	if (mdict_insert(dict, tmp, value) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_insert_ss(struct mobject *dict, const char *key, const char *value)
{
	struct mobject *tmp;

	if ((tmp = mstring_new(value)) == NULL)
		return NULL;
	if (mdict_insert_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_insert_si(struct mobject *dict, const char *key, int64_t value)
{
	struct mobject *tmp;

	if ((tmp = mint_new(value)) == NULL)
		return NULL;
	if (mdict_insert_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_insert_sd(struct mobject *dict, const char *key)
{
	struct mobject *tmp;

	if ((tmp = mdict_new()) == NULL)
		return NULL;
	if (mdict_insert_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_insert_sa(struct mobject *dict, const char *key)
{
	struct mobject *tmp;

	if ((tmp = marray_new()) == NULL)
		return NULL;
	if (mdict_insert_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_insert_sn(struct mobject *dict, const char *key)
{
	struct mobject *tmp;

	if ((tmp = mnone_new()) == NULL)
		return NULL;
	if (mdict_insert_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_replace_s(struct mobject *dict, const char *key, struct mobject *value)
{
	struct mobject *tmp;

	if ((tmp = mstring_new(key)) == NULL)
		return NULL;
	if (mdict_replace(dict, tmp, value) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_replace_ss(struct mobject *dict, const char *key, const char *value)
{
	struct mobject *tmp;

	if ((tmp = mstring_new(value)) == NULL)
		return NULL;
	if (mdict_replace_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_replace_si(struct mobject *dict, const char *key, int64_t value)
{
	struct mobject *tmp;

	if ((tmp = mint_new(value)) == NULL)
		return NULL;
	if (mdict_replace_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_replace_sd(struct mobject *dict, const char *key)
{
	struct mobject *tmp;

	if ((tmp = mdict_new()) == NULL)
		return NULL;
	if (mdict_replace_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_replace_sa(struct mobject *dict, const char *key)
{
	struct mobject *tmp;

	if ((tmp = marray_new()) == NULL)
		return NULL;
	if (mdict_replace_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
mdict_replace_sn(struct mobject *dict, const char *key)
{
	struct mobject *tmp;

	if ((tmp = mnone_new()) == NULL)
		return NULL;
	if (mdict_replace_s(dict, key, tmp) == NULL) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_set_s(struct mobject *array, size_t ndx, char *s)
{
	struct mobject *tmp;

	if ((tmp = mstring_new(s)) == NULL)
		return NULL;
	if (marray_set(array, ndx, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_set_i(struct mobject *array, size_t ndx, int64_t i)
{
	struct mobject *tmp;

	if ((tmp = mint_new(i)) == NULL)
		return NULL;
	if (marray_set(array, ndx, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_set_d(struct mobject *array, size_t ndx)
{
	struct mobject *tmp;

	if ((tmp = mdict_new()) == NULL)
		return NULL;
	if (marray_set(array, ndx, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_set_a(struct mobject *array, size_t ndx)
{
	struct mobject *tmp;

	if ((tmp = marray_new()) == NULL)
		return NULL;
	if (marray_set(array, ndx, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

struct mobject *
marray_set_n(struct mobject *array, size_t ndx)
{
	struct mobject *tmp;

	if ((tmp = mnone_new()) == NULL)
		return NULL;
	if (marray_set(array, ndx, tmp) == -1) {
		mobject_free(tmp);
		return NULL;
	}
	return tmp;
}

