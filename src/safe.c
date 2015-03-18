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

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kickpass.h"

#include "editor.h"
#include "safe.h"
#include "storage.h"

/*
 * Load an existing safe.
 * Init the kp_safe structure and open the cipher file.
 * The returned safe is closed.
 */
kp_error_t
kp_safe_load(struct kp_ctx *ctx, const char *path, enum kp_safe_plaintext_type type, struct kp_safe *safe)
{
	struct stat stats;

	if (strlcpy(safe->path, path, PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	safe->plain.type = type;
	switch (safe->plain.type) {
	case KP_SAFE_PLAINTEXT_FILE:
		safe->plain.fd = 0;
		break;
	case KP_SAFE_PLAINTEXT_MEMORY:
		safe->plain.size = 0;
		safe->plain.data = NULL;
		break;
	default:
		assert(false);
	}
	safe->open = false;

	if (stat(path, &stats) != 0) {
		warn("unknown safe %s", path);
		return KP_EINPUT;
	}

	safe->cipher.fd = open(safe->path, O_RDWR | O_NONBLOCK);
	if (safe->cipher.fd < 0) {
		warn("cannot open safe %s", safe->path);
		return KP_EINPUT;
	}

	return KP_SUCCESS;
}

/*
 * Create a new safe.
 * The returned safe is opened.
 */
kp_error_t
kp_safe_create(struct kp_ctx *ctx, const char *path, enum kp_safe_plaintext_type type, struct kp_safe *safe)
{
	kp_error_t ret;
	struct stat stats;

	if (strlcpy(safe->path, path, PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	safe->plain.type = type;
	safe->open = true;

	if (stat(path, &stats) == 0) {
		warnx("safe %s already exists", path);
		return KP_EEXIST;
	} else if (errno != ENOENT) {
		warn("cannot create safe %s", path);
		return KP_EINPUT;
	}

	safe->cipher.fd = open(safe->path, O_RDWR | O_NONBLOCK | O_CREAT, S_IRUSR | S_IWUSR);
	if (safe->cipher.fd < 0) {
		warn("cannot open safe %s", safe->path);
		return KP_EINPUT;
	}

	if ((ret = kp_editor_get_tmp(ctx, safe, true)) != KP_SUCCESS) {
		return ret;
	}

	dprintf(safe->plain.fd, "%s", KP_SAFE_TEMPLATE);

	return KP_SUCCESS;
}

/*
 * Close a safe.
 * Take care of cleaning the safe plain text and closing the opened file
 * descriptor.
 */
kp_error_t
kp_safe_close(struct kp_ctx *ctx, struct kp_safe *safe)
{
	if (close(safe->cipher.fd) < 0) {
		warn("cannot close safe");
		return errno;
	}

	safe->cipher.fd = 0;

	switch (safe->plain.type) {
	case KP_SAFE_PLAINTEXT_FILE:
		if (safe->plain.fd == 0) break;

		if (close(safe->plain.fd) < 0) {
			warn("cannot close clear text safe");
			return errno;
		}

		safe->plain.fd = 0;

		if (unlink(safe->plain.path) < 0) {
			warn("cannot delete clear text safe");
			return errno;
		}
		break;
	case KP_SAFE_PLAINTEXT_MEMORY:
		if (safe->plain.data == NULL) break;

		// XXX shred memory ?
		free(safe->plain.data);
		break;
	default:
		assert(false);
	}

	return KP_SUCCESS;
}