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

#ifndef KP_KPAGENT_H
#define KP_KPAGENT_H

#include <limits.h>

#include "kickpass.h"

#define KP_AGENT_SOCKET_ENV "KP_AGENT_SOCK"

enum kp_agent_msg_type {
	KP_MSG_STORE,
	KP_MSG_SEARCH,
};

struct kp_unsafe {
	time_t timeout; /* timeout of the safe */
	char path[PATH_MAX]; /* name of the safe */
	char password[KP_PASSWORD_MAX_LEN]; /* plain text password (null terminated) */
	char metadata[KP_METADATA_MAX_LEN]; /* plain text metadata (null terminated) */
};

struct kp_agent_safe {
	time_t timeout; /* timeout of the safe */
	char path[PATH_MAX]; /* name of the safe */
	char * const password;      /* plain text password (null terminated) */
	char * const metadata;      /* plain text metadata (null terminated) */
};

/* Client side */
kp_error_t kp_agent_init(struct kp_agent *, const char *);
kp_error_t kp_agent_connect(struct kp_agent *);
kp_error_t kp_agent_listen(struct kp_agent *);
kp_error_t kp_agent_send(struct kp_agent *, enum kp_agent_msg_type, void *, size_t);
kp_error_t kp_agent_close(struct kp_agent *);

/* Server side */
kp_error_t kp_agent_safe_create(struct kp_agent *, struct kp_agent_safe **);
kp_error_t kp_agent_store(struct kp_agent *, struct kp_agent_safe *);
kp_error_t kp_agent_search(struct kp_agent *, char *, struct kp_agent_safe **);

#endif /* KP_KPAGENT_H */
