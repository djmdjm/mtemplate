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

/* Simple generic object system, loosely modelled on Python's */

#include <sys/types.h>
#include <sys/param.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vis.h>

#include "sys-queue.h"
#include "strlcpy.h"
#include "xobject.h"

/* unbelievable that some systems lack this */
#ifndef SIZE_T_MAX
#define SIZE_T_MAX UINT_MAX
#endif /* SIZE_T_MAX */

/*
 * Maximum number of entries in an array. NB. be careful - 
 * (XARRAY_MAX + 1 * sizeof(struct xobject *)) * 2 must be less than
 * SIZE_T_MAX to avoid int overflow
 */
#define XARRAY_MAX	(1024 * 1024)

/* **** Private types **** */

/* Generic stub */
struct xobject {
	enum xobject_type type;
};

/* "None" type */
struct xnone {
	enum xobject_type type; /* TYPE_XNONE */
};

/* String (bytes + len) type */
struct xstring {
	enum xobject_type type; /* TYPE_XSTRING */
	u_char *value;
	size_t len;
};

/* Integer (signed, 64 bit) type */
struct xint {
	enum xobject_type type; /* TYPE_XINT */
	int64_t value;
};

/* Array type */
struct xarray {
	enum xobject_type type; /* TYPE_XARRAY */
	struct xobject **entries;
	size_t nalloc;
	size_t nused;
};
/* XXX: better data structure for sparse arrays here */

/* Dictionary entry (not user visible) */
struct xdict_entry {
	struct xobject *key;
	struct xobject *value;
	TAILQ_ENTRY(xdict_entry) entry;
};
TAILQ_HEAD(xdict_entries, xdict_entry);
/* XXX: better data structure for dicts here (hash or RB tree) */

/* Dictionary type */
struct xdict {
	enum xobject_type type; /* TYPE_XDICT */
	size_t num_entries;
	struct xdict_entries entries;
};

/* Generic iterator */
struct xiterator {
	struct xobject *object;
	u_int started;
	struct xiteritem iteritem;
	size_t array_ndx;		/* Only valid for TYPE_XARRAY */
	struct xobject *array_last_key;	/* Only valid for TYPE_XARRAY */
	struct xdict_entry *dict_ptr;	/* Only valid for TYPE_XDICT */
};

/* Single instance of xnone */
static struct xnone *xnone_singleton = NULL;

struct xobject *
xnone_new(void)
{
	/* Keep a single instance of xnone */
	/* XXX: memprotect it */
	if (xnone_singleton != NULL)
		return (struct xobject *)xnone_singleton;
	if ((xnone_singleton = calloc(1, sizeof(*xnone_singleton))) == NULL)
		return NULL;
	xnone_singleton->type = TYPE_XNONE;
	return (struct xobject *)xnone_singleton;
}

struct xobject *
xint_new(int64_t v)
{
	struct xint *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_XINT;
	ret->value = v;
	return (struct xobject *)ret;
}

struct xobject *
xstring_new2(const u_char *value, size_t len)
{
	struct xstring *ret;

	if (len > SIZE_T_MAX - 1)
		return NULL;
	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_XSTRING;
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
	return (struct xobject *)ret;
}

struct xobject *
xstring_new(const u_char *value)
{
	return (struct xobject *)xstring_new2(value, strlen(value));
}

struct xobject *
xarray_new(void)
{
	struct xarray *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_XARRAY;
	ret->entries = 0;
	return (struct xobject *)ret;
}

struct xobject *
xdict_new(void)
{
	struct xdict *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_XDICT;
	TAILQ_INIT(&ret->entries);
	return (struct xobject *)ret;
}

enum xobject_type
xobject_type(const struct xobject *obj)
{
	return obj->type;
}

static void
xnone_free(const struct xnone *o)
{
	/* Do nothing - xnone is a singleton */
}

static void
xstring_free(struct xstring *o)
{
	if (o->value != NULL && o->len > 0) {
		bzero(o->value, o->len);
		free(o->value);
	}
	bzero(o, sizeof(*o));
	free(o);
}

static void
xint_free(struct xint *o)
{
	bzero(o, sizeof(*o));
	free(o);
}

static void
xarray_free(struct xarray *o)
{
	size_t i;
	struct xobject *oi;

	for (i = 0; i < o->nused; i++) {
		oi = o->entries[i];
		if (oi == NULL)
			continue;
		o->entries[i] = NULL;
		xobject_free(oi);
	}
	free(o->entries);
	bzero(o, sizeof(*o));
	free(o);
}

static void
xdict_free(struct xdict *o)
{
	struct xdict_entry *oe;

	while ((oe = TAILQ_FIRST(&o->entries)) != NULL) {
		TAILQ_REMOVE(&o->entries, oe, entry);
		xobject_free(oe->key);
		xobject_free(oe->value);
		bzero(oe, sizeof(*oe));
		free(oe);
	}
	bzero(o, sizeof(*o));
	free(o);
}

void
xobject_free(struct xobject *o)
{
	switch (o->type) {
	case TYPE_XNONE:
		xnone_free((struct xnone *)o);
		break;
	case TYPE_XSTRING:
		xstring_free((struct xstring *)o);
		break;
	case TYPE_XINT:
		xint_free((struct xint *)o);
		break;
	case TYPE_XARRAY:
		xarray_free((struct xarray *)o);
		break;
	case TYPE_XDICT:
		xdict_free((struct xdict *)o);
		break;
	}
}

static size_t
xstring_to_string(const struct xstring *o, u_char *s, size_t len)
{
	char vbuf[5], *ve;
	size_t i, j, l, done = 0;

	/* This is basically strnvis + strvisx */
	for (i = j = 0; i < o->len; i++) {
		ve = vis(vbuf, o->value[i], VIS_OCTAL,
		    (i + 1) < o->len ? 0 : o->value[i + 1]);
		l = ve - vbuf;
		if (!done && j + l + 1 <= len && j < SIZE_T_MAX - l - 1)
			memcpy(s + j, vbuf, l + 1);
		else
			done = 1;
		/* Avoid integer wrap */
		if (j >= SIZE_T_MAX - l - 1)
			j = SIZE_T_MAX - 1;
		else
			j += l;
	}
	return j;
}

size_t
xobject_to_string(const struct xobject *o, u_char *s, size_t len)
{
	switch (o->type) {
	case TYPE_XNONE:
		return strlcpy(s, "None", len);
	case TYPE_XSTRING:
		return xstring_to_string((struct xstring *)o, s, len);
	case TYPE_XINT:
		return snprintf(s, len, "%lld", ((struct xint *)o)->value);
	case TYPE_XARRAY:
		return snprintf(s, len, "xarray(%p, %llu)", o,
		    (unsigned long long)((struct xarray *)o)->nused);
	case TYPE_XDICT:
		return snprintf(s, len, "xdict(%p)", o);
	default:
		return strlcpy(s, "Unsupported object type %d", o->type);
	}
}

struct xobject *
xobject_deepcopy(struct xobject *o)
{
	struct xobject *new_obj;
	struct xobject *k, *v;
	struct xiterator *iter;
	struct xiteritem *item;
	size_t n;

	switch (o->type) {
	case TYPE_XNONE:
		return xnone_new();
	case TYPE_XSTRING:
		return xstring_new2(xstring_ptr(o), xstring_len(o));
	case TYPE_XINT:
		return xint_new(xint_value(o));
	case TYPE_XARRAY:
		if ((new_obj = xarray_new()) == NULL)
			return NULL;
		for (n = 0; n < xarray_len(o); n++) {
			if ((v = xobject_deepcopy(xarray_item(o, n))) == NULL) {
				xobject_free(new_obj);
				return NULL;
			}
			xarray_set(new_obj, n, v);
		}
		return new_obj;
	case TYPE_XDICT:
		if ((new_obj = xdict_new()) == NULL)
			return NULL;
		if ((iter = xobject_getiter(o)) == NULL) {
			xobject_free(new_obj);
			return NULL;
		}
		while ((item = xiterator_next(iter)) != NULL) {
			if ((k = xobject_deepcopy(item->key)) == NULL) {
 xdict_deepcopy_err:
				xobject_free(new_obj);
				xiterator_free(iter);
				return NULL;
			}
			if ((v = xobject_deepcopy(item->value)) == NULL) {
				xobject_free(k);
				goto xdict_deepcopy_err;
			}
			if (xdict_insert(new_obj, k, v) != 0) {
				xobject_free(k);
				xobject_free(v);
				goto xdict_deepcopy_err;
			}
		}
		return new_obj;
	default:
		return NULL;
	}
}

int64_t
xint_value(const struct xobject *_v)
{
	struct xint *v = (struct xint *)_v;

	if (v->type != TYPE_XINT)
		return 0;
	return v->value;
}

int
xint_add(struct xobject *_v, int64_t n)
{
	struct xint *v = (struct xint *)_v;

	if (v->type != TYPE_XINT)
		return -1;
	v->value += n;
	return 0;
}

size_t
xstring_len(const struct xobject *_s)
{
	struct xstring *s = (struct xstring *)_s;

	if (s->type != TYPE_XSTRING)
		return 0;
	return s->len;
}

const u_char *
xstring_ptr(const struct xobject *_s)
{
	struct xstring *s = (struct xstring *)_s;

	if (s->type != TYPE_XSTRING)
		return NULL;
	return s->value;
}

static int
xarray_resize(struct xarray *array, size_t want)
{
	struct xobject **tmp;
	size_t n, i;

	if (want < array->nalloc)
		return 0;
	/*
	 * Allocate the next power of two up.
	 * NB. XARRAY_MAX must be sized to avoid integer overflow
	 */
	for (n = MAX(array->nalloc, 4); n < XARRAY_MAX && n <= want; n <<= 1)
		;
	if (n >= XARRAY_MAX || n <= array->nused || n <= want)
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
xarray_prepend(struct xobject *_array, struct xobject *object)
{
	struct xarray *array = (struct xarray *)_array;

	if (array->type != TYPE_XARRAY)
		return -1;
	if (xarray_resize(array, array->nused + 1) == -1)
		return -1;
	memmove(array->entries + 1, array->entries,
	    sizeof(*array->entries) * array->nused);
	array->entries[0] = object;
	array->nused++;
	return 0;
}

int
xarray_append(struct xobject *_array, struct xobject *object)
{
	struct xarray *array = (struct xarray *)_array;

	if (array->type != TYPE_XARRAY)
		return -1;
	if (xarray_resize(array, array->nused + 1) == -1)
		return -1;
	array->entries[array->nused++] = object;
	return 0;
}

int
xarray_set(struct xobject *_array, size_t ndx, struct xobject *object)
{
	struct xarray *array = (struct xarray *)_array;
	size_t i;

	if (array->type != TYPE_XARRAY)
		return -1;
	if (ndx >= XARRAY_MAX)
		return -1;
	if (xarray_resize(array, ndx + 1) == -1)
		return -1;
	/* Fill unallocated entries with None */
	for (i = array->nused; i < ndx; i++)
		array->entries[i] = (struct xobject *)xnone_new();
	if (array->entries[ndx] != NULL)
		xobject_free(array->entries[ndx]);
	array->entries[ndx] = object;
	array->nused = MAX(array->nused, ndx + 1);
	return 0;
}

size_t
xarray_len(struct xobject *_array)
{
	struct xarray *array = (struct xarray *)_array;

	if (array->type != TYPE_XARRAY)
		return 0;
	return array->nused;
}

struct xobject *
xarray_last(struct xobject *_array)
{
	struct xarray *array = (struct xarray *)_array;

	if (array->type != TYPE_XARRAY)
		return NULL;
	return array->nused == 0 ? NULL : array->entries[array->nused - 1];
}

struct xobject *
xarray_first(struct xobject *_array)
{
	struct xarray *array = (struct xarray *)_array;

	if (array->type != TYPE_XARRAY)
		return NULL;
	return array->nused == 0 ? NULL : array->entries[0];
}

struct xobject *
xarray_pop(struct xobject *_array)
{
	struct xarray *array = (struct xarray *)_array;
	struct xobject *ret;

	if (array->type != TYPE_XARRAY)
		return NULL;
	if (array->nused == 0)
		return NULL;
	ret = array->entries[--array->nused];
	array->entries[array->nused] = NULL;
	return ret;
}

struct xobject *
xarray_pull(struct xobject *_array)
{
	struct xarray *array = (struct xarray *)_array;
	struct xobject *ret;

	if (array->type != TYPE_XARRAY)
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

struct xobject *
xarray_item(struct xobject *_array, size_t ndx)
{
	struct xarray *array = (struct xarray *)_array;

	if (array->type != TYPE_XARRAY)
		return NULL;
	if (ndx >= array->nused)
		return NULL;
	return array->entries[ndx];
}

static int
xstring_cmp(const struct xobject *_a, const struct xobject *_b)
{
	struct xstring *a = (struct xstring *)_a;
	struct xstring *b = (struct xstring *)_b;
	int r;

	if (a->len == 0 && b->len == 0)
		return 0;
	if (a->len == 0)
		return -1;
	if (b->len == 0)
		return 1;
	if ((r = memcmp(a->value, b->value, MIN(a->len, b->len))) != 0)
		return r;
	if (a->len > b->len)
		return 1;
	if (b->len > a->len)
		return -1;
	return 0;
}

struct xobject *
xdict_item(const struct xobject *_dict, const struct xobject *key)
{
	struct xdict *dict = (struct xdict *)_dict;
	struct xdict_entry *e;

	if (dict->type != TYPE_XDICT || key->type != TYPE_XSTRING)
		return NULL;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (xstring_cmp(e->key, key) == 0)
			return e->value;
	}
	return NULL;
}

struct xobject *
xdict_remove(struct xobject *_dict, const struct xobject *key)
{
	struct xdict *dict = (struct xdict *)_dict;
	struct xdict_entry *e;
	struct xobject *ret;

	if (dict->type != TYPE_XDICT || key->type != TYPE_XSTRING)
		return NULL;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (xstring_cmp(e->key, key) == 0) {
			TAILQ_REMOVE(&dict->entries, e, entry);
			ret = e->value;
			xobject_free((struct xobject *)e->key);
			bzero(e, sizeof(*e));
			free(e);
			dict->num_entries--;
			return ret;
		}
	}
	return NULL;
}

int
xdict_delete(struct xobject *_dict, const struct xobject *key)
{
	struct xdict *dict = (struct xdict *)_dict;
	struct xobject *o;

	if (dict->type != TYPE_XDICT || key->type != TYPE_XSTRING)
		return -1;
	if ((o = xdict_remove(_dict, key)) == NULL)
		return -1;
	xobject_free(o);
	dict->num_entries--;
	return 0;
}

int
xdict_insert(struct xobject *_dict, struct xobject *key,
    struct xobject *value)
{
	struct xdict *dict = (struct xdict *)_dict;
	struct xdict_entry *e;

	if (dict->type != TYPE_XDICT || key->type != TYPE_XSTRING)
		return -1;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (xstring_cmp(e->key, key) == 0)
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
xdict_replace(struct xobject *_dict, struct xobject *key,
    struct xobject *value)
{
	struct xdict *dict = (struct xdict *)_dict;
	struct xdict_entry *e;

	if (dict->type != TYPE_XDICT || key->type != TYPE_XSTRING)
		return -1;
	TAILQ_FOREACH(e, &dict->entries, entry) {
		if (xstring_cmp(e->key, key) == 0)
			break;
	}
	if (e != TAILQ_END(&dict->entries)) {
		TAILQ_REMOVE(&dict->entries, e, entry);
		xobject_free((struct xobject *)e->key);
		xobject_free(e->value);
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
xdict_len(const struct xobject *_dict)
{
	struct xdict *dict = (struct xdict *)_dict;

	if (dict->type != TYPE_XDICT)
		return 0;
	return dict->num_entries;
}

struct xiterator *
xobject_getiter(struct xobject *obj)
{
	struct xiterator *ret;

	switch (obj->type) {
	case TYPE_XARRAY:
	case TYPE_XDICT:
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
xiterator_free(struct xiterator *iter)
{
	if (iter->started && iter->object &&
	    iter->object->type == TYPE_XARRAY &&
	    iter->array_last_key != NULL)
		xobject_free(iter->array_last_key);
	bzero(iter, sizeof(iter));
	free(iter);
}

static struct xiteritem *
xiterator_next_array(struct xiterator *iter)
{
	struct xarray *array = (struct xarray *)(iter->object);
	struct xobject *key;

	if (!iter->started) {
		iter->array_ndx = 0;
		iter->array_last_key = NULL;
		iter->started = 1;
	}
	if (iter->array_ndx >= array->nused)
		return NULL;
	bzero(&iter->iteritem, sizeof(iter->iteritem));
	if ((key = (struct xobject *)xint_new(iter->array_ndx)) == NULL)
		return NULL;
	if (iter->array_last_key != NULL)
		xobject_free(iter->array_last_key);
	iter->array_last_key = key;
	iter->iteritem.key = key;
	iter->iteritem.value = array->entries[iter->array_ndx++];
	return &iter->iteritem;
}

static struct xiteritem *
xiterator_next_dict(struct xiterator *iter)
{
	struct xdict *dict = (struct xdict *)(iter->object);
	
	if (!iter->started) {
		iter->dict_ptr = TAILQ_FIRST(&dict->entries);
		iter->started = 1;
	}
	if (iter->dict_ptr == TAILQ_END(&dict->entries))
		return NULL;
	bzero(&iter->iteritem, sizeof(iter->iteritem));
	iter->iteritem.key = (struct xobject *)iter->dict_ptr->key;
	iter->iteritem.value = iter->dict_ptr->value;
	iter->dict_ptr = TAILQ_NEXT(iter->dict_ptr, entry);
	return &iter->iteritem;
}


struct xiteritem *
xiterator_next(struct xiterator *iter)
{
	switch (iter->object->type) {
	case TYPE_XARRAY:
		return xiterator_next_array(iter);
	case TYPE_XDICT:
		return xiterator_next_dict(iter);
	default:
		return NULL;
	}
}
