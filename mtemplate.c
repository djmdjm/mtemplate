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

#include <sys/types.h>
#include <sys/param.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "sys-queue.h"
#include "strlcpy.h"
#include "strlcat.h"

#include "xtemplate.h"
#include "xobject.h"

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

struct xtemplate_nodes;
TAILQ_HEAD(xtemplate_nodes, xtemplate_node);

struct xtemplate_node {
	enum node_type type;
	char *text;
	u_int lnum;
	char *localvar;		/* Used for iteration variable in 'for' */
	u_int in_else;		/* Only valid for "if" */
	struct xtemplate_nodes child_nodes;
	struct xtemplate_nodes child_nodes_else;
	struct xtemplate_node *parentp;
	TAILQ_ENTRY(xtemplate_node) entry;
};

struct xtemplate {
	struct xtemplate_node root;
};

static int
xtemplate_run_nodes(struct xtemplate_nodes *nodes, struct xdict *namespace,
    struct xdict *local_namespace, FILE *out, char *ebuf, size_t elen);

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

static struct xtemplate_node *
alloc_node(const char *p, size_t len, enum node_type type, u_int lnum,
    struct xtemplate_node *parent, char *ebuf, size_t elen)
{
	struct xtemplate_node *node;

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
parse_for(struct xtemplate_node *n)
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

	/* Check for [[ escape */
	if (len == 2 && strncmp(directive, "{{", 2) == 0) {
		*end_p = directive;
		*typep = NODE_TEXT;
		return 0;
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
xtemplate_node_parent(enum node_type n)
{
	if (n <= NODE_NONE || n > NODE_MAX)
		return NODE_NONE;
	return node_info[n].parent;
}

/* XXX append-to-template/incremental parse mode (keep state for [/[) */

struct xtemplate *
xtemplate_parse(const char *template, char *ebuf, size_t elen)
{
	size_t o, p, dlen, tlen;
	int lnum;
	const char *start_p, *end_p, *cp;
	struct xtemplate *ret;
	struct xtemplate_node *node, *parent;
	struct xtemplate_nodes *activep;
	enum node_type type, expected;

	/* XXX leak on err */
	if ((ret = calloc(1, sizeof(*ret))) == NULL) {
		format_err(-1, ebuf, elen, "%s: calloc(xtemplate)", __func__);
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
		start_p = strpbrk(template + o + p, "\n{");

		/* Append string at end of template */
		if (start_p == NULL) {
			p = strlen(template + o);
			if (p > 0)
				break;
			if ((node = alloc_node(template + o, p, NODE_TEXT,
			    lnum, parent, ebuf, elen)) == NULL)
				return NULL;
			TAILQ_INSERT_TAIL(activep, node, entry);
			break;
		}

		/* Count newlines */
		if (*start_p == '\n') {
			lnum++;
			p = (start_p + 1) - (template + o);
			continue;
		}
		/* Make sure we have a full opening sequence */
		if (*(start_p + 1) != '{') {
			p = (start_p + 1) - (template + o);
			continue;
		}

		/* We have found a directive */

		/* Append a text node up until the current directive */
		if (start_p > template + o) {
			if ((node = alloc_node(template + o,
			    start_p - (template + o), NODE_TEXT,
			    lnum, parent, ebuf, elen)) == NULL)
			return NULL;
			TAILQ_INSERT_TAIL(activep, node, entry);
		}
	
		/* skip opening of directive '}}' */
		start_p += 2;

		/* Find end of directive */
		if ((end_p = strstr(start_p, "}}")) == NULL) {
			format_err(lnum, ebuf, elen, "Unterminated directive");
			return NULL;
		}
		/* Disallow newlines inside directive */
		if ((cp = strchr(start_p, '\n')) != NULL && cp <= end_p) {
			format_err(lnum, ebuf, elen, "Newline in directive");
			return NULL;
		}

		dlen = end_p - start_p;
		/* Disallow empty directives */
		if (dlen < 1) {
			format_err(lnum, ebuf, elen, "Empty directive");
			return NULL;
		}

		/* Skip comment directives entirely */
		if (*start_p == '#')
			goto node_done;

		if (classify_node(start_p, dlen, &cp, &type) == -1 ||
		    (cp != NULL && cp >= end_p)) {
			format_err(lnum, ebuf, elen, "Invalid directive");
			return NULL;
		}
		tlen = (cp == NULL) ? 0 : end_p - cp;

		/* Check parents are valid if required */
		expected = xtemplate_node_parent(type);
		if (expected != NODE_NONE &&
		    (parent == NULL || parent->type != expected)) {
			format_err(lnum, ebuf, elen,
			    "\"%s\" directive outside of \"%s\"",
			    node_type_ntop(type),
			    node_type_ntop(expected));
			return NULL;
		}

		/* Handle else, ascend on block close */
		switch (type) {
		case NODE_DIRECTIVE_ELSE:
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
			return NULL;
		}
		TAILQ_INSERT_TAIL(activep, node, entry);

		/* Special treatment for 'for', descend for opening blocks */
		switch (type) {
		case NODE_DIRECTIVE_FOR:
			if (parse_for(node) == -1) {
				format_err(lnum, ebuf, elen,
				    "Invalid \"for\" syntax");
				return NULL;
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
		o = 2 + end_p - template;
	}

	return ret;
}

/* XXX move this to libxobject */
static u_int
xobject_as_boolean(struct xobject *o)
{
	if (o == NULL)
		return 0;
	switch (xobject_type(o)) {
		case TYPE_XNONE:
			return 0;
		case TYPE_XSTRING:
			return (xstring_len((struct xstring *)o) > 0);
		case TYPE_XINT:
			return (xint_value((struct xint *)o) != 0);
		case TYPE_XARRAY:
			return (xarray_len((struct xarray *)o) > 0);
		case TYPE_XDICT:
			/* XXX */
			return 1;
		default:
			return 0;
	}
}

static struct xobject *
fetch_var(char *name, struct xdict *namespace,
    struct xdict *local_namespace, u_int lnum, char *directive,
    char *ebuf, size_t elen)
{
	char buf[1024];
	struct xobject *o;

	/* XXX need better xnamespace_lookup return values */
	/* Look up in local namespace first */
	if (xnamespace_lookup(local_namespace, name, &o,
	    buf, sizeof(buf)) == 0)
		return o;
	/* Try user namespace next */
	if (namespace != NULL &&
	    xnamespace_lookup(namespace, name, &o, buf, sizeof(buf)) == 0)
		return o;

	format_err(lnum, ebuf, elen, "Error in %s: %s", directive, buf);
	return NULL;
}

static int
do_loop(FILE *out, struct xtemplate_node *n, struct xiteritem *item,
    struct xdict *namespace, struct xdict *frame, char *ebuf, size_t elen)
{
	struct xobject *k, *v;
	struct xdict *loopvar;
	int r;

	if ((loopvar = (struct xdict *)xdict_item_s(frame,
	    n->localvar)) == NULL) {
		format_err(n->lnum, ebuf, elen, "Loop variable missing");
		return -1;
	}
	/* Enter the name into the local namespace */
	if ((k = xobject_deepcopy((struct xobject *)item->key)) == NULL) {
		format_err(n->lnum, ebuf, elen, "Loop key copy failed");
		return -1;
	}
	if ((v = xobject_deepcopy((struct xobject *)item->value)) == NULL) {
		xobject_free(k);
		format_err(n->lnum, ebuf, elen, "Loop value copy failed");
		return -1;
	}
	if (xdict_replace_s(loopvar, "key", k) == -1) {
		xobject_free(k);
		xobject_free(v);
		format_err(n->lnum, ebuf, elen, "Loop key insert failed");
		return -1;
	}
	if (xdict_replace_s(loopvar, "value", v) == -1) {
		xobject_free(v);
		format_err(n->lnum, ebuf, elen, "Loop value insert failed");
		return -1;
	}
	r = xtemplate_run_nodes(&n->child_nodes, namespace, frame,
	    out, ebuf, elen);
	return 0;
}

/* XXX: libxobject should have a non-vis mode */
#define RENDER_XO_ALLOC 256
static int
render_xobject(struct xobject *o, FILE *out, u_int lnum,
    char *ebuf, size_t elen)
{
	char *buf, *tmp;
	size_t need;

	if (xobject_type(o) == TYPE_XSTRING) {
		if (fputs(xstring_ptr((struct xstring *)o), out) == EOF) {
			format_err(lnum, ebuf, elen, "write error");
			return -1;
		}
		return 0;
	}
	if ((buf = malloc(RENDER_XO_ALLOC)) == NULL) {
		format_err(lnum, ebuf, elen,
		    "malloc(%zu) failed for substitution", RENDER_XO_ALLOC);
		return -1;
	}
	need = xobject_to_string(o, buf, RENDER_XO_ALLOC) + 1;
	if (need >= RENDER_XO_ALLOC) {
		if ((tmp = realloc(buf, need)) == NULL) {
			format_err(lnum, ebuf, elen,
			    "realloc(%zu) failed for substitution", need);
			free(buf);
			return -1;
		}
		buf = tmp;
		xobject_to_string(o, buf, need);
	}
	if (fputs(buf, out) == EOF) {
		format_err(lnum, ebuf, elen, "write error");
		return -1;
	}
	free(buf);
	return 0;
}

static int
xtemplate_run_nodes(struct xtemplate_nodes *nodes, struct xdict *namespace,
    struct xdict *local_namespace, FILE *out, char *ebuf, size_t elen)
{
	struct xtemplate_node *n;
	struct xobject *o;
	struct xiterator *iter;
	struct xiteritem *item;
	struct xdict *frame;
	int r;

	TAILQ_FOREACH(n, nodes, entry) {
		switch (n->type) {
		case NODE_TEXT:
			if (fputs(n->text, out) == EOF) {
				format_err(n->lnum, ebuf, elen, "write error");
				return -1;
			}
			break;
		case NODE_DIRECTIVE_IF:
			if ((o = fetch_var(n->text, namespace, local_namespace,
			    n->lnum, "\"if\" directive", ebuf, elen)) == NULL)
				return -1;
			if (xobject_as_boolean(o)) {
				r = xtemplate_run_nodes(&n->child_nodes,
				    namespace, local_namespace, out,
				    ebuf, elen);
			} else {
				r = xtemplate_run_nodes(&n->child_nodes_else,
				    namespace, local_namespace, out,
				    ebuf, elen);
			}
			if (r != 0)
				return r;
			break;
		case NODE_DIRECTIVE_FOR:
			if ((o = fetch_var(n->text, namespace, local_namespace,
			    n->lnum, "\"for\" directive", ebuf, elen)) == NULL)
				return -1;
			if ((iter = xobject_getiter(o)) == NULL) {
				format_err(n->lnum, ebuf, elen,
				    "Error in \"for\": "
				    "could not get iterator from object %s",
				    n->text);
				return -1;
			}
			if ((frame = (struct xdict *)xobject_deepcopy(
			    (struct xobject *)local_namespace)) == NULL) {
				format_err(n->lnum, ebuf, elen,
				    "Could not create local namespace");
				xiterator_free(iter);
				return -1;
			}
			/* Create the loop variable in this frame */
			if (xdict_replace_sd(frame, n->localvar) == -1) {
				format_err(n->lnum, ebuf, elen,
				    "Could not setup loop variable");
				xobject_free((struct xobject *)frame);
				xiterator_free(iter);
				return -1;
			}
			while ((item = xiterator_next(iter)) != NULL) {
				if (do_loop(out, n, item, namespace,
				    frame, ebuf, elen) == -1)
					return -1;
			}
			xiterator_free(iter);
			xobject_free((struct xobject *)frame);
			break;
		case NODE_DIRECTIVE_SUBST:
			if ((o = fetch_var(n->text, namespace, local_namespace,
			    n->lnum, "variable substitution",
			    ebuf, elen)) == NULL)
				return -1;
			if (render_xobject(o, out, n->lnum, ebuf, elen) == -1)
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
xtemplate_run(struct xtemplate *t, struct xdict *namespace, FILE *out,
    char *ebuf, size_t elen)
{
	struct xdict *local_namespace;
	int r;

	if ((local_namespace = xdict_new()) == NULL) {
		format_err(-1, ebuf, elen,
		    "Unable to allocate local namespace");
		return -1;
	}
	r = xtemplate_run_nodes(&t->root.child_nodes, namespace,
	    local_namespace, out, ebuf, elen);
	xobject_free((struct xobject *)local_namespace);
	return r;
}

/* XXX template_reassemble */
