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
#define SIZE_T_MAX ULONG_MAX
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
	struct xstring *key;
	struct xobject *value;
	TAILQ_ENTRY(xdict_entry) entry;
};
TAILQ_HEAD(xdict_entries, xdict_entry);
/* XXX: better data structure for dicts here (hash or RB tree) */

/* Dictionary type */
struct xdict {
	enum xobject_type type; /* TYPE_XDICT */
	struct xdict_entries entries;
};

/* Generic iterator */
struct xiterator {
	struct xobject *object;
	u_int started;
	size_t array_ndx;		/* Only valid for TYPE_XARRAY */
	struct xobject *array_last_key;	/* Only valid for TYPE_XARRAY */
	struct xdict_entry *dict_ptr;	/* Only valid for TYPE_XDICT */
};

/* Single instance of xnone */
static struct xnone *xnone_singleton = NULL;

struct xnone *
xnone_new(void)
{
	/* Keep a single instance of xnone */
	/* XXX: memprotect it */
	if (xnone_singleton != NULL)
		return xnone_singleton;
	if ((xnone_singleton = calloc(1, sizeof(*xnone_singleton))) == NULL)
		return NULL;
	xnone_singleton->type = TYPE_XNONE;
	return xnone_singleton;
}

struct xint *
xint_new(int64_t v)
{
	struct xint *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_XINT;
	ret->value = v;
	return ret;
}

struct xstring *
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
	return ret;
}

struct xstring *
xstring_new(const u_char *value)
{
	return xstring_new2(value, strlen(value));
}

struct xarray *
xarray_new(void)
{
	struct xarray *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_XARRAY;
	ret->entries = 0;
	return ret;
}

struct xdict *
xdict_new(void)
{
	struct xdict *ret;

	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->type = TYPE_XDICT;
	TAILQ_INIT(&ret->entries);
	return ret;
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
		xobject_free((struct xobject *)oe->key);
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
		/* XXX assumes snprintf return value is sane */
		return snprintf(s, len, "%lld", ((struct xint *)o)->value);
	case TYPE_XARRAY:
		/* XXX assumes snprintf return value is sane */
		return snprintf(s, len, "xarray(%p, %llu)", o,
		    (unsigned long long)((struct xarray *)o)->nused);
	case TYPE_XDICT:
		/* XXX assumes snprintf return value is sane */
		return snprintf(s, len, "xdict(%p)", o);
	default:
		return strlcpy(s, "Unsupported object type %d", o->type);
	}
}

int64_t
xint_value(const struct xint *v)
{
	if (v->type != TYPE_XINT)
		return 0;
	return v->value;
}

size_t
xstring_len(const struct xstring *s)
{
	if (s->type != TYPE_XSTRING)
		return 0;
	return s->len;
}

const u_char *
xstring_ptr(const struct xstring *s)
{
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
xarray_prepend(struct xarray *array, struct xobject *object)
{
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
xarray_append(struct xarray *array, struct xobject *object)
{
	if (array->type != TYPE_XARRAY)
		return -1;
	if (xarray_resize(array, array->nused + 1) == -1)
		return -1;
	array->entries[array->nused++] = object;
	return 0;
}

size_t
xarray_len(struct xarray *array)
{
	if (array->type != TYPE_XARRAY)
		return 0;
	return array->nused;
}

const struct xobject *
xarray_last(struct xarray *array)
{
	if (array->type != TYPE_XARRAY)
		return NULL;
	return array->nused == 0 ? NULL : array->entries[array->nused - 1];
}

const struct xobject *
xarray_first(struct xarray *array)
{
	if (array->type != TYPE_XARRAY)
		return NULL;
	return array->nused == 0 ? NULL : array->entries[0];
}

struct xobject *
xarray_pop(struct xarray *array)
{
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
xarray_pull(struct xarray *array)
{
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

const struct xobject *
xarray_item(struct xarray *array, size_t ndx)
{
	if (array->type != TYPE_XARRAY)
		return NULL;
	if (ndx >= array->nused)
		return NULL;
	return array->entries[ndx];
}

static int
xstring_cmp(const struct xstring *a, const struct xstring *b)
{
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

const struct xobject *
xdict_item(struct xdict *dict, const struct xstring *key)
{
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
xdict_remove(struct xdict *dict, const struct xstring *key)
{
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
			return ret;
		}
	}
	return NULL;
}

int
xdict_delete(struct xdict *dict, const struct xstring *key)
{
	struct xobject *o = xdict_remove(dict, key);

	if (dict->type != TYPE_XDICT || key->type != TYPE_XSTRING)
		return -1;
	if (o == NULL)
		return -1;
	xobject_free(o);
	return 0;
}

int
xdict_insert(struct xdict *dict, struct xstring *key, struct xobject *value)
{
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
	return 0;
}

int
xdict_replace(struct xdict *dict, struct xstring *key, struct xobject *value)
{
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
	} else {
		if ((e = calloc(1, sizeof(*e))) == NULL)
			return -1;
	}
	e->key = key;
	e->value = value;
	TAILQ_INSERT_TAIL(&dict->entries, e, entry);
	return 0;
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
	static struct xiteritem *ret;
	struct xobject *key;

	if (!iter->started) {
		iter->array_ndx = 0;
		iter->array_last_key = NULL;
		iter->started = 1;
	}
	if (iter->array_ndx >= array->nused)
		return NULL;
	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	if ((key = (struct xobject *)xint_new(iter->array_ndx)) == NULL) {
		free(ret);
		return NULL;
	}
	if (iter->array_last_key != NULL)
		xobject_free(iter->array_last_key);
	iter->array_last_key = key;
	ret->key = key;
	ret->value = array->entries[iter->array_ndx++];
	return ret;
}

static struct xiteritem *
xiterator_next_dict(struct xiterator *iter)
{
	struct xdict *dict = (struct xdict *)(iter->object);
	static struct xiteritem *ret;
	
	if (!iter->started) {
		iter->dict_ptr = TAILQ_FIRST(&dict->entries);
		iter->started = 1;
	}
	if (iter->dict_ptr == TAILQ_END(&dict->entries))
		return NULL;
	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;
	ret->key = (struct xobject *)iter->dict_ptr->key;
	ret->value = iter->dict_ptr->value;
	iter->dict_ptr = TAILQ_NEXT(iter->dict_ptr, entry);
	return ret;
}


const struct xiteritem *
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
