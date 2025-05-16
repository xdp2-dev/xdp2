// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2024 Tom Herbert
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

/* Support functions for variable structures */

#include <stdbool.h>
#include <string.h>

#include "xdp2/utility.h"
#include "xdp2/vstruct.h"

/* Find a entry in a def_vsmap by name */
const struct xdp2_vstruct_def_entry *xdp2_vstruct_find_def_entry(
		const struct xdp2_vstruct_def_generic *def_vsmap,
		const char *name, int *index)
{
	int i;
	const struct xdp2_vstruct_def_entry *def_entry;

	for (i = 0; i < def_vsmap->num_els; i++) {
		def_entry = &def_vsmap->entries[i];
		if (!strcmp(def_entry->name, name)) {
			*index = i;
			return def_entry;
		}
	}

	*index = -1;
	return NULL;
}

/* Print one entry in a vsmap */
void xdp2_vstruct_print_one(void *cli, char *indent,
			     struct xdp2_vstruct_generic *vsmap,
			     struct xdp2_vstruct_def_generic *def_vsmap,
			     int index, unsigned int offset, char *vsmap_name,
			     unsigned int base_offset, bool no_zero_size,
			     bool short_output, bool csv, bool label)
{
	const struct xdp2_vstruct_def_entry *def_entry;
	unsigned int rel_offset = offset - base_offset;

	if (!def_vsmap) {
		XDP2_CLI_PRINT(cli, "Field <>, offset %u, rel-offset %u\n",
				offset, rel_offset);
		return;
	}

	if (index >= 0) {
		def_entry = &def_vsmap->entries[index];
	} else {
		def_entry = xdp2_vstruct_find_def_entry(def_vsmap,
			vsmap_name, &index);
		if (!def_entry) {
			XDP2_WARN("%s not found in def_map\n", vsmap_name);
			return;
		}
	}

	if (vsmap) {
		unsigned int off1 = vsmap->offsets[index] - vsmap->offsets[0];
		unsigned int off2 = offset - base_offset;

		XDP2_ASSERT(off1 == off2, "Offsets mismatch for %s: "
					   "%u != %u\n", vsmap_name,
			     off1, off2);
	}

	if (no_zero_size && !def_entry->num_els)
		return;

	if (!csv) {
		XDP2_CLI_PRINT(cli, "%s%s, all_size: %u num_els: %u, "
				     "align: %u, struct_size: %u, "
				     "offset: %u, rel-offset: %u\n",
					indent, def_entry->name,
				def_entry->num_els *
					def_entry->struct_size,
				def_entry->num_els,
				def_entry->align,
				def_entry->struct_size,
				offset, rel_offset);
	} else if (!short_output) {
		XDP2_CLI_PRINT(cli, "%s%s,%u,%u,%u,%u,%u,%u\n",
				indent, def_entry->name,
				def_entry->num_els *
					def_entry->struct_size,
				def_entry->num_els,
				def_entry->align,
				def_entry->struct_size,
				offset, rel_offset);
	} else {
		XDP2_CLI_PRINT(cli, "%s%s,%u,%u\n",
				indent, def_entry->name,
				offset, rel_offset);
	}
}

/* Print a vsmap */
void xdp2_vstruct_print_vsmap(void *cli, char *indent,
			      struct xdp2_vstruct_def_generic *def_vsmap,
			      struct xdp2_vstruct_generic *vsmap,
			      char *vsmap_name, unsigned int base_offset,
			      bool no_zero_size, bool short_output, bool csv,
			      bool label)
{
	int i;

	if (no_zero_size && def_vsmap) {
		int count = 0;

		for (i = 0; i < def_vsmap->num_els; i++) {
			struct xdp2_vstruct_def_entry *def_entry =
						&def_vsmap->entries[i];

			if (def_entry->num_els)
				count++;
		}
		if (!short_output && !csv)
			XDP2_CLI_PRINT(cli, "%sVsmap %s: %u fields, "
					     "(%u total fields)\n",
					indent, vsmap_name, count,
					def_vsmap->num_els);
	} else {
		if (!short_output && !csv)
			XDP2_CLI_PRINT(cli, "%sVsmap %s: %u fields\n",
					indent, vsmap_name, def_vsmap->num_els);
	}

	if (csv && label) {
		if (def_vsmap)
			XDP2_CLI_PRINT(cli, "field-name,all_size,num_els,"
					     "align,struct_size,offset,"
					     "rel-offset\n");
		else
			XDP2_CLI_PRINT(cli, "field-name,offset,rel-offset\n");
	}

	for (i = 0; i < def_vsmap->num_els; i++)
		xdp2_vstruct_print_one(cli, indent, vsmap, def_vsmap, i,
					vsmap->offsets[i], vsmap_name,
					base_offset, no_zero_size,
					short_output, csv, label);
}

/* Instantiate a vsmap for a variable structure. The offsets for the various
 * fields are set per the length and alignments specified in def_vsmap
 */
size_t xdp2_vstruct_instantiate_vsmap(void *_vsmap, void *_def_vsmap,
				       unsigned int num_els, size_t offset)
{
	struct xdp2_vstruct_def_generic *def_vsmap = _def_vsmap;
	struct xdp2_vstruct_generic *vsmap = _vsmap;
	int i;

	for (i = 0; i < num_els; i++) {
		if (def_vsmap->entries[i].align)
			offset = xdp2_round_up(offset,
						def_vsmap->entries[i].align);
		vsmap->offsets[i] = offset;
		offset += def_vsmap->entries[i].num_els *
			  def_vsmap->entries[i].struct_size;
	}

	return offset;
}

/* Count the number of entries in a vsmap */
unsigned int xdp2_vstruct_count_vsmap(struct xdp2_vstruct_generic *vsmap,
				       struct xdp2_vstruct_def_generic
								*def_vsmap,
				       unsigned int *_zero_count)
{
	unsigned int zero_count = 0;
	int i;

	for (i = 0; i < def_vsmap->num_els; i++) {
		if (def_vsmap->entries[i].num_els)
			zero_count++;
	}

	*_zero_count = zero_count;

	return def_vsmap->num_els;
}
