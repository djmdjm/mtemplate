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

/* Simple template system */

#include <sys/types.h>
#include <sys/param.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "sys-queue.h"
#include "compat.h"

#include "mtemplate.h"
#include "mobject.h"

#define SUBST_OK	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"\
			"abcdefghijklmnopqrstuvwxyz"\
			"0123456789._[]"

enum node_type {
	NODE_NONE = -1,
	NODE_TEXT = 0,
	NODE_DIRECTIVE_IF = 1,
	NODE_DIRECTIVE_ELSE = 2,
	NODE_DIRECTIVE_ENDIF = 3,
	NODE_DIRECTIVE_FOR = 4,
	NODE_DIRECTIVE_ENDFOR = 5,
	NODE_DIRECTIVE_SUBST = 6,
};
#define NODE_MAX NODE_DIRECTIVE_SUBST

struct node_info {
	enum node_type n;
	const char *p;
	enum node_type parent;
	u_int want_expression;
} node_info[] = {
	{ NODE_TEXT,		"text",		NODE_NONE,		0 },
	{ NODE_DIRECTIVE_IF,	"if",		NODE_NONE,		1 },
	{ NODE_DIRECTIVE_ELSE,	"else",		NODE_DIRECTIVE_IF,	0 },
	{ NODE_DIRECTIVE_ENDIF,	"endif",	NODE_DIRECTIVE_IF,	0 },
	{ NODE_DIRECTIVE_FOR,	"for",		NODE_NONE,		1 },
	{ NODE_DIRECTIVE_ENDFOR,"endfor",	NODE_DIRECTIVE_FOR,	0 },
	{ NODE_DIRECTIVE_SUBST,	"subst",	NODE_NONE,		1 },
	{ NODE_NONE, 		NULL,		NODE_NONE,		0 },
};

struct mtemplate_nodes;
TAILQ_HEAD(mtemplate_nodes, mtemplate_node);

struct mtemplate_node {
	enum node_type type;
	char *text;
	u_int lnum;
	char *localvar;		/* Used for iteration variable in 'for' */
	u_int in_else;		/* Only valid for "if" */
	struct mtemplate_nodes child_nodes;
	struct mtemplate_nodes child_nodes_else;
	struct mtemplate_node *parentp;
	TAILQ_ENTRY(mtemplate_node) entry;
};

struct mtemplate {
	struct mtemplate_node root;
};

/* Callback structure for memory buffer output */
struct alloc_cb_ctx {
	char *s;
	size_t len;
	size_t alloc;
};
#define ALLOC_MAX	(64 * 1024 * 1024)

static int
mtemplate_run_nodes(struct mtemplate_nodes *nodes, struct mobject *ns,
    struct mobject *local_ns, char *ebuf, size_t elen,
    int (*out_cb)(const char *, void *), void *out_ctx);

static void
format_err(int lnum, char *ebuf, size_t elen, const char *fmt, ...)
{
	char at[64];
	va_list args;
	
	if (ebuf == NULL || elen == 0)
		return;
	
	*at = '\0';
	if (lnum > 0)
		snprintf(at, sizeof(at), " at line %d", lnum);
	
	va_start(args, fmt);
	vsnprintf(ebuf, elen, fmt, args);
	va_end(args);
	
	strlcat(ebuf, at, elen);
}

static struct mtemplate_node *
alloc_node(const char *p, size_t len, enum node_type type, u_int lnum,
    struct mtemplate_node *parent, char *ebuf, size_t elen)
{
	struct mtemplate_node *node;

	if ((node = calloc(1, sizeof(*node))) == NULL) {
		format_err(lnum, ebuf, elen, "Node allocation failed");
		return NULL;
	}
	TAILQ_INIT(&node->child_nodes);
	TAILQ_INIT(&node->child_nodes_else);
	node->parentp = parent;
	if (p == NULL) {
		node->text = NULL;
	} else {
		if ((node->text = malloc(len + 1)) == NULL) {
			format_err(lnum, ebuf, elen,
			    "Node text allocation failed");
			free(node);
			return NULL;
		}
		memcpy(node->text, p, len);
		node->text[len] = '\0';
	}
	node->localvar = NULL;
	node->type = type;
	node->lnum = lnum;
	return node;
}

static int
parse_for(struct mtemplate_node *n)
{
	char *cp, *tmp;

	/* expect {LOCALVAR in REFERENCE} */
	if ((cp = strstr(n->text, " in ")) == NULL)
		return -1;
	*cp = '\0';
	cp += 4;
	if ((tmp = strdup(cp)) == NULL)
		return -1;
	if ((n->localvar = strdup(n->text)) == NULL) {
		free(tmp);
		return -1;
	}
	if (strchr(tmp, ' ') != NULL) {
		free(n->localvar);
		free(tmp);
		return -1;
	}
	free(n->text);
	n->text = tmp;
	return 0;
}

static int
classify_node(const char *directive, size_t len, const char **end_p,
    enum node_type *typep)
{
	size_t i, dlen;

	if (len < 1)
		return -1;

	*end_p = NULL;
	for (i = 0; node_info[i].p != NULL; i++) {
		dlen = strlen(node_info[i].p);
		if (len < dlen ||
		    strncmp(directive, node_info[i].p, dlen) != 0)
			continue;
		*typep = node_info[i].n;
		if (!node_info[i].want_expression)
			return 0;
		/* If an expression is required, then make sure it exists */
		if (len <= dlen + 1 || directive[dlen] != ' ')
			return -1;
		*end_p = directive + dlen + 1;
		return 0;
	}

	/* Check for {* escape */
	if (*directive == '{') {
		for (i = 1; i < len && directive[i] == '{'; i++)
			;
		if (i >= len) {
			*end_p = directive;
			*typep = NODE_TEXT;
			return 0;
		}
	}

	/* Probably a DIRECTIVE_SUBST, but do a basic sanity check */
	for (i = 0; i < len; i++) {
		if (strchr(SUBST_OK, directive[i]) == NULL)
			return -1;
	}

	*end_p = directive;
	*typep = NODE_DIRECTIVE_SUBST;
	return 0;
}

static const char *
node_type_ntop(enum node_type n)
{
	if (n <= NODE_NONE || n > NODE_MAX)
		return "UNKNOWN";		
	return node_info[n].p;
}

static enum node_type
mtemplate_node_parent(enum node_type n)
{
	if (n <= NODE_NONE || n > NODE_MAX)
		return NODE_NONE;
	return node_info[n].parent;
}

/* XXX append-to-template/incremental parse mode (keep state for [/[) */

struct mtemplate *
mtemplate_parse(const char *text, char *ebuf, size_t elen)
{
	size_t o, p, dlen, tlen;
	int lnum;
	const char *start_p, *end_p, *cp;
	struct mtemplate *ret;
	struct mtemplate_node *node, *parent;
	struct mtemplate_nodes *activep;
	enum node_type type, expected;

	/* XXX leak on err */
	if ((ret = calloc(1, sizeof(*ret))) == NULL) {
		format_err(-1, ebuf, elen, "%s: calloc(mtemplate)", __func__);
		return NULL;
	}

	/* Root node */
	ret->root.type = NODE_NONE;
	TAILQ_INIT(&ret->root.child_nodes);
	TAILQ_INIT(&ret->root.child_nodes_else); /* Should never be used */
	ret->root.parentp = NULL;

	activep = &ret->root.child_nodes;
	parent = &ret->root;
	node = NULL;
	for(p = o = 0, lnum = 1;;) {
		/* Match newlines and directive starting sequence */
		start_p = strpbrk(text + o + p, "\n{");

		/* Append string at end of template */
		if (start_p == NULL) {
			p = strlen(text + o);
			if (p == 0)
				break;
			if ((node = alloc_node(text + o, p, NODE_TEXT,
			    lnum, parent, ebuf, elen)) == NULL) {
 mtemplate_parse_err:
				mtemplate_free(ret);
				return NULL;
			}
			TAILQ_INSERT_TAIL(activep, node, entry);
			break;
		}

		/* Count newlines */
		if (*start_p == '\n') {
			lnum++;
			p = (start_p + 1) - (text + o);
			continue;
		}
		/* Make sure we have a full opening sequence */
		if (*(start_p + 1) != '{') {
			p = (start_p + 1) - (text + o);
			continue;
		}

		/* We have found a directive */

		/* Append a text node up until the current directive */
		if (start_p > text + o) {
			if ((node = alloc_node(text + o,
			    start_p - (text + o), NODE_TEXT,
			    lnum, parent, ebuf, elen)) == NULL)
				goto mtemplate_parse_err;
			TAILQ_INSERT_TAIL(activep, node, entry);
		}
	
		/* skip opening of directive '}}' */
		start_p += 2;

		/* Find end of directive */
		if ((end_p = strstr(start_p, "}}")) == NULL) {
			format_err(lnum, ebuf, elen, "Unterminated directive");
			goto mtemplate_parse_err;
		}
		/* Disallow newlines inside directive */
		if ((cp = strchr(start_p, '\n')) != NULL && cp <= end_p) {
			format_err(lnum, ebuf, elen, "Newline in directive");
			goto mtemplate_parse_err;
		}

		dlen = end_p - start_p;
		/* Disallow empty directives */
		if (dlen < 1) {
			format_err(lnum, ebuf, elen, "Empty directive");
			goto mtemplate_parse_err;
		}

		/* Skip comment directives entirely */
		if (*start_p == '#')
			goto node_done;

		if (classify_node(start_p, dlen, &cp, &type) == -1 ||
		    (cp != NULL && cp >= end_p)) {
			format_err(lnum, ebuf, elen, "Invalid directive");
			goto mtemplate_parse_err;
		}
		tlen = (cp == NULL) ? 0 : end_p - cp;

		/* Check parents are valid if required */
		expected = mtemplate_node_parent(type);
		if (expected != NODE_NONE &&
		    (parent == NULL || parent->type != expected)) {
			format_err(lnum, ebuf, elen,
			    "\"%s\" directive outside of \"%s\"",
			    node_type_ntop(type),
			    node_type_ntop(expected));
			goto mtemplate_parse_err;
		}

		/* Handle else, ascend on block close */
		switch (type) {
		case NODE_DIRECTIVE_ELSE:
			if (parent->in_else) {
				format_err(lnum, ebuf, elen,
				    "\"else\" inside \"else\"");
				goto mtemplate_parse_err;
			}
			parent->in_else = 1;
			activep = &parent->child_nodes_else;
			goto node_done;
		case NODE_DIRECTIVE_ENDIF:
		case NODE_DIRECTIVE_ENDFOR:
			/* XXX i think this can be avoided */
			if (parent->parentp->type == NODE_DIRECTIVE_IF &&
			    parent->parentp->in_else)
				activep = &parent->parentp->child_nodes_else;
			else
				activep = &parent->parentp->child_nodes;
			parent = parent->parentp;
			goto node_done;
		default:
			break;
		}

		if ((node = alloc_node(cp, tlen, type, lnum,
		    parent, ebuf, elen)) == NULL) {
			goto mtemplate_parse_err;
		}
		TAILQ_INSERT_TAIL(activep, node, entry);

		/* Special treatment for 'for', descend for opening blocks */
		switch (type) {
		case NODE_DIRECTIVE_FOR:
			if (parse_for(node) == -1) {
				format_err(lnum, ebuf, elen,
				    "Invalid \"for\" syntax");
				goto mtemplate_parse_err;
			}
			/* FALLTHROUGH */
		case NODE_DIRECTIVE_IF:
			activep = &node->child_nodes;
			parent = node;
			break;
		default:
			break;
		}

node_done:
		p = 0;
		o = 2 + end_p - text;
	}

	if (parent != &ret->root) {
		format_err(lnum, ebuf, elen,
		    "Unclosed \"%s\" block, begun at line %u",
		    node_type_ntop(parent->type), parent->lnum);
		goto mtemplate_parse_err;
	}

	return ret;
}

static void
mtemplate_free_nodes(struct mtemplate_nodes *nodes)
{
	struct mtemplate_node *n;

	while ((n = TAILQ_FIRST(nodes)) != NULL) {
		TAILQ_REMOVE(nodes, n, entry);
		if (n->text != NULL) {
			bzero(n->text, strlen(n->text));
			free(n->text);
		}
		if (n->localvar != NULL) {
			bzero(n->localvar, strlen(n->localvar));
			free(n->localvar);
		}
		mtemplate_free_nodes(&n->child_nodes);
		mtemplate_free_nodes(&n->child_nodes_else);
		bzero(n, sizeof(n));
		free(n);
	}
}

void
mtemplate_free(struct mtemplate *tmpl)
{
	mtemplate_free_nodes(&tmpl->root.child_nodes);
	bzero(tmpl, sizeof(tmpl));
	free(tmpl);
}

/* XXX move this to libmobject */
static u_int
mobject_as_boolean(struct mobject *o)
{
	if (o == NULL)
		return 0;
	switch (mobject_type(o)) {
		case TYPE_MNONE:
			return 0;
		case TYPE_MSTRING:
			return (mstring_len(o) > 0);
		case TYPE_MINT:
			return (mint_value(o) != 0);
		case TYPE_MARRAY:
			return (marray_len(o) > 0);
		case TYPE_MDICT:
			return (mdict_len(o) > 0);
		default:
			return 0;
	}
}

static struct mobject *
fetch_var(char *name, struct mobject *ns, struct mobject *local_ns,
    u_int lnum, char *directive, char *ebuf, size_t elen)
{
	char buf[1024];
	struct mobject *o;

	/* XXX need better mnamespace_lookup return values */
	/* Look up in local namespace first */
	if (mnamespace_lookup(local_ns, name, &o, buf, sizeof(buf)) == 0)
		return o;
	/* Try user namespace next */
	if (ns != NULL &&
	    mnamespace_lookup(ns, name, &o, buf, sizeof(buf)) == 0)
		return o;

	format_err(lnum, ebuf, elen, "Error in %s: %s", directive, buf);
	return NULL;
}

static int
do_loop(struct mtemplate_node *n, struct miteritem *item,
    struct mobject *ns, struct mobject *frame, char *ebuf, size_t elen,
    int (*out_cb)(const char *, void *), void *out_ctx)
{
	struct mobject *k, *v;
	struct mobject *loopvar;
	int r;

	if ((loopvar = mdict_item_s(frame, n->localvar)) == NULL) {
		format_err(n->lnum, ebuf, elen, "Loop variable missing");
		return -1;
	}
	/* Enter the name into the local namespace */
	if ((k = mobject_deepcopy(item->key)) == NULL) {
		format_err(n->lnum, ebuf, elen, "Loop key copy failed");
		return -1;
	}
	if ((v = mobject_deepcopy(item->value)) == NULL) {
		mobject_free(k);
		format_err(n->lnum, ebuf, elen, "Loop value copy failed");
		return -1;
	}
	if (mdict_replace_s(loopvar, "key", k) == NULL) {
		mobject_free(k);
		mobject_free(v);
		format_err(n->lnum, ebuf, elen, "Loop key insert failed");
		return -1;
	}
	if (mdict_replace_s(loopvar, "value", v) == NULL) {
		mobject_free(v);
		format_err(n->lnum, ebuf, elen, "Loop value insert failed");
		return -1;
	}
	r = mtemplate_run_nodes(&n->child_nodes, ns, frame, ebuf, elen,
	    out_cb, out_ctx);
	return r;
}

/* XXX: libmobject should have a non-vis mode */
#define RENDER_MO_ALLOC 256
static int
render_mobject(struct mobject *o, u_int lnum, char *ebuf, size_t elen,
    int (*out_cb)(const char *, void *), void *out_ctx)
{
	char *buf, *tmp;
	size_t need;

	if (mobject_type(o) == TYPE_MSTRING) {
		if (out_cb((char *)mstring_ptr(o), out_ctx) != 0) {
			format_err(lnum, ebuf, elen, "write error");
			return -1;
		}
		return 0;
	}
	if ((buf = malloc(RENDER_MO_ALLOC)) == NULL) {
		format_err(lnum, ebuf, elen,
		    "malloc(%zu) failed for substitution", RENDER_MO_ALLOC);
		return -1;
	}
	need = mobject_to_string(o, buf, RENDER_MO_ALLOC) + 1;
	if (need >= RENDER_MO_ALLOC) {
		if ((tmp = realloc(buf, need)) == NULL) {
			format_err(lnum, ebuf, elen,
			    "realloc(%zu) failed for substitution", need);
			free(buf);
			return -1;
		}
		buf = tmp;
		mobject_to_string(o, buf, need);
	}
	if (out_cb(buf, out_ctx) != 0) {
		format_err(lnum, ebuf, elen, "write error");
		return -1;
	}
	free(buf);
	return 0;
}

static int
mtemplate_run_nodes(struct mtemplate_nodes *nodes, struct mobject *ns,
    struct mobject *local_ns, char *ebuf, size_t elen,
    int (*out_cb)(const char *, void *), void *out_ctx)
{
	struct mtemplate_node *n;
	struct mobject *o;
	struct miterator *iter;
	struct miteritem *item;
	struct mobject *frame;
	int r;

	TAILQ_FOREACH(n, nodes, entry) {
		switch (n->type) {
		case NODE_TEXT:
			if (out_cb(n->text, out_ctx) == EOF) {
				format_err(n->lnum, ebuf, elen, "write error");
				return -1;
			}
			break;
		case NODE_DIRECTIVE_IF:
			if ((o = fetch_var(n->text, ns, local_ns, n->lnum,
			    "\"if\" directive", ebuf, elen)) == NULL)
				return -1;
			if (mobject_as_boolean(o)) {
				r = mtemplate_run_nodes(&n->child_nodes, ns,
				    local_ns, ebuf, elen, out_cb, out_ctx);
			} else {
				r = mtemplate_run_nodes(&n->child_nodes_else,
				    ns, local_ns, ebuf, elen, out_cb, out_ctx);
			}
			if (r != 0)
				return r;
			break;
		case NODE_DIRECTIVE_FOR:
			if ((o = fetch_var(n->text, ns, local_ns, n->lnum,
			    "\"for\" directive", ebuf, elen)) == NULL)
				return -1;
			if ((iter = mobject_getiter(o)) == NULL) {
				format_err(n->lnum, ebuf, elen,
				    "Error in \"for\": "
				    "could not get iterator from object %s",
				    n->text);
				return -1;
			}
			frame = mobject_deepcopy(local_ns);
			if (frame == NULL) {
				format_err(n->lnum, ebuf, elen,
				    "Could not create local namespace");
				miterator_free(iter);
				return -1;
			}
			/* Create the loop variable in this frame */
			if (mdict_replace_sd(frame, n->localvar) == NULL) {
				format_err(n->lnum, ebuf, elen,
				    "Could not setup loop variable");
				mobject_free(frame);
				miterator_free(iter);
				return -1;
			}
			while ((item = miterator_next(iter)) != NULL) {
				if (do_loop(n, item, ns, frame,
				    ebuf, elen, out_cb, out_ctx) == -1) {
					miterator_free(iter);
					mobject_free(frame);
					return -1;
				}
			}
			miterator_free(iter);
			mobject_free(frame);
			break;
		case NODE_DIRECTIVE_SUBST:
			if ((o = fetch_var(n->text, ns, local_ns, n->lnum,
			    "variable substitution", ebuf, elen)) == NULL)
				return -1;
			if (render_mobject(o, n->lnum, ebuf, elen,
			    out_cb, out_ctx) == -1)
				return -1;
			break;
		default:
			format_err(n->lnum, ebuf, elen,
			    "unsupported node type %s (%d)",
			    node_type_ntop(n->type), n->type);
			return -1;
		}
	}
	return 0;
}

int
mtemplate_run_cb(struct mtemplate *tmpl, struct mobject *ns, char *ebuf,
    size_t elen, int (*out_cb)(const char *, void *), void *out_ctx)
{
	struct mobject *local_ns;
	int r;

	if ((local_ns = mdict_new()) == NULL) {
		format_err(-1, ebuf, elen,
		    "Unable to allocate local namespace");
		return -1;
	}
	r = mtemplate_run_nodes(&tmpl->root.child_nodes, ns,
	    local_ns, ebuf, elen, out_cb, out_ctx);
	mobject_free(local_ns);
	return r;
}

static int
out_stdio_cb(const char *buf, void *ctx)
{
	return fputs(buf, (FILE *)ctx) == EOF ? -1 : 0;
}

int
mtemplate_run_stdio(struct mtemplate *tmpl, struct mobject *ns, FILE *out,
    char *ebuf, size_t elen)
{
	return mtemplate_run_cb(tmpl, ns, ebuf, elen, out_stdio_cb, out);
}

static int
out_alloc_cb(const char *buf, void *_ctx)
{
	struct alloc_cb_ctx *ctx = (struct alloc_cb_ctx *)_ctx;
	size_t addlen = strlen(buf);
	char *tmp;

	if (addlen == 0)
		return 0;
	if (addlen >= ALLOC_MAX || ctx->len + addlen >= ALLOC_MAX)
		goto out_alloc_cb_err;
	if (ctx->len + addlen + 1 > ctx->alloc) {
		if (ctx->alloc == 0)
			ctx->alloc = MAX(addlen + 1, 256);
		else {
			ctx->alloc = MIN((ctx->alloc * 2), ALLOC_MAX);
			if (ctx->alloc < addlen + 1)
				ctx->alloc = addlen + 1;
		}
		if ((tmp = realloc(ctx->s, ctx->alloc)) == NULL)
			goto out_alloc_cb_err;
		ctx->s = tmp;
	}
	memcpy(ctx->s + ctx->len, buf, addlen + 1);
	ctx->len += addlen;
	return 0;
	
 out_alloc_cb_err:
	if (ctx->s)
		free(ctx->s);
	ctx->s = NULL;
	ctx->alloc = ctx->len = 0;
	return -1;
}

int
mtemplate_run_mbuf(struct mtemplate *tmpl, struct mobject *ns, char **outp,
    char *ebuf, size_t elen)
{
	struct alloc_cb_ctx ctx = { NULL, 0, 0 };
	int r;

	*outp = NULL;
	r = mtemplate_run_cb(tmpl, ns, ebuf, elen, out_alloc_cb, &ctx);
	if (r == 0)
		*outp = ctx.s;
	else
		free(ctx.s);
	return r;
}
