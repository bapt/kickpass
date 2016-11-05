/*
 * Copyright (c) 2015 Paul Fariello <paul@fariello.eu>
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

#include <sys/stat.h>

#include <errno.h>
#include <getopt.h>
#include <sodium.h>
#include <stdio.h>
#include <string.h>

#include "kickpass.h"

#include "command.h"
#include "create.h"
#include "editor.h"
#include "password.h"
#include "prompt.h"
#include "safe.h"
#include "log.h"

static kp_error_t create(struct kp_ctx *, int, char **);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static void       usage(void);

struct kp_cmd kp_cmd_create = {
	.main  = create,
	.usage = usage,
	.opts  = "create [-hgl] <safe>",
	.desc  = "Create a new password safe",
};

static bool generate = false;
static int  password_len = 20;

kp_error_t
create(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	struct kp_safe safe;
	char *password = NULL;

	if ((ret = parse_opt(ctx, argc, argv)) != KP_SUCCESS) {
		return ret;
	}

	if (argc - optind != 1) {
		ret = KP_EINPUT;
		kp_warn(ret, "missing safe name");
		return ret;
	}

	/* Ask for password, otherwise it is asked on kp_safe_save which seems
	 * weird for user */
	if ((ret = ctx->password_cb(ctx)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_create(ctx, &safe, argv[optind])) != KP_SUCCESS) {
		if (ret == KP_ERRNO && errno == EEXIST) {
			kp_warn(ret, "use 'edit' command to edit an existing safe");
		}
		goto out;
	}

	if (generate) {
		kp_password_generate(safe.password, password_len);
	} else {
		if ((ret = kp_prompt_password("safe", true, safe.password)) != KP_SUCCESS) {
			goto out;
		}
	}

	snprintf((char *)safe.metadata, KP_METADATA_MAX_LEN, KP_METADATA_TEMPLATE);

	if ((ret = kp_edit(ctx, &safe)) != KP_SUCCESS) {
		goto out;
	}

	if ((ret = kp_safe_save(ctx, &safe)) != KP_SUCCESS) {
		goto out;
	}

	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		goto out;
	}

	ret = KP_SUCCESS;

out:
	sodium_free(password);
	return ret;
}

static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	int opt;
	kp_error_t ret = KP_SUCCESS;
	static struct option longopts[] = {
		{ "generate", no_argument,       NULL, 'g' },
		{ "length",   required_argument, NULL, 'l' },
		{ NULL,       0,                 NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "gl:", longopts, NULL)) != -1) {
		switch (opt) {
		case 'g':
			generate = true;
			break;
		case 'l':
			password_len = atoi(optarg);
			break;
		default:
			ret = KP_EINPUT;
			kp_warn(ret, "unknown option %c", opt);
		}
	}

	return ret;
}

void
usage(void)
{
	printf("options:\n");
	printf("    -g, --generate     Randomly generate a password\n");
	printf("    -l, --length=len   Length of the generated passwerd. Default to 20\n");
}
