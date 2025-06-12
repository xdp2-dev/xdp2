// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2023 Tom Herbert
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

/* Configuration functions, include after config.h */

#include <ctype.h>
#include <errno.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "xdp2/config.h"
#include "xdp2/utility.h"

/* Helper to convert a string to lower case. Note this returns pointer to
 * a static variable
 */
static const char *strlc(const char *str)
{
	static char string[128+1];
	int i;

	for (i = 0; str[i] && i < 128; i++)
		string[i] = tolower(str[i]);

	string[i] = 0;

	return string;
}
/* Read a configuration parameter from a config structure */
static __u64 config_read_val(const void *config, size_t offset, size_t size)
{
	const void *base = config + offset;

	switch (size) {
	case sizeof(__u8):
		return *(__u8 *)base;
	case sizeof(__u16):
		return *(__u16 *)base;
	case sizeof(__u32):
		return *(__u32 *)base;
	case sizeof(__u64):
		return *(__u64 *)base;
	default:
		XDP2_ERR(1, "Base size in configuration descriptor: %lu\n",
			  size);
	}
}

/* Write a configuration parameter to a config structure */
static void config_write_val(void *config, size_t offset, size_t size,
			     __u64 val)
{
	void *base = config + offset;

	switch (size) {
	case sizeof(__u8):
		*(__u8 *)base  = (__u8)val;
		break;
	case sizeof(__u16):
		*(__u16 *)base = (__u16)val;
		break;
	case sizeof(__u32):
		*(__u32 *)base = (__u32)val;
		break;
	case sizeof(__u64):
		*(__u64 *)base = (__u64)val;
		break;
	default:
		XDP2_ERR(1, "Base size in configuration descriptor: %lu\n",
			  size);
	}
}

#if 0
/* Read bytes of a configuration parameter from a config structure */
static void config_read_bytes(void *dest, const void *config,
			      size_t offset, size_t size)
{
	memcpy(dest, config + offset, size);
}

/* Write bytes of a configuration parameter to a config structure */
static void config_write_bytes(void *config, size_t offset,
			       void *src, size_t size)
{
	memcpy(config + offset, src, size);
}
#endif

/* Find a configuration descriptor in a descriptor table by name */
static const struct xdp2_config_desc *find_config_desc(
		const struct xdp2_config_desc_table *config_table,
		const char *conf_name)
{
	int i;

	for (i = 0; i < config_table->table_size; i++) {
		const struct xdp2_config_desc *desc = &config_table->table[i];

		if (!strcmp(strlc(desc->conf_name), conf_name))
			return desc;
	}
	return NULL;
}

/* Print a configuration parameter value */
static void print_one_config(void *cli, char *indent, const void *config,
			     const struct xdp2_config_desc *desc)
{
	__u64 val;

	val = config_read_val(config, desc->offset, desc->size);
	if (!val)
		val = desc->derived_func(config);

	if (strcasestr(desc->text, "bitmap")) {
		XDP2_CLI_PRINT(cli, "%s%s: 0x%llx\n", indent,
				strlc(desc->conf_name), val);
	} else if (strcasestr(desc->text, "IPv4 address")) {
		struct in_addr inaddr;

		inaddr.s_addr = val;
		XDP2_CLI_PRINT(cli, "%s%s: %s\n", indent,
				strlc(desc->conf_name),
				inet_ntoa(inaddr));
	} else {
		XDP2_CLI_PRINT(cli, "%s%s: %llu\n", indent,
				strlc(desc->conf_name), val);
	}
}

/* Check a configuration parameter against the limits in the configuration
 * descriptor for the parameter
 */
void xdp2_config_check_config_one(const struct xdp2_config_desc_table
							*config_table,
				   const char *text, bool *rval,
				   const char *conf_name, unsigned int value)
{
	const struct xdp2_config_desc *desc =
		find_config_desc(config_table, conf_name);

	if (!desc) {
		XDP2_WARN("%s%s: configuration parameter not found",
			   text, conf_name);
		*rval = true;
		return;
	}

	if (value < desc->min) {
		XDP2_WARN("%s%s: %u is less than minimum %llu",
			   text, conf_name, value, desc->min);
		*rval = true;
		return;
	}

	if (desc->max != -1U && value > desc->max) {
		XDP2_WARN("%s%s: %u is greater than maximum %llu",
			   text, conf_name, value, desc->max);
		*rval = true;
		return;
	}
}

/* Print the derived configuration parameters in a descriptor table */
void xdp2_config_print_derived_config(
		const struct xdp2_config_desc_table *config_table,
		void *cli, char *indent, const void *config, char *conf_name,
		unsigned long long derived_val,
		unsigned int derived_low_water_mark)
{
	const struct xdp2_config_desc *desc =
		find_config_desc(config_table, conf_name);
	__u64 val;

	if (!desc) {
		XDP2_CLI_PRINT(cli, "%s%s: No config value, derived: %llu\n",
				indent, conf_name, derived_val);
		return;
	}

	val = config_read_val(config, desc->offset, desc->size);
	if (!val)
		val = desc->derived_func(config);

	if (strcasestr(desc->text, "bitmap")) {
		XDP2_CLI_PRINT(cli, "%s%s: 0x%llx, derived: 0x%llx\n",
				indent, conf_name, val,
				derived_val);
	} else if (strcasestr(desc->text, "IPv4 address")) {
		struct in_addr inaddr1, inaddr2;

		inaddr1.s_addr = val;
		inaddr2.s_addr = (unsigned int)derived_val;
		XDP2_CLI_PRINT(cli, "%s%s: %s, derived: %s\n", indent,
				conf_name, inet_ntoa(inaddr1),
				inet_ntoa(inaddr2));
	} else {
		XDP2_CLI_PRINT(cli, "%s%s: %llu, derived: %llu\n",
				indent, conf_name, val,
				derived_val);
	}
}

/* Print all configuration parameters in one configuration structure */
void xdp2_config_print_config(const struct xdp2_config_desc_table *config_table,
			       void *cli, char *indent, const void *config)
{
	int i;

	for (i = 0; i < config_table->table_size; i++) {
		const struct xdp2_config_desc *desc = &config_table->table[i];

		print_one_config(cli, indent, config, desc);
	}
}

/* Print a configuration parameter in csv format. This can be used for
 * saving a configuration
 */
void xdp2_config_print_config_csv(const struct xdp2_config_desc_table
								*config_table,
				   void *cli, const void *config, bool key)
{
	__u64 val;
	int i;

	if (key) {
		XDP2_CLI_PRINT(cli, "# Key: Bitmap name,bitmap,description\n");
		XDP2_CLI_PRINT(cli, "# Key: IP4addr,name,addr,description\n");
		XDP2_CLI_PRINT(cli, "# Key: Value,name,value,min,max,"
				     "description\n");
		XDP2_CLI_PRINT(cli, "# Key: Fifo,name,limit,"
				     "low-water-mark,description\n");
	}

	for (i = 0; i < config_table->table_size; i++) {
		const struct xdp2_config_desc *desc = &config_table->table[i];

		val = config_read_val(config, desc->offset, desc->size);
		if (strcasestr(desc->text, "bitmap")) {
			XDP2_CLI_PRINT(cli,
					"Bitmap,%s,0x%llx,\"%s\"\n",
					strlc(desc->conf_name), val,
					desc->text);
		} else if (strcasestr(desc->text, "IPv4 address")) {
			struct in_addr inaddr;

			inaddr.s_addr = val;
			XDP2_CLI_PRINT(cli, "IP4addr,%s,%s,\"%s\"\n",
					strlc(desc->conf_name),
					inet_ntoa(inaddr), desc->text);
		} else {
			XDP2_CLI_PRINT(cli, "Value,%s,%llu,%llu,%llu,"
					     "\"%s\"\n",
					strlc(desc->conf_name), val,
					desc->min, desc->max,
					desc->text);
		}
	}
}

/* Print a configuration parameter in a short csv format. This can be used for
 * saving a configuration
 */
void xdp2_config_print_config_csv_short(const struct xdp2_config_desc_table
								*config_table,
					 void *cli, const void *config,
					 char sep, bool key)
{
	__u64 val;
	int i;

	if (key && sep != '=')
		XDP2_CLI_PRINT(cli, "# Key: name,value\n");

	for (i = 0; i < config_table->table_size; i++) {
		const struct xdp2_config_desc *desc = &config_table->table[i];

		val = config_read_val(config, desc->offset, desc->size);

		if (sep == '=' && !val) {
			/* Check for a derived value */
			__u64 dval = desc->derived_func(config);

			if (dval)
				val = dval;
		}

		if (strcasestr(desc->text, "bitmap")) {
			XDP2_CLI_PRINT(cli, "%s%c0x%llx\n",
					strlc(desc->conf_name), sep,
					val);
		} else if (strcasestr(desc->text, "IPv4 address")) {
			struct in_addr inaddr;

			inaddr.s_addr = val;
			XDP2_CLI_PRINT(cli, "%s%c%s\n",
					strlc(desc->conf_name), sep,
					inet_ntoa(inaddr));
		} else {
			XDP2_CLI_PRINT(cli, "%s%c%llu\n",
					strlc(desc->conf_name), sep,
					val);
		}
	}
}

/* Validate a configuration */
static int validate_configuration(const struct xdp2_config_desc_table
								*config_table,
				  const void *config)
{
	return 0;
}

/* Parse a configuration option from the command line */
static int parse_one_option(const struct xdp2_config_desc_table *config_table,
			    void *config, char *tok, char *label)
{

	const struct xdp2_config_desc *desc;
	char *idx;
	__u64 val;

	idx = strchr(tok, '=');
	if (!idx) {
		XDP2_WARN("%s: Bad token %s\n", label, tok);
		return -1;
	}
	*idx = '\0';

	desc = find_config_desc(config_table, tok);

	*idx++ = '=';

	if (!desc) {
		XDP2_WARN("%s: Configuration variable not "
			   "found: %s\n", label, tok);
		return -1;
	}

	if (strcasestr(desc->text, "IPv4 address")) {
		struct in_addr inaddr;

		inaddr.s_addr = inet_addr(idx);
		val = inaddr.s_addr;
	} else {
		val = strtoul(idx, NULL, 0);
	}

	if (val < desc->min) {
		XDP2_WARN("%s: Setting %s: %llu is less than "
			   "the minimum value %llu\n",
			   label, tok, val, desc->min);
		return -1;
	}
	if (desc->max != -1U && val > desc->max) {
		XDP2_WARN("%s: Setting %s: %llu is greater "
			   "than the maximum value %llu\n",
			   label, tok, val, desc->max);
		return -1;
	}

	config_write_val(config, desc->offset, desc->size, val);

	*--idx = '=';

	return 0;
}

/* Parse configuration options for command line */
int xdp2_config_parse_options(const struct xdp2_config_desc_table
								*config_table,
	       void *config, char *text)
{
	char *tok, *save_ptr;
	int ret;

	if (!text)
		return 0;

	for (tok = strtok_r(text, " \t", &save_ptr); tok;
	     tok = strtok_r(NULL, " \t", &save_ptr)) {
		ret = parse_one_option(config_table, config,
				       tok, "Parse options block");
		if (ret)
			return ret;
	}

	return validate_configuration(config_table, config);
}

#define MAXCHAR 256

/* Parse options from a file */
int xdp2_config_parse_options_file(const struct xdp2_config_desc_table
								*config_table,
				    void *config, const char *filename)
{
	char row[MAXCHAR];
	FILE *fp;
	int ret;

	fp = fopen(filename, "r");
	if (!fp) {
		XDP2_WARN("Parsing options file: Opening FIFO file %s: %s",
			   filename, strerror(errno));
		return -1;
	}

	while (fgets(row, MAXCHAR, fp)) {
		if (xdp2_line_is_whitespace(row) || row[0] == '#')
			continue;

		ret = parse_one_option(config_table, config, row,
				       "Parse options file");
		if (ret)
			return ret;
	}

	return validate_configuration(config_table, config);
}

/* Parse a options from command line starting with defaults */
int xdp2_config_parse_options_with_defaults(
	const struct xdp2_config_desc_table *config_table,
	void *config, char *text)
{
	config_table->set_default_config(config);

	return xdp2_config_parse_options(config_table, config, text);
}

/* Print the info able optins in a descriptor table */
void xdp2_config_print_config_info(const struct xdp2_config_desc_table
								*config_table,
				    void *cli, char *indent, bool highlight)
{
	__u64 default_val;
	void *config;
	char *highc;
	int i;

	config = malloc(config_table->config_struct_size);
	if (!config) {
		XDP2_CLI_PRINT(cli, "Malloc of config struct failed!\n");
		return;
	}

	config_table->set_default_config(config);

	highc = highlight ? "**" : "";

	for (i = 0; i < config_table->table_size; i++) {
		const struct xdp2_config_desc *desc = &config_table->table[i];

		default_val = config_read_val(config,
					      desc->offset,
					      desc->size);
		if (strcasestr(desc->text, "bitmap"))
			XDP2_CLI_PRINT(cli, "%s%s%s%s: default=0x%llx "
					     "allflags=%0llx, %s\n",
					indent, highc,
					strlc(desc->conf_name),
					highc, default_val,
					desc->max, desc->text);
		else
			XDP2_CLI_PRINT(cli, "%s%s%s%s: default=%llu "
					     "min=%llu max=%llu, %s\n",
					indent, highc,
					strlc(desc->conf_name),
					highc, default_val,
					desc->min, desc->max,
					desc->text);
	}

	free(config);
}

/* Printf config help */
void xdp2_config_print_config_help(const struct xdp2_config_desc_table
								*config_table,
				     void *cli, bool highlight)
{
	XDP2_CLI_PRINT(cli, "Configuration parameters:\n");
	xdp2_config_print_config_info(config_table, cli, "\t", highlight);
	XDP2_CLI_PRINT(cli, "\n");
}
