/*-
 * Copyright (c) 2009 Luigi Rizzo Universita` di Pisa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: user/luigi/ipfw3-head/sys/netinet/ipfw/ip_fw_table.c 200601 2009-12-16 10:48:40Z luigi $");

/*
 * Rule and pipe lookup support for ipfw.
 *

ipfw and dummynet need to quickly find objects (rules, pipes)
that may be dynamically created or destroyed.
To address the problem, we label each new object with a unique
32-bit identifier whose low K bits are the index in a lookup
table. All existing objects are referred by the lookup table,
and identifiers are chosen so that for each slot there is
at most one active object (whose identifier points to the slot).
This is almost a hash table, except that we can pick the
identifiers after looking at the table's occupation so
we have a trivial hash function and are collision free.

With this structure, operations are very fast and simple:
- the table has N entries s[i] with two fields, 'id' and 'ptr',
  with N <= M = 2^k (M is an upper bound to the size of the table);
- initially, all slots have s[i].id = i, and the pointers
  are used to build a freelist (tailq).
- a slot is considered empty if ptr == NULL or s[0] <= ptr < s[N].
  This is easy to detect and we can use ptr to build the freelist.
- when a new object is created, we put it in the empty slot i at the
  head of the freelist, and set the id to s[i].id;
- when an object is destroyed, we append its slot i to the end
  of the freelist, and set s[i].id += M (note M, not N).
- on a lookup for id = X, we look at slot i = X & (M-1),
  and consider the lookup successful only if the slot is not
  empty and s[i].id == X;
- wraps occur at most every F * 2^32/M operations, where F is
  the number of free slots. Because F is usually a reasonable
  fraction of M, we should not worry too much.
- if the table fills up, we can extend it by increasing N
- shrinking the table is more difficult as we might create
  collisions during the rehashing.
 *
 */

#include <sys/cdefs.h>
#ifdef _KERNEL
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/rwlock.h>
MALLOC_DEFINE(M_IPFW_LUT, "ipfw_lookup", "IpFw lookup");
#define Malloc(n)	malloc(n, M_IPFW_LUT, M_WAITOK)
#define Calloc(n)	calloc(n, M_IPFW_LUT, M_WAITOK | M_ZERO)
#define Free(p)		free(p, M_IPFW_LUT)

#define log(x, arg...)

#else /* !_KERNEL */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define Malloc(n)	malloc(n)
#define Calloc(n)	calloc(1, n)
#define Free(p)		free(p)
#define log(x, arg...)	fprintf(stderr, "%s: " x "\n", __FUNCTION__, ##arg)
#endif /* !_KERNEL */

struct entry {
	uint32_t	id;
	struct entry	*ptr;
};

struct lookup_table {
	int _size;
	int used;
	int mask; /* 2^k -1, used for hashing */
	struct entry *f_head, *f_tail; /* freelist */
	struct entry *	s;	/* slots, array of N entries */
};

static __inline int empty(struct lookup_table *head, const void *p)
{
	const struct entry *ep = p;
	return (ep == NULL ||
		(ep >= head->s && ep < &head->s[head->_size]));
}

/*
 * init or reinit a table
 */
struct lookup_table *
ipfw_lut_init(struct lookup_table *head, int new_size, int mask)
{
	int i;
	struct entry *s;	/* the new slots */
	struct entry *fh, *ft;	/* the freelist */

	if (head != NULL) {
		mask = head->mask;
		if (new_size <= head->_size)
			return head;
		if (new_size >= mask+1) {
			log("size larger than mask");
			return NULL;
		}
	} else {
		log("old is null, initialize");
		head = Calloc(sizeof(*head));
		if (head == NULL)
			return NULL;
		if (new_size >= mask)
			mask = new_size;
		if (mask & (mask -1)) {
			for (i = 1; i < mask; i += i)
			    ;
			log("mask %d not 2^k, round up to %d", mask, i);
			mask = i;
		}
		mask = head->mask = mask - 1;
	}

	s = Calloc(new_size * sizeof(*s));
	if (s == NULL)
		return NULL;
	if (!head->s) {
		head->s = s;
		head->_size = 1;
	}
	fh = ft = NULL;
	/* remap the entries, adjust the freelist */
	for (i = 0; i < new_size; i++) {
		s[i].id = (i >= head->_size) ? i : head->s[i].id;
		if (i < head->_size && !empty(head, head->s[i].ptr)) {
			s[i].ptr = head->s[i].ptr;
			continue;
		}
		if (fh == NULL)
			fh = &s[i];
		else
			ft->ptr = &s[i];
		ft = &s[i];
	}
	head->f_head = fh;
	head->f_tail = ft;

	/* write lock on the structure, to protect the readers */
	fh = head->s;
	head->s = s;
	head->_size = new_size;
	/* release write lock */
	if (fh != s)
		Free(fh);
	log("done");
	return head;
}

/* insert returns the id */
int
ipfw_lut_insert(struct lookup_table *head, void *d)
{
	struct entry *e;

	e = head->f_head;
	if (e == NULL)
		return -1;
	head->f_head = e->ptr;
	e->ptr = d;
	head->used++;
	return e->id;
}

/* delete, returns the original entry */
void *
ipfw_lut_delete(struct lookup_table *head, int id)
{
	int i = id & head->mask;
	void *result;
	struct entry *e;

	if (i >= head->_size)
		return NULL;
	e = &head->s[i];
	if (e->id != id)
		return NULL;
	result = e->ptr;
	/* write lock to invalidate the entry to readers */
	e->id += head->mask + 1; /* prepare for next insert */
	e->ptr = NULL;
	/* release write lock */
	if (head->f_head == NULL)
		head->f_head = e;
	else
		head->f_tail->ptr = e;
	head->f_tail = e;
	head->used--;
	return result;
}

void *
ipfw_lut_lookup(struct lookup_table *head, int id)
{
	int i = id & head->mask;
	struct entry *e;

	if (i >= head->_size)
		return NULL;
	e = &head->s[i];
	return (e->id == id) ? e->ptr : NULL;
}

void
ipfw_lut_dump(struct lookup_table *head)
{
	int i;

	log("head %p size %d used %d freelist %d",
	    head, head->_size, head->used, head->f_head ?
		    head->f_head - head->s : -1);
	for (i = 0; i < head->_size; i++) {
		struct entry *e = &head->s[i];
		char ee = empty(head, e->ptr) ? 'E' : ' ';
		log("%5d  %5d %c %p", i, e->id, ee,
		    ee == 'E' && e->ptr != NULL ?
		    (void *)((struct entry *)e->ptr - head->s) : e->ptr);
	}
}

#ifndef _KERNEL
void dump_p(struct lookup_table *p, int *map)
{
	int i;
	for (i = 0; i < p->_size; i++) {
	    int id = (int)ipfw_lut_lookup(p, map[i]);
	    log("%3d: %3d: %c", map[i] % 64, i, id);
	}
}
int main(int argc, char *argv[])
{
	int i, j, l;
#define S 1000
	int map[S];
	struct lookup_table *p;
	struct lookup_table *p1;
	const char *m = "nel mezzo del cammin di nostra vita mi ritrovai"
		" in una selva oscura e la diritta via era smarrita!";

	fprintf(stderr, "testing lookup\n");

	l = strlen(m);

	p = ipfw_lut_init(NULL, 120, 33);

	ipfw_lut_dump(p);
	for (i = 0; i < l; i++) {
	    int x = m[i];
	    int id = ipfw_lut_insert(p, (void *)x);
	    //ipfw_lut_dump(p);
	    map[i] = id;
	    for (j=0; j < 10; j++) {
		    id = ipfw_lut_insert(p, (void *)'a');
		    // ipfw_lut_dump(p);
		    ipfw_lut_delete(p, id);
	    	    // ipfw_lut_dump(p);
	    }
	//    ipfw_lut_dump(p);
	} 
	dump_p(p, map);
	p1 = ipfw_lut_init(p, 23, 0);
	if (!p1)
		return 1;
	dump_p(p1, map);
	p1 = ipfw_lut_init(p1, 120, 0);
	if (!p1)
		return 1;
	dump_p(p1, map);
	return 0;
}
#endif
/* end of file */
