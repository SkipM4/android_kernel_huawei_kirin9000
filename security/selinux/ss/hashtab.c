// SPDX-License-Identifier: GPL-2.0
/*
 * Implementation of the hash table type.
 *
 * Author : Stephen Smalley, <sds@tycho.nsa.gov>
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#ifdef CONFIG_HKIP_SELINUX_PROT
#include <platform_include/see/prmem.h>
#endif
#include "hashtab.h"

#ifdef CONFIG_HKIP_SELINUX_PROT
extern struct prmem_pool selinux_pool;
#else
static struct kmem_cache *hashtab_node_cachep;
#endif

/*
 * Here we simply round the number of elements up to the nearest power of two.
 * I tried also other options like rouding down or rounding to the closest
 * power of two (up or down based on which is closer), but I was unable to
 * find any significant difference in lookup/insert performance that would
 * justify switching to a different (less intuitive) formula. It could be that
 * a different formula is actually more optimal, but any future changes here
 * should be supported with performance/memory usage data.
 *
 * The total memory used by the htable arrays (only) with Fedora policy loaded
 * is approximately 163 KB at the time of writing.
 */
static u32 hashtab_compute_size(u32 nel)
{
	return nel == 0 ? 0 : roundup_pow_of_two(nel);
}

int hashtab_init(struct hashtab *h, u32 nel_hint)
{
	h->size = hashtab_compute_size(nel_hint);
	h->nel = 0;
	if (!h->size)
		return 0;

#ifdef CONFIG_HKIP_SELINUX_PROT
	unsigned long size = h->size * sizeof(*h->htable);
	h->htable = pmalloc(&selinux_pool, size, PRMEM_NO_FLAGS);
#else
	h->htable = kcalloc(h->size, sizeof(*h->htable), GFP_KERNEL);
#endif
	return h->htable ? 0 : -ENOMEM;
}

int __hashtab_insert(struct hashtab *h, struct hashtab_node **dst,
		     void *key, void *datum)
{
	struct hashtab_node *newnode;

#ifdef CONFIG_HKIP_SELINUX_PROT
	newnode = pzalloc(&selinux_pool, sizeof(struct hashtab_node),
			  PRMEM_NO_FLAGS);
#else
	newnode = kmem_cache_zalloc(hashtab_node_cachep, GFP_KERNEL);
#endif
	if (!newnode)
		return -ENOMEM;
	newnode->key = key;
	newnode->datum = datum;
	newnode->next = *dst;
	*dst = newnode;

	h->nel++;
	return 0;
}

void hashtab_destroy(struct hashtab *h)
{
	u32 i;
	struct hashtab_node *cur, *temp;

#ifdef CONFIG_HKIP_SELINUX_PROT
	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur) {
			temp = cur;
			cur = cur->next;
			pfree(temp);
		}
		h->htable[i] = NULL;
	}

	pfree(h->htable);
#else
	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur) {
			temp = cur;
			cur = cur->next;
			kmem_cache_free(hashtab_node_cachep, temp);
		}
		h->htable[i] = NULL;
	}

	kfree(h->htable);
#endif
	h->htable = NULL;
}

int hashtab_map(struct hashtab *h,
		int (*apply)(void *k, void *d, void *args),
		void *args)
{
	u32 i;
	int ret;
	struct hashtab_node *cur;

	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur) {
			ret = apply(cur->key, cur->datum, args);
			if (ret)
				return ret;
			cur = cur->next;
		}
	}
	return 0;
}


void hashtab_stat(struct hashtab *h, struct hashtab_info *info)
{
	u32 i, chain_len, slots_used, max_chain_len;
	struct hashtab_node *cur;

	slots_used = 0;
	max_chain_len = 0;
	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		if (cur) {
			slots_used++;
			chain_len = 0;
			while (cur) {
				chain_len++;
				cur = cur->next;
			}

			if (chain_len > max_chain_len)
				max_chain_len = chain_len;
		}
	}

	info->slots_used = slots_used;
	info->max_chain_len = max_chain_len;
}

int hashtab_duplicate(struct hashtab *new, struct hashtab *orig,
		int (*copy)(struct hashtab_node *new,
			struct hashtab_node *orig, void *args),
		int (*destroy)(void *k, void *d, void *args),
		void *args)
{
	struct hashtab_node *cur, *tmp, *tail;
	int i, rc;

	memset(new, 0, sizeof(*new));
#ifdef CONFIG_HKIP_SELINUX_PROT
	unsigned long size = orig->size * sizeof(*new->htable);
	new->htable = pmalloc(&selinux_pool, size, PRMEM_NO_FLAGS);
#else
	new->htable = kcalloc(orig->size, sizeof(*new->htable), GFP_KERNEL);
#endif
	if (!new->htable)
		return -ENOMEM;

	new->size = orig->size;

	for (i = 0; i < orig->size; i++) {
		tail = NULL;
		for (cur = orig->htable[i]; cur; cur = cur->next) {
#ifdef CONFIG_HKIP_SELINUX_PROT
			tmp = pzalloc(&selinux_pool, sizeof(struct hashtab_node),
				      PRMEM_NO_FLAGS);
			if (!tmp)
				goto error;
			rc = copy(tmp, cur, args);
			if (rc) {
				pfree(tmp);
				goto error;
			}
#else
			tmp = kmem_cache_zalloc(hashtab_node_cachep,
						GFP_KERNEL);
			if (!tmp)
				goto error;
			rc = copy(tmp, cur, args);
			if (rc) {
				kmem_cache_free(hashtab_node_cachep, tmp);
				goto error;
			}
#endif
			tmp->next = NULL;
			if (!tail)
				new->htable[i] = tmp;
			else
				tail->next = tmp;
			tail = tmp;
			new->nel++;
		}
	}

	return 0;

 error:
#ifdef CONFIG_HKIP_SELINUX_PROT
	for (i = 0; i < new->size; i++) {
		for (cur = new->htable[i]; cur; cur = tmp) {
			tmp = cur->next;
			destroy(cur->key, cur->datum, args);
			pfree(cur);
		}
	}
	pfree(new);
#else
	for (i = 0; i < new->size; i++) {
		for (cur = new->htable[i]; cur; cur = tmp) {
			tmp = cur->next;
			destroy(cur->key, cur->datum, args);
			kmem_cache_free(hashtab_node_cachep, cur);
		}
	}
	kmem_cache_free(hashtab_node_cachep, new);
#endif
	return -ENOMEM;
}

#ifndef CONFIG_HKIP_SELINUX_PROT
void __init hashtab_cache_init(void)
{
		hashtab_node_cachep = kmem_cache_create("hashtab_node",
			sizeof(struct hashtab_node),
			0, SLAB_PANIC, NULL);
}
#else
void hashtab_cache_init(void)
{
}
#endif
