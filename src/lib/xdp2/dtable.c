// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) Tom Herbert 2025
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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "siphash/siphash.h"

#include "xdp2/cli.h"
#include "xdp2/dtable.h"

/* Dynamic plain tables */

LIST_HEAD(__xdp2_dtable_table_listhead, xdp2_dtable_table);

struct __xdp2_dtable_table_listhead plain_tables, lpm_tables, tern_tables;

#define BASE_IDENT 1000000

siphash_key_t siphash_key = { { 0x1234567890abcdef, 0xfedcba0987654321 } };

/* Find a table. Arguments are:
 * - indent: A pointer to an identfier. If the value is zero then a table
 *   identifier is bing requwsted to find. If the value is zero that
 *   indicates that an identifier should be automatically chosen for a new
 *   table
 *
 * - pprev_table: A pointer to return the previous table in the list. This
 *   is used when inserting a new table (tables are ordered by identifier)
 *
 * - list_head: List of tables. Either plain_tables, tern_tables, or lpm_tables
 */
static struct xdp2_dtable_table *__xdp2_dtable_find_table(
		int *ident, struct xdp2_dtable_table **pprev_table,
		struct __xdp2_dtable_table_listhead *list_head)
{
	struct xdp2_dtable_table *prev_table = NULL;
	struct xdp2_dtable_table *table;
	int want_ident = BASE_IDENT;

	if (*ident < 0)
		return NULL;

	LIST_FOREACH(table, list_head, list_ent) {
		if (*ident) {
			if (table->ident == *ident)
				break;
			if (table->ident > *ident) {
				table = NULL;
				break;
			}
		} else {
			if (table->ident < BASE_IDENT)
				continue;
			if (table->ident != want_ident) {
				*ident = want_ident;
				table = NULL;
				break;
			}
			want_ident++;
			if (want_ident < 0) {
				table = NULL;
				prev_table = NULL;
				break;
			}
		}
		prev_table = table;
	}

	if (pprev_table)
		*pprev_table = prev_table;

	if (!*ident)
		*ident = want_ident;

	return table;
}

/* Insert a new tables in a tables list if the table is not already present */
static int __xdp2_dtable_insert_table(struct xdp2_dtable_table *table,
				       int *ident,
				       struct __xdp2_dtable_table_listhead
								*list_head)
{
	struct xdp2_dtable_table *otable, *prev_table;

	otable = __xdp2_dtable_find_table(ident, &prev_table, list_head);
	if (otable)
		return -EALREADY;

	if (*ident < 0)
		return -EINVAL;

	table->ident = *ident;

	if (prev_table)
		LIST_INSERT_AFTER(prev_table, table, list_ent);
	else
		LIST_INSERT_HEAD(list_head, table, list_ent);

	return 0;
}

/* Backend function to create a new table */
static struct xdp2_dtable_table *xdp2_dtable_create(
		const char *name, size_t key_len,
		size_t all_key_len, int *ident, size_t sz,
		void *default_target, size_t target_len,
		struct __xdp2_dtable_table_listhead *list_head)
{
	struct xdp2_dtable_table *table, *prev_table;
	void *dtarget;

	table = __xdp2_dtable_find_table(ident, &prev_table, list_head);
	if (table)
		return NULL;

	if (*ident < 0)
		return NULL;

	table = malloc(sz);
	if (!table)
		return NULL;

	dtarget = malloc(target_len);
	if (!dtarget) {
		free(table);
		return NULL;
	}

	memcpy(dtarget, default_target, target_len);

	table->default_target = dtarget;

	LIST_INIT(&table->entries);
	LIST_INIT(&table->entries_lookup);

	table->name = name;
	table->key_len = key_len;
	table->targ_len = target_len;
	table->entry_len = sizeof(struct xdp2_dtable_entry) +
		target_len + all_key_len;
	table->ident = *ident;

	if (__xdp2_dtable_insert_table(table, ident, list_head))
		return NULL;

	return table;
}

/* Find an entry by its identifier */
static struct xdp2_dtable_entry *xdp2_dtable_find_ent_by_id(
		struct xdp2_dtable_table *table, unsigned int ident)
{
	struct xdp2_dtable_entry *entry;

	LIST_FOREACH(entry, &table->entries, list_ent) {
		if (entry->ident > ident)
			break;
		if (entry->ident == ident)
			return entry;
	}

	return NULL;
}

/* Add an entry to a table */
static struct xdp2_dtable_entry *xdp2_dtable_add(
		struct xdp2_dtable_table *table,
		struct xdp2_dtable_entry *plentry,
		int *ident, void *target)
{
	struct xdp2_dtable_entry  *entry, *pentry = NULL;
	int want_ident = BASE_IDENT;

	LIST_FOREACH(entry, &table->entries, list_ent) {
		if (*ident) {
			if (entry->ident == *ident)
				return NULL;
			if (entry->ident > *ident)
				break;
		} else {
			if (entry->ident < BASE_IDENT)
				continue;
			if (entry->ident != want_ident) {
				*ident = want_ident;
				break;
			}
			want_ident++;
		}
		pentry = entry;
	}

	if (want_ident < 0)
		return NULL;

	entry = malloc(table->entry_len);
	if (!entry)
		return NULL;

	if (!*ident)
		*ident = want_ident;

	entry->ident = *ident;

	memcpy(XDP2_DTABLE_TARG(table, entry), target, table->targ_len);

	if (!pentry)
		LIST_INSERT_HEAD(&table->entries, entry, list_ent);
	else
		LIST_INSERT_AFTER(pentry, entry, list_ent);

	if (!plentry)
		LIST_INSERT_HEAD(&table->entries_lookup, entry,
				 list_ent_lookup);
	else
		LIST_INSERT_AFTER(plentry, entry, list_ent_lookup);

	return entry;
}

/* Delete an entry in a table */
static void xdp2_dtable_del(struct xdp2_dtable_table *table,
			     struct xdp2_dtable_entry *entry)
{
	LIST_REMOVE(entry, list_ent);
	LIST_REMOVE(entry, list_ent_lookup);

	free(entry);
}

/* Delete an entry in a table by its identfier */
static void xdp2_dtable_del_by_id(struct xdp2_dtable_table *table,
				   unsigned int ident)
{
	struct xdp2_dtable_entry *entry;

	entry = xdp2_dtable_find_ent_by_id(table, ident);
	if (entry)
		xdp2_dtable_del(table, entry);
}

/* Change a table entry in place (change function or argument) */
static void xdp2_dtable_change(struct xdp2_dtable_table *table,
				struct xdp2_dtable_entry *entry,
				void *target)
{
	memcpy(XDP2_DTABLE_TARG(table, entry), target, table->targ_len);
}

/* Change a table entry in place by its identfier */
static int xdp2_dtable_change_by_id(struct xdp2_dtable_table *table,
				     unsigned int ident, void *target)
{
	struct xdp2_dtable_entry *entry;

	entry = xdp2_dtable_find_ent_by_id(table, ident);
	if (!entry)
		return -ENOENT;

	xdp2_dtable_change(table, entry, target);

	return 0;
}

/* Create a dynamic plain table */
struct xdp2_dtable_plain_table *xdp2_dtable_create_plain(
		const char *name, size_t key_len, void *default_target,
		size_t target_len, int *ident)
{
	return (struct xdp2_dtable_plain_table *)
	    xdp2_dtable_create(name, key_len,
			key_len + sizeof(struct xdp2_plain_entry_key), ident,
			sizeof(struct xdp2_dtable_plain_table),
			default_target, target_len, &plain_tables);
}

/* Find a matching entry by key for a plain table and return it. Also
 * return a pointer to the previous next for deletion
 */
static struct xdp2_dtable_entry *__xdp2_dtable_find_plain(
		struct xdp2_dtable_plain_table *table, const void *key,
		struct xdp2_dtable_entry **pprev, __u64 *_hash)
{
	struct xdp2_dtable_entry *entry, *prev = NULL;
	__u64 hash;

	hash = siphash(key, table->key_len, &siphash_key);
	if (_hash)
		*_hash = hash;

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup) {
		struct xdp2_plain_entry_key *keyinfo =
			(struct xdp2_plain_entry_key *)
				XDP2_DTABLE_KEY(table, entry);

		if (keyinfo->hash > hash) {
			entry = NULL;
			break;
		}

		if (keyinfo->hash == hash &&
		    xdp2_compare_equal(keyinfo->key, key, table->key_len))
			break;
		prev = entry;
	}

	if (pprev)
		*pprev = prev;

	return entry;
}

/* Add an entry to a plain table */
int xdp2_dtable_add_plain(struct xdp2_dtable_plain_table *table,
		int ident, const void *key, void *target)
{
	struct xdp2_dtable_entry *prev_entry, *entry;
	struct xdp2_plain_entry_key *keyinfo;
	__u64 hash;

	if (__xdp2_dtable_find_plain(table, key, &prev_entry, &hash))
		return -EALREADY;

	entry = xdp2_dtable_add((struct xdp2_dtable_table *)table,
				 prev_entry, &ident, target);
	if (!entry)
		return -ENOMEM;

	keyinfo = (struct xdp2_plain_entry_key *)
			XDP2_DTABLE_KEY(table, entry);
	keyinfo->hash = hash;
	memcpy(keyinfo->key, key, table->key_len);

	return ident;
}

/* Delete an entry from a plain table */
void xdp2_dtable_del_plain(struct xdp2_dtable_plain_table *ptable,
			    const void *key)
{
	struct xdp2_dtable_entry *entry;

	entry = __xdp2_dtable_find_plain(ptable, key, NULL, NULL);
	if (entry)
		xdp2_dtable_del((struct xdp2_dtable_table *)ptable, entry);
}

/* Delete an entry from a plain table by its identifier */
void xdp2_dtable_del_plain_by_id(struct xdp2_dtable_plain_table *ptable,
				  int ident)
{
	xdp2_dtable_del_by_id((struct xdp2_dtable_table *)ptable, ident);
}

/* Change an entry in a plain table */
int xdp2_dtable_change_plain(struct xdp2_dtable_plain_table *ptable,
			      const void *key, void *target)
{
	struct xdp2_dtable_entry *entry;

	entry = __xdp2_dtable_find_plain(ptable, key, NULL, NULL);
	if (!entry)
		return -ENOENT;

	xdp2_dtable_change((struct xdp2_dtable_table *)ptable,
			    entry, target);

	return 0;
}

/* Change an entry in a plain table by its identifier*/
int xdp2_dtable_change_plain_by_id(struct xdp2_dtable_plain_table *ptable,
				    int ident, void *target)
{
	return xdp2_dtable_change_by_id((struct xdp2_dtable_table *)ptable,
					 ident, target);

	return 0;
}

/* Perform a lookup in a plain table */
const void *xdp2_dtable_lookup_plain(struct xdp2_dtable_plain_table *table,
				      const void *key)
{
	struct xdp2_dtable_entry *entry;
	__u64 hash;

	hash = siphash(key, table->key_len, &siphash_key);

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup) {
		struct xdp2_plain_entry_key *keyinfo =
			(struct xdp2_plain_entry_key *)
				XDP2_DTABLE_KEY(table, entry);

		if (keyinfo->hash > hash)
			break;

		if (keyinfo->hash == hash &&
		    xdp2_compare_equal(keyinfo->key, key, table->key_len))
			return XDP2_DTABLE_TARG(table, entry);
	}

	return table->default_target;
}

/* Dynamic ternary tables */

/* Create a dynamic tern table */
struct xdp2_dtable_tern_table *xdp2_dtable_create_tern(
		const char *name, size_t key_len, void *default_target,
		size_t target_len, int *ident)
{
	return (struct xdp2_dtable_tern_table *)
	    xdp2_dtable_create(name, key_len, 2 * key_len +
				sizeof(struct xdp2_tern_entry_key), ident,
				sizeof(struct xdp2_dtable_tern_table),
				default_target, target_len,
				&tern_tables);
}

/* Find a matching entry by key and key-mask and return it. Also return a
 * pointer to the previous next for deletion
 */
static struct xdp2_dtable_tern_entry *__xdp2_dtable_find_tern(
		struct xdp2_dtable_tern_table *table, const void *key,
		const void *key_mask, unsigned int pos,
		struct xdp2_dtable_entry **pprev)
{
	struct xdp2_dtable_entry *entry, *prev = NULL;

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup) {
		struct xdp2_tern_entry_key *keyinfo =
			(struct xdp2_tern_entry_key *)
				XDP2_DTABLE_KEY(table, entry);

		if (keyinfo->pos > pos) {
			entry = NULL;
			break;
		}

		if (xdp2_compare_equal(keyinfo->key, key, table->key_len) &&
		    xdp2_compare_equal(keyinfo->key + table->key_len, key_mask,
				       table->key_len))
			break;
		prev = entry;
	}

	if (pprev)
		*pprev = prev;

	return (struct xdp2_dtable_tern_entry *)entry;
}

/* Add an entry to a ternary table */
int xdp2_dtable_add_tern(struct xdp2_dtable_tern_table *table,
			  int ident, const void *key, const void *key_mask,
			  unsigned int pos, void *target)
{
	struct xdp2_dtable_entry *prev_entry, *entry;
	struct xdp2_tern_entry_key *keyinfo;

	if (__xdp2_dtable_find_tern(table, key, key_mask, pos, &prev_entry))
		return -EALREADY;

	entry = xdp2_dtable_add((struct xdp2_dtable_table *)table, prev_entry,
				 &ident, target);
	if (!entry)
		return -ENOMEM;

	keyinfo = (struct xdp2_tern_entry_key *)
			XDP2_DTABLE_KEY(table, entry);
	keyinfo->pos = pos;
	memcpy(keyinfo->key, key, table->key_len);
	memcpy(keyinfo->key + table->key_len, key_mask, table->key_len);

	return 0;
}

/* Delete an entry from a ternary table */
void xdp2_dtable_del_tern(struct xdp2_dtable_tern_table *ttable,
			   const void *key, const void *key_mask,
			   unsigned int pos)
{
	struct xdp2_dtable_tern_entry *tentry;

	tentry = __xdp2_dtable_find_tern(ttable, key, key_mask, pos, NULL);
	if (tentry)
		xdp2_dtable_del((struct xdp2_dtable_table *)ttable,
				    (struct xdp2_dtable_entry *)tentry);
}

void xdp2_dtable_del_tern_by_id(struct xdp2_dtable_tern_table *ttable,
				 int ident)
{
	xdp2_dtable_del_by_id((struct xdp2_dtable_table *)ttable, ident);
}

/* Change an entry in a ternary table */
int xdp2_dtable_change_tern(struct xdp2_dtable_tern_table *ttable,
			     const void *key, const void *key_mask,
			     unsigned int pos, void *target)
{
	struct xdp2_dtable_tern_entry *tentry;

	tentry = __xdp2_dtable_find_tern(ttable, key, key_mask, pos, NULL);
	if (!tentry)
		return -ENOENT;

	xdp2_dtable_change((struct xdp2_dtable_table *)ttable,
			    (struct xdp2_dtable_entry *)tentry, target);

	return 0;
}

/* Change an entry in a ternary table by its identifier */
int xdp2_dtable_change_tern_by_id(struct xdp2_dtable_tern_table *ttable,
				   int ident, void *target)
{
	return xdp2_dtable_change_by_id((struct xdp2_dtable_table *)ttable,
					 ident, target);

	return 0;
}

/* Perform a lookup in a ternary table */
const void *xdp2_dtable_lookup_tern(struct xdp2_dtable_tern_table *table,
			      const void *key)
{
	struct xdp2_dtable_entry *entry;

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup) {
		struct xdp2_tern_entry_key *keyinfo =
			(struct xdp2_tern_entry_key *)
				XDP2_DTABLE_KEY(table, entry);

		if (xdp2_compare_tern(keyinfo->key, key,
				      keyinfo->key + table->key_len,
				      table->key_len))
			return XDP2_DTABLE_TARG(table, entry);
	}

	return table->default_target;
}

/* Dynamic LPM tables */

/* Create a dynamic lpm table */
struct xdp2_dtable_lpm_table *xdp2_dtable_create_lpm(
		const char *name, size_t key_len, void *default_target,
		size_t target_len, int *ident)
{
	return (struct xdp2_dtable_lpm_table *)
	    xdp2_dtable_create(name, key_len,
				key_len + sizeof(struct xdp2_lpm_entry_key),
				ident, sizeof(struct xdp2_dtable_lpm_table),
				default_target, target_len, &lpm_tables);
}

/* Find a matching entry in a longest prefix match table and return it. Also
 * return a pointer to the previous next for deletion. The list is ordered
 * by descending prefix length
 */
static struct xdp2_dtable_lpm_entry *__xdp2_dtable_find_lpm(
		struct xdp2_dtable_lpm_table *table, const void *key,
		size_t prefix_len, struct xdp2_dtable_entry **pprev)
{
	struct xdp2_dtable_entry *entry, *prev = NULL;

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup) {
		struct xdp2_lpm_entry_key *keyinfo =
			(struct xdp2_lpm_entry_key *)
				XDP2_DTABLE_KEY(table, entry);

		if (keyinfo->prefix_len < prefix_len) {
			entry = NULL;
			break;
		}

		if (xdp2_compare_equal(keyinfo->key, key, table->key_len) &&
		    keyinfo->prefix_len == prefix_len)
			break;
		prev = entry;
	}

	if (pprev)
		*pprev = prev;

	return (struct xdp2_dtable_lpm_entry *)entry;
}

/* Add an entry to a longest prefix match table */
int xdp2_dtable_add_lpm(struct xdp2_dtable_lpm_table *table,
			 int ident, const void *key, size_t prefix_len,
			 void *target)
{
	struct xdp2_dtable_entry *prev_entry, *entry;
	struct xdp2_lpm_entry_key *keyinfo;

	if (__xdp2_dtable_find_lpm(table, key, prefix_len, &prev_entry))
		return -EALREADY;

	entry = xdp2_dtable_add((struct xdp2_dtable_table *)table,
				 prev_entry, &ident, target);
	if (!entry)
		return -ENOMEM;

	keyinfo = (struct xdp2_lpm_entry_key *)
			XDP2_DTABLE_KEY(table, entry);

	keyinfo->prefix_len = prefix_len;
	memcpy(keyinfo->key, key, table->key_len);

	return 0;
}

/* Delete an entry from a longest prefix match table */
void xdp2_dtable_del_lpm(struct xdp2_dtable_lpm_table *ltable,
			  const void *key, size_t prefix_len)
{
	struct xdp2_dtable_lpm_entry *lentry;

	lentry = __xdp2_dtable_find_lpm(ltable, key, prefix_len, NULL);
	if (lentry)
		xdp2_dtable_del((struct xdp2_dtable_table *)ltable,
				 (struct xdp2_dtable_entry *)lentry);
}

/* Delete an entry from a longest prefix match table by its identifier*/
void xdp2_dtable_del_lpm_by_id(struct xdp2_dtable_lpm_table *ltable,
				int ident)
{
	xdp2_dtable_del_by_id((struct xdp2_dtable_table *)ltable, ident);
}

/* Change an entry in a longest prefix match table */
int xdp2_dtable_change_lpm(struct xdp2_dtable_lpm_table *ltable,
			    const void *key, size_t prefix_len,
			    void *target)
{
	struct xdp2_dtable_lpm_entry *lentry;

	lentry = __xdp2_dtable_find_lpm(ltable, key, prefix_len, NULL);
	if (!lentry)
		return -ENOENT;

	xdp2_dtable_change((struct xdp2_dtable_table *)ltable,
			    (struct xdp2_dtable_entry *)lentry, target);

	return 0;
}

/* Change an entry in a longest prefix match table by its identifier */
int xdp2_dtable_change_lpm_by_id(struct xdp2_dtable_lpm_table *ltable,
				  int ident, void *target)
{
	return xdp2_dtable_change_by_id((struct xdp2_dtable_table *)ltable,
					 ident, target);
}

/* Perform a lookup in a longest prefix match table. Note that this list
 * is sorted by descending prefix length, so the first match found will be
 * the correct one
 */
const void *xdp2_dtable_lookup_lpm(struct xdp2_dtable_lpm_table *table,
				    const void *key)
{
	struct xdp2_dtable_entry *entry;

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup) {
		struct xdp2_lpm_entry_key *keyinfo =
			(struct xdp2_lpm_entry_key *)
				XDP2_DTABLE_KEY(table, entry);

		if (xdp2_compare_prefix(keyinfo->key, key,
					keyinfo->prefix_len))
			return XDP2_DTABLE_TARG(table, entry);
	}

	return table->default_target;
}

/* Create a dummy table to ensure that the section is defined (if no
 * objects for a section are ever dfined then there will be a linker error).
 * This entry is distinguished by table_type which is 0 (invalid type)
 */
static struct xdp2_dtable_table XDP2_SECTION_ATTR(
				xdp2_dtable_section_tables) dummy_table;

/* Print one key */
static void __xdp2_dtable_print_key(void *cli,
				     struct xdp2_dtable_table *table,
				     const void *_key)
{
	__u8 *key = (__u8 *)_key;
	int i;

	XDP2_CLI_PRINT(cli, "<");
	for (i = 0; i < table->key_len; i++) {
		if (i != 0)
			XDP2_CLI_PRINT(cli, " ");
		XDP2_CLI_PRINT(cli, "0x%02x", key[i]);
	}
	XDP2_CLI_PRINT(cli, ">");
}

/* Print one plain entry */
static void xdp2_dtable_print_plain_entry(void *cli,
					   struct xdp2_dtable_table *table,
					   struct xdp2_dtable_entry *entry)
{
	struct xdp2_plain_entry_key *keyinfo =
		(struct xdp2_plain_entry_key *)
			XDP2_DTABLE_KEY(table, entry);

	XDP2_CLI_PRINT(cli, "\tEntry: ident: %u\n", entry->ident);
	XDP2_CLI_PRINT(cli, "\t\tHash: 0x%016llx\n", keyinfo->hash);
	XDP2_CLI_PRINT(cli, "\t\tKey: ");
	__xdp2_dtable_print_key(cli, table, keyinfo->key);
	XDP2_CLI_PRINT(cli, "\n");
}

/* Print one ternary entry */
static void xdp2_dtable_print_tern_entry(void *cli,
					  struct xdp2_dtable_table *table,
					  struct xdp2_dtable_entry *entry)
{
	struct xdp2_tern_entry_key *keyinfo =
		(struct xdp2_tern_entry_key *)
			XDP2_DTABLE_KEY(table, entry);

	XDP2_CLI_PRINT(cli, "\tEntry: ident: %u\n", entry->ident);
	XDP2_CLI_PRINT(cli, "\t\tKey     : ");
	__xdp2_dtable_print_key(cli, table, keyinfo->key);
	XDP2_CLI_PRINT(cli, "\n\t\tKey-mask: ");
	__xdp2_dtable_print_key(cli, table, keyinfo->key + table->key_len);
	XDP2_CLI_PRINT(cli, "\n\t\tPosition: %u\n", keyinfo->pos);
}

/* Print one longest prefix match entry */
static void xdp2_dtable_print_lpm_entry(void *cli,
					 struct xdp2_dtable_table *table,
					 struct xdp2_dtable_entry *entry)
{
	struct xdp2_lpm_entry_key *keyinfo =
		(struct xdp2_lpm_entry_key *)
			XDP2_DTABLE_KEY(table, entry);

	XDP2_CLI_PRINT(cli, "\tEntry: ident: %u\n", entry->ident);
	XDP2_CLI_PRINT(cli, "\t\tKey: ");
	__xdp2_dtable_print_key(cli, table, keyinfo->key);
	XDP2_CLI_PRINT(cli, "\n\t\tPrefix length: %lu\n", keyinfo->prefix_len);
}

/* Print one plain table */
static void xdp2_dtable_print_one_plain_table(void *cli,
					struct xdp2_dtable_table *table)
{
	struct xdp2_dtable_entry *entry;

	XDP2_CLI_PRINT(cli, "Plain table %s: ident %d, key length %lu,\n",
	       table->name, table->ident, table->key_len);

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup)
		xdp2_dtable_print_plain_entry(cli, table, entry);
}

/* Print one ternary table */
static void xdp2_dtable_print_one_tern_table(void *cli,
				       struct xdp2_dtable_table *table)
{
	struct xdp2_dtable_entry *entry;

	XDP2_CLI_PRINT(cli, "Ternary table %s: ident %u\n",
		       table->name, table->ident);

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup)
		xdp2_dtable_print_tern_entry(cli, table, entry);
}

/* Print one longest prefix match table */
static void xdp2_dtable_print_one_lpm_table(void *cli,
				      struct xdp2_dtable_table *table)
{
	struct xdp2_dtable_entry *entry;

	XDP2_CLI_PRINT(cli, "Longest prefix match table %s: ident %u\n",
	       table->name, table->ident);

	LIST_FOREACH(entry, &table->entries_lookup, list_ent_lookup)
		xdp2_dtable_print_lpm_entry(cli, table, entry);
}

/* Print all plain tables */
static void __xdp2_dtable_print_plain_tables(void *cli)
{
	struct xdp2_dtable_table *table;

	LIST_FOREACH(table, &plain_tables, list_ent)
		xdp2_dtable_print_one_plain_table(cli, table);
}

/* Print all ternary tables */
static void __xdp2_dtable_print_tern_tables(void *cli)
{
	struct xdp2_dtable_table *table;

	LIST_FOREACH(table, &tern_tables, list_ent)
		xdp2_dtable_print_one_tern_table(cli, table);
}

/* Print all longest prefix match tables */
static void __xdp2_dtable_print_lpm_tables(void *cli)
{
	struct xdp2_dtable_table *table;

	LIST_FOREACH(table, &lpm_tables, list_ent)
		xdp2_dtable_print_one_lpm_table(cli, table);
}

/* List one table */
static void __xdp2_dtable_list_tables(void *cli,
		struct __xdp2_dtable_table_listhead *list_head,
		const char *type)
{
	struct xdp2_dtable_table *table;

	LIST_FOREACH(table, list_head, list_ent)
		XDP2_CLI_PRINT(cli, "%s %s:%u\n", table->name, type,
			       table->ident);
}

/* Print all tables */
void xdp2_dtable_print_all_tables(void)
{
	__xdp2_dtable_print_plain_tables(NULL);
	__xdp2_dtable_print_tern_tables(NULL);
	__xdp2_dtable_print_lpm_tables(NULL);
}

/* Show all tables from CLI */
static void xdp2_dtable_show_all_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	__xdp2_dtable_print_plain_tables(cli);
	__xdp2_dtable_print_tern_tables(cli);
	__xdp2_dtable_print_lpm_tables(cli);
}

/* Show all plain tables or one table from CLI */
static void xdp2_dtable_show_plain_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	struct xdp2_dtable_table *table;
	int ident;

	if (arg) {
		ident = strtol((char *)arg, NULL, 0);

		table = __xdp2_dtable_find_table(&ident, NULL, &plain_tables);
		if (table)
			xdp2_dtable_print_one_plain_table(cli, table);
	} else {
		__xdp2_dtable_print_plain_tables(cli);
	}
}

/* Show all ternary tables or one table from CLI */
static void xdp2_dtable_show_tern_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	struct xdp2_dtable_table *table;
	int ident;

	if (arg) {
		ident = strtol((char *)arg, NULL, 0);

		table = __xdp2_dtable_find_table(&ident, NULL, &tern_tables);
		if (table)
			xdp2_dtable_print_one_tern_table(cli, table);
	} else {
		__xdp2_dtable_print_tern_tables(cli);
	}
}

/* Show all longest prefix match tables or one table from CLI */
static void xdp2_dtable_show_lpm_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	struct xdp2_dtable_table *table;
	int ident;

	if (arg) {
		ident = strtol(arg, NULL, 0);

		table = __xdp2_dtable_find_table(&ident, NULL, &lpm_tables);
		if (table)
			xdp2_dtable_print_one_lpm_table(cli, table);
	} else {
		__xdp2_dtable_print_lpm_tables(cli);
	}
}

XDP2_CLI_ADD_SHOW_CONFIG("all", xdp2_dtable_show_all_cli, 0xffff);
XDP2_CLI_ADD_SHOW_CONFIG_ARGOK("plain", xdp2_dtable_show_plain_cli, 0xffff);
XDP2_CLI_ADD_SHOW_CONFIG_ARGOK("tern", xdp2_dtable_show_tern_cli, 0xffff);
XDP2_CLI_ADD_SHOW_CONFIG_ARGOK("lpm", xdp2_dtable_show_lpm_cli, 0xffff);

/* List all tables */
static void xdp2_dtable_show_table_list_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	__xdp2_dtable_list_tables(cli, &plain_tables, "plain");
	__xdp2_dtable_list_tables(cli, &tern_tables, "tern");
	__xdp2_dtable_list_tables(cli, &lpm_tables, "lpm");
}

/* List plain tables */
static void xdp2_dtable_show_table_list_plain_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	__xdp2_dtable_list_tables(cli, &plain_tables, "plain");
}

/* List ternary tables */
static void xdp2_dtable_show_table_list_tern_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	__xdp2_dtable_list_tables(cli, &tern_tables, "tern");
}

/* List longest prefix match tables */
static void xdp2_dtable_show_table_list_lpm_cli(void *cli,
		struct xdp2_cli_thread_info *info,
		const char *arg)
{
	__xdp2_dtable_list_tables(cli, &lpm_tables, "lpm");
}

XDP2_CLI_ADD_SHOW_CONFIG("list", xdp2_dtable_show_table_list_cli, 0xffff);
XDP2_CLI_ADD_SHOW_CONFIG("list-plain", xdp2_dtable_show_table_list_plain_cli,
			 0xffff);
XDP2_CLI_ADD_SHOW_CONFIG("list-tern", xdp2_dtable_show_table_list_tern_cli,
			 0xffff);
XDP2_CLI_ADD_SHOW_CONFIG("list-lpm", xdp2_dtable_show_table_list_lpm_cli,
			 0xffff);

/* Initialize dynamic tables */
void xdp2_dtable_init(void)
{
	int num_els = xdp2_section_array_size_xdp2_dtable_section_tables();
	struct xdp2_dtable_table *def_base =
		xdp2_section_base_xdp2_dtable_section_tables();
	int ident = 0;
	int i;

	LIST_INIT(&plain_tables);
	LIST_INIT(&tern_tables);
	LIST_INIT(&lpm_tables);

	/* Create tables that were defined in a section */
	for (i = 0; i < num_els; i++) {
		LIST_INIT(&def_base[i].entries);
		LIST_INIT(&def_base[i].entries_lookup);

		switch (def_base[i].table_type) {
		case XDP2_DTABLE_TABLE_TYPE_PLAIN:
				ident = 0;
				if (__xdp2_dtable_insert_table(&def_base[i],
							       &ident,
							       &plain_tables))
					XDP2_ERR(1, "Initialization of "
						    "dynamic tables from "
						    "section failed\n");
				break;
		case XDP2_DTABLE_TABLE_TYPE_TERN:
				ident = 0;
				if (__xdp2_dtable_insert_table(&def_base[i],
							       &ident,
							       &tern_tables))
					XDP2_ERR(1, "Initialization of "
						    "dynamic tables from "
						    "section failed\n");
				break;
		case XDP2_DTABLE_TABLE_TYPE_LPM:
			ident = 0;
			if (__xdp2_dtable_insert_table(&def_base[i], &ident,
							&lpm_tables))
				XDP2_ERR(1, "Initialization of dynamic tables "
					    "from section failed\n");
			break;
		case XDP2_DTABLE_TABLE_TYPE_INVALID:
			/* Probably the dummy entry, just skip */
			continue;

		default:
			XDP2_ERR(1, "Unknown type %u in initialization of "
				    "dynamic tables\n", def_base[i].table_type);
		}
	}
}
