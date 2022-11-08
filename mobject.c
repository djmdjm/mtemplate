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

/* Simple generic object system, loosely modelled on Python's */

#include <sys/types.h>
#include <sys/param.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vis.h>

#include "sys-queue.h"
#include "compat.h"
#include "mobject.h"

/*
 * Maximum number of entries in an array. NB. be careful - 
 * (MARRAY_MAX + 1 * sizeof(struct mobject *)) * 2 must be less than
 * SIZE_T_MAX to avoid int overflow
 */
#define MARRAY_MAX	(128 * 1024 * 1024)

/* Maximum length of a string */
#define MSTRING_MAX	(256 * 1024 * 1024)

/* **** Private types **** */

/* Generic stub */
struct mobject {
	enum mobject_type type;
};

/* "None" type */
struct mnone {
	enum mobject_type type; /* TYPE_MNONE */
};

/* String (bytes + len) type */
struct mstring {
	enum mobject_type type; /* TYPE_MSTRING */
	u_char *value;
	size_t len;
};

/* Integer (signed, 64 bit) type */
struct mint {
	enum mobject_type type; /* TYPE_MINT */
	int64_t value;
};

/* Array type */
struct marray {
	enum mobject_type type; /* TYPE_MARRAY */
	struct mobject **entries;
	size_t nalloc;
	size_t nused;
};
/* XXX: better data structure for sparse arrays here */

/* Dictionary entry (not user visible) */
struct mdict_entry {
	struct mobject *key;
	struct mobject *value;
	TAILQ_ENTRY(mdict_entry) entry;
};
TAILQ_HEAD(mdict_entries, mdict_entry);
/* XXX: better data structure for dicts here (hash or RB tree) */

/* Dictionary type */
struct mdict {
	enum mobject_type type; /* TYPE_MDICT */
	size_t num_entries;
	struct mdict_entries entries;
};

/* Generic iterator */
struct miterator {
	struct mobject *object;
	u_int started;
	struct miteritem iteritem;
	size_t array_ndx;		/* Only valid for TYPE_MARRAY */
	struct mobject *array_last_key;	/* Only valid for TYPE_MARRAY */
	struct mdict_entry *dict_ptr;	/* Only valid for TYPE_MDICT */
};

/* Single instance of mnone */
static struct mnone *mnone_singleton = NULL;

struct mobject *
mnone_new(void)
{
	/* Keep a single instance of mnone */
	/* XXX: memprotect it */
	if (mnone_singleton != NULL)
		return (struct mobject *)mnone_singleton;
	if ((mnone_singleton = calloc(1, sizeof(*mnone_singleton))) == NULL)
		return NULL;
	mnone_singleton->type = TYPE_MNONE;
	return (struct mobject *)mnone_singleton;
}

struct mobject *
mint_new(int64_t v)
{
	struct mint *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_MINT;
	ret->value = v;
	return (struct mobject *)ret;
}

struct mobject *
mstring_new2(const u_int8_t *value, size_t len)
{
	struct mstring *ret;

	if (len > MSTRING_MAX - 1)
		return NULL;
	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_MSTRING;
	/*
	 * Hack: allocate one extra byte and zero it so nul-terminated
	 * string remain so
	 */
	if ((ret->value = malloc(len + 1)) == NULL) {
		free(ret);
		return NULL;
	}
	memcpy(ret->value, value, len);
	ret->value[len] = '\0';
	ret->len = len;
	return (struct mobject *)ret;
}

struct mobject *
mstring_new(const char *value)
{
	return mstring_new2((u_int8_t *)value, strlen(value));
}

struct mobject *
marray_new(void)
{
	struct marray *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_MARRAY;
	ret->entries = 0;
	return (struct mobject *)ret;
}

struct mobject *
mdict_new(void)
{
	struct mdict *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_MDICT;
	TAILQ_INIT(&ret->entries);
	return (struct mobject *)ret;
}

enum mobject_type
mobject_type(const struct mobject *obj)
{
	return obj->type;
}

static void
mnone_free(const struct mnone *o)
{
	/* Do nothing - mnone is a singleton */
	(void)o;
}

static void
mstring_free(struct mstring *o)
{
	if (o->value != NULL && o->len > 0) {
		bzero(o->value, o->len);
		free(o->value);
	}
	bzero(o, sizeof(*o));
	free(o);
}

static void
mint_free(struct mint *o)
{
	bzero(o, sizeof(*o));
	free(o);
}

static void
marray_free(struct marray *o)
{
	size_t i;
	struct mobject *oi;

	for (i = 0; i < o->nused; i++) {
		oi = o->entries[i];
		if (oi == NULL)
			continue;
		o->entries[i] = NULL;
		mobject_free(oi);
	}
	free(o->entries);
	bzero(o, sizeof(*o));
	free(o);
}

static void
mdict_free(struct mdict *o)
{
	struct mdict_entry *oe;

	while ((oe = TAILQ_FIRST(&o->entries)) != NULL) {
		TAILQ_REMOVE(&o->entries, oe, entry);
		mobject_free(oe->key);
		mobject_free(oe->value);
		bzero(oe, sizeof(*oe));
		free(oe);
	}
	bzero(o, sizeof(*o));
	free(o);
}

void
mobject_free(struct mobject *o)
{
	switch (o->type) {
	case TYPE_MNONE:
		mnone_free((struct mnone *)o);
		break;
	case TYPE_MSTRING:
		mstring_free((struct mstring *)o);
		break;
	case TYPE_MINT:
		mint_free((struct mint *)o);
		break;
	case TYPE_MARRAY:
		marray_free((struct marray *)o);
		break;
	case TYPE_MDICT:
		mdict_free((struct mdict *)o);
		break;
	}
}

static size_t
mstring_to_string(const struct mstring *o, char *s, size_t len)
{
	char vbuf[5], *ve;
	size_t i, j, l, done = 0;

	/* This is basically strnvis + strvisx */
	for (i = j = 0; i < o->len; i++) {
		ve = vis(vbuf, o->value[i], VIS_OCTAL,
		    (i + 1) < o->len ? 0 : o->value[i + 1]);
		l = ve - vbuf;
		if (!done && j + l + 1 <= len)
			memcpy(s + j, vbuf, l + 1);
		else
			done = 1;
		/* Avoid integer wrap */
		if (j + l >= j)
			j += l;
	}
	return j;
}

size_t
mobject_to_string(const struct mobject *o, char *s, size_t len)
{
	switch (o->type) {
	case TYPE_MNONE:
		return strlcpy(s, "None", len);
	case TYPE_MSTRING:
		return mstring_to_string((struct mstring *)o, s, len);
	case TYPE_MINT:
		return snprintf(s, len, "%lld",
		    (unsigned long long)((struct mint *)o)->value);
	case TYPE_MARRAY:
		return snprintf(s, len, "marray(%p, %llu)", o,
		    (unsigned long long)((struct marray *)o)->nused);
	case TYPE_MDICT:
		return snprintf(s, len, "mdict(%p)", o);
	default:
		return strlcpy(s, "Unsupported object type %d", o->type);
	}
}

struct mobject *
mobject_deepcopy(struct mobject *o)
{
	struct mobject *new_obj;
	struct mobject *k, *v;
	struct miterator *iter;
	struct miteritem *item;
	size_t n;

	switch (o->type) {
	case TYPE_MNONE:
		return mnone_new();
	case TYPE_MSTRING:
		return mstring_new2(mstring_ptr(o), mstring_len(o));
	case TYPE_MINT:
		return mint_new(mint_value(o));
	case TYPE_MARRAY:
		if ((new_obj = marray_new()) == NULL)
			return NULL;
		for (n = 0; n < marray_len(o); n++) {
			if ((v = mobject_deepcopy(marray_item(o, n))) == NULL) {
				mobject_free(new_obj);
				return NULL;
			}
			marray_set(new_obj, n, v);
		}
		return new_obj;
	case TYPE_MDICT:
		if ((new_obj = mdict_new()) == NULL)
			return NULL;
		if ((iter = mobject_getiter(o)) == NULL) {
			mobject_free(new_obj);
			return NULL;
		}
		while ((item = miterator_next(iter)) != NULL) {
			if ((k = mobject_deepcopy(item->key)) == NULL) {
 mdict_deepcopy_err:
				mobject_free(new_obj);
				miterator_free(iter);
				return NULL;
			}
			if ((v = mobject_deepcopy(item->value)) == NULL) {
				mobject_free(k);
				goto mdict_deepcopy_err;
			}
			if (mdict_insert(new_obj, k, v) != 0) {
				mobject_free(k);
				mobject_free(v);
				goto mdict_deepcopy_err;
			}
		}
		miterator_free(iter);
		return new_obj;
	default:
		return NULL;
	}
}

int64_t
mint_value(const struct mobject *_v)
{
	struct mint *v = (struct mint *)_v;

	if (v->type != TYPE_MINT)
		return 0;
	return v->value;
}

int
mint_add(struct mobject *_v, int64_t n)
{
	struct mint *v = (struct mint *)_v;

	if (v->type != TYPE_MINT)
		return -1;
	v->value += n;
	return 0;
}

size_t
mstring_len(const struct mobject *_s)
{
	struct mstring *s = (struct mstring *)_s;

	if (s->type != TYPE_MSTRING)
		return 0;
	return s->len;
}

const u_char *
mstring_ptr(const struct mobject *_s)
{
	struct mstring *s = (struct mstring *)_s;

	if (s->type != TYPE_MSTRING)
		return NULL;
	return s->value;
}

static int
marray_resize(struct marray *array, size_t want)
{
	struct mobject **tmp;
	size_t n, i;

	if (want < array->nalloc)
		return 0;
	/*
	 * Allocate the next power of two up.
	 * NB. MARRAY_MAX must be sized to avoid integer overflow
	 */
	for (n = MAX(array->nalloc, 4); n < MARRAY_MAX && n <= want; n <<= 1)
		;
	if (n >= MARRAY_MAX || n <= array->nused || n <= want)
		return -1;	
	if ((tmp = realloc(array->entries, n * sizeof(*array->entries))) == NULL)
		return -1;
	array->entries = tmp;
	array->nalloc = n;
	/* Zero just-allocated entries */
	for (i = array->nused; i < array->nalloc; i++)
		array->entries[i] = NULL;
	return 0;
}

int
marray_prepend(struct mobject *_array, struct mobject *object)
{
	struct marray *array = (struct marray *)_array;

	if (array->type != TYPE_MARRAY)
		return -1;
	if (marray_resize(array, array->nused + 1) == -1)
		return -1;
	memmove(array->entries + 1, array->entries,
	    sizeof(*array->entries) * array->nused);
	array->entries[0] = object;
	array->nused++;
	return 0;
}

int
marray_append(struct mobject *_array, struct mobject *object)
{
	struct marray *array = (struct marray *)_array;

	if (array->type != TYPE_MARRAY)
		return -1;
	if (marray_resize(array, array->nused + 1) == -1)
		return -1;
	array->entries[array->nused++] = object;
	return 0;
}

int
marray_set(struct mobject *_array, size_t ndx, struct mobject *object)
{
	struct marray *array = (struct marray *)_array;
	size_t i;

	if (array->type != TYPE_MARRAY)
		return -1;
	if (ndx >= MARRAY_MAX)
		return -1;
	if (marray_resize(array, ndx + 1) == -1)
		return -1;
	/* Fill unallocated entries with None */
	for (i = array->nused; i < ndx; i++)
		array->entries[i] = (struct mobject *)mnone_new();
	if (array->entries[ndx] != NULL)
		mobject_free(array->entries[ndx]);
	array->entries[ndx] = object;
	array->nused = MAX(array->nused, ndx + 1);
	return 0;
}

size_t
marray_len(struct mobject *_array)
{
	struct marray *array = (struct marray *)_array;

	if (array->type != TYPE_MARRAY)
		return 0;
	return array->nused;
}

struct mobject *
marray_last(struct mobject *_array)
{
	struct marray *array = (struct marray *)_array;

	if (array->type != TYPE_MARRAY)
		return NULL;
	return array->nused == 0 ? NULL : array->entries[array->nused - 1];
}

struct mobject *
marray_first(struct mobject *_array)
{
	struct marray *array = (struct marray *)_array;

	if (array->type != TYPE_MARRAY)
		return NULL;
	return array->nused == 0 ? NULL : array->entries[0];
}

struct mobject *
marray_pop(struct mobject *_array)
{
	struct marray *array = (struct marray *)_array;
	struct mobject *ret;

	if (array->type != TYPE_MARRAY)
		return NULL;
	if (array->nused == 0)
		return NULL;
	ret = array->entries[--array->nused];
	array->entries[array->nused] = NULL;
	return ret;
}

struct mobject *
marray_pull(struct mobject *_array)
{
	struct marray *array = (struct marray *)_array;
	struct mobject *ret;

	if (array->type != TYPE_MARRAY)
		return NULL;
	if (array->nused == 0)
		return NULL;
	ret = array->entries[0];
	array->nused--;
	if (array->nused == 0)
		array->entries[0] = NULL;
	else {
		memmove(array->entries, array->entries + 1,
		    sizeof(*array->entries) * array->nused);
	}
	return ret;
}

struct mobject *
marray_item(struct mobject *_array, size_t ndx)
{
	struct marray *array = (struct marray *)_array;

	if (array->type != TYPE_MARRAY)
		return NULL;
	if (ndx >= array->nused)
		return NULL;
	return array->entries[ndx];
}

static int
mobject_cmp_byaddr(const struct mobject *a, const struct mobject *b)
{
	if (a == b)
		return 0;
	return a < b ? -1 : 1;
}

static int
marray_cmp(const struct mobject *_a, const struct mobject *_b)
{
	struct marray *a = (struct marray *)_a;
	struct marray *b = (struct marray *)_b;
	size_t i;
	int r;

	if (a->nused != b->nused)
		return a->nused < b->nused ? -1 : 1;
	for (i = 0; i < a->nused; i++) {
		if (a->entries[i] == NULL) {
			if (b->entries[i] == NULL)
				continue;
			return -1;
		}
		if (b->entries[i] == NULL)
			return 1;
		if ((r = mobject_cmp(a->entries[i], b->entries[i])) != 0)
			return r;
	}
	return 0;
}

static int
mint_cmp(const struct mobject *_a, const struct mobject *_b)
{
	struct mint *a = (struct mint *)_a;
	struct mint *b = (struct mint *)_b;

	if (a->value == b->value)
		return 0;
	return a->value < b->value ? -1 : 1;
}

static int
mstring_cmp(const struct mobject *_a, const struct mobject *_b)
{
	struct mstring *a = (struct mstring *)_a;
	struct mstring *b = (struct mstring *)_b;
	int r;

	if (a->len == 0 && b->len == 0)
		return 0;
	if (a->len == 0)
		return -1;
	if (b->len == 0)
		return 1;
	if ((r = memcmp(a->value, b->value, MIN(a->len, b->len))) != 0)
		return r < 0 ? -1 : (r > 0 ? 1 : 0);
	if (a->len > b->len)
		return 1;
	if (b->len > a->len)
		return -1;
	return 0;
}

int
mobject_cmp(const struct mobject *a, const struct mobject *b)
{
	if (a == b)
		return 0;
	if (a->type == b->type) {
		switch (a->type) {
		case TYPE_MNONE:
			return 0;
		case TYPE_MINT:
			return mint_cmp(a, b);
		case TYPE_MSTRING:
			return mstring_cmp(a, b);
		case TYPE_MARRAY:
			return marray_cmp(a, b);
		case TYPE_MDICT:
			return mobject_cmp_byaddr(a, b);
		default:
			return 0;
		}
	}
	return a->type < b->type ? -1 : 1;
}

struct mobject *
mdict_item(const struct mobject *_dict, const struct mobject *key)
{
	struct mdict *dict = (struct mdict *)_dict;
	struct mdict_entry *e;

	if (dict->type != TYPE_MDICT || key->type != TYPE_MSTRING)
		return NULL;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (mstring_cmp(e->key, key) == 0)
			return e->value;
	}
	return NULL;
}

struct mobject *
mdict_remove(struct mobject *_dict, const struct mobject *key)
{
	struct mdict *dict = (struct mdict *)_dict;
	struct mdict_entry *e;
	struct mobject *ret;

	if (dict->type != TYPE_MDICT || key->type != TYPE_MSTRING)
		return NULL;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (mstring_cmp(e->key, key) == 0) {
			TAILQ_REMOVE(&dict->entries, e, entry);
			ret = e->value;
			mobject_free((struct mobject *)e->key);
			bzero(e, sizeof(*e));
			free(e);
			dict->num_entries--;
			return ret;
		}
	}
	return NULL;
}

int
mdict_delete(struct mobject *_dict, const struct mobject *key)
{
	struct mdict *dict = (struct mdict *)_dict;
	struct mobject *o;

	if (dict->type != TYPE_MDICT || key->type != TYPE_MSTRING)
		return -1;
	if ((o = mdict_remove(_dict, key)) == NULL)
		return -1;
	mobject_free(o);
	dict->num_entries--;
	return 0;
}

int
mdict_insert(struct mobject *_dict, struct mobject *key,
    struct mobject *value)
{
	struct mdict *dict = (struct mdict *)_dict;
	struct mdict_entry *e;

	if (dict->type != TYPE_MDICT || key->type != TYPE_MSTRING)
		return -1;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (mstring_cmp(e->key, key) == 0)
			return -1;
	}
	if ((e = calloc(1, sizeof(*e))) == NULL)
		return -1;
	e->key = key;
	e->value = value;
	TAILQ_INSERT_TAIL(&dict->entries, e, entry);
	dict->num_entries++;
	return 0;
}

int
mdict_replace(struct mobject *_dict, struct mobject *key,
    struct mobject *value)
{
	struct mdict *dict = (struct mdict *)_dict;
	struct mdict_entry *e;

	if (dict->type != TYPE_MDICT || key->type != TYPE_MSTRING)
		return -1;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (mstring_cmp(e->key, key) == 0)
			break;
	}
	if (e != TAILQ_END(&dict->entries)) {
		TAILQ_REMOVE(&dict->entries, e, entry);
		mobject_free((struct mobject *)e->key);
		mobject_free(e->value);
		dict->num_entries--;
	} else {
		if ((e = calloc(1, sizeof(*e))) == NULL)
			return -1;
	}
	e->key = key;
	e->value = value;
	TAILQ_INSERT_TAIL(&dict->entries, e, entry);
	dict->num_entries++;
	return 0;
}

size_t
mdict_len(const struct mobject *_dict)
{
	struct mdict *dict = (struct mdict *)_dict;

	if (dict->type != TYPE_MDICT)
		return 0;
	return dict->num_entries;
}

struct miterator *
mobject_getiter(struct mobject *obj)
{
	struct miterator *ret;

	switch (obj->type) {
	case TYPE_MARRAY:
	case TYPE_MDICT:
		break;
	default:
		return NULL;
	}
	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->started = 0;
	ret->object = obj;
	return ret;
}

void
miterator_free(struct miterator *iter)
{
	if (iter->started && iter->object &&
	    iter->object->type == TYPE_MARRAY &&
	    iter->array_last_key != NULL)
		mobject_free(iter->array_last_key);
	bzero(iter, sizeof(*iter));
	free(iter);
}

static struct miteritem *
miterator_next_array(struct miterator *iter)
{
	struct marray *array = (struct marray *)(iter->object);
	struct mobject *key;

	if (!iter->started) {
		iter->array_ndx = 0;
		iter->array_last_key = NULL;
		iter->started = 1;
	}
	if (iter->array_ndx >= array->nused)
		return NULL;
	bzero(&iter->iteritem, sizeof(iter->iteritem));
	if ((key = (struct mobject *)mint_new(iter->array_ndx)) == NULL)
		return NULL;
	if (iter->array_last_key != NULL)
		mobject_free(iter->array_last_key);
	iter->array_last_key = key;
	iter->iteritem.key = key;
	iter->iteritem.value = array->entries[iter->array_ndx++];
	return &iter->iteritem;
}

static struct miteritem *
miterator_next_dict(struct miterator *iter)
{
	struct mdict *dict = (struct mdict *)(iter->object);
	
	if (!iter->started) {
		iter->dict_ptr = TAILQ_FIRST(&dict->entries);
		iter->started = 1;
	}
	if (iter->dict_ptr == TAILQ_END(&dict->entries))
		return NULL;
	bzero(&iter->iteritem, sizeof(iter->iteritem));
	iter->iteritem.key = (struct mobject *)iter->dict_ptr->key;
	iter->iteritem.value = iter->dict_ptr->value;
	iter->dict_ptr = TAILQ_NEXT(iter->dict_ptr, entry);
	return &iter->iteritem;
}


struct miteritem *
miterator_next(struct miterator *iter)
{
	switch (iter->object->type) {
	case TYPE_MARRAY:
		return miterator_next_array(iter);
	case TYPE_MDICT:
		return miterator_next_dict(iter);
	default:
		return NULL;
	}
}
