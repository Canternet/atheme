/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Management of tainted running configuration reasons and status.
 */

#ifndef ATHEME_TAINT_H
#define ATHEME_TAINT_H

struct taint_reason
{
	mowgli_node_t node;
	char buf[BUFSIZE];
	char condition[BUFSIZE];
	char file[BUFSIZE];
	int line;
};

extern mowgli_list_t taint_list;

#define IS_TAINTED      MOWGLI_LIST_LENGTH(&taint_list)

#define TAINT_ON(cond, reason)                                                          \
        if ((cond))                                                                     \
        {                                                                               \
                struct taint_reason *const tr = smalloc(sizeof *tr);                    \
                (void) mowgli_strlcpy(tr->buf, (reason), sizeof tr->buf);               \
                (void) mowgli_strlcpy(tr->condition, #cond, sizeof tr->condition);      \
                (void) mowgli_strlcpy(tr->file, __FILE__, sizeof tr->file);             \
                tr->line = __LINE__;                                                    \
                (void) mowgli_node_add(tr, &tr->node, &taint_list);                     \
                (void) slog(LG_ERROR, "TAINTED: %s", (reason));                         \
                if (! config_options.allow_taint)                                       \
                {                                                                       \
                        slog(LG_ERROR, "exiting due to taint");                         \
                        exit(EXIT_FAILURE);                                             \
                }                                                                       \
        }

#endif
