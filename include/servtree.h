/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers for service selection tree.
 */

#ifndef SERVTREE_H
#define SERVTREE_H

struct service
{
	char *internal_name;

	char *nick;
	char *user;
	char *host;
	char *real;
	char *disp;

	struct user *me;

	void (*handler)(struct sourceinfo *, int, char **);
	void (*notice_handler)(struct sourceinfo *, int, char **);

	mowgli_patricia_t *commands;
	mowgli_patricia_t *aliases;
	mowgli_patricia_t *access;

	bool chanmsg;

	mowgli_list_t conf_table;

	bool botonly;

	struct service *logtarget;
};

static inline const char *service_get_log_target(const struct service *svs)
{
	if (svs != NULL && svs->logtarget != NULL)
		return service_get_log_target(svs->logtarget);

	return svs != NULL ? svs->nick : me.name;
}

extern mowgli_patricia_t *services_name;
extern mowgli_patricia_t *services_nick;

extern void servtree_init(void);
extern struct service *service_add(const char *name, void (*handler)(struct sourceinfo *si, int parc, char *parv[]));
extern struct service *service_add_static(const char *name, const char *user, const char *host, const char *real, void (*handler)(struct sourceinfo *si, int parc, char *parv[]), struct service *logtarget);
extern void service_delete(struct service *sptr);
extern struct service *service_find_any(void);
extern struct service *service_find(const char *name);
extern struct service *service_find_nick(const char *nick);
extern char *service_name(char *name);
extern void service_set_chanmsg(struct service *, bool);
extern const char *service_resolve_alias(struct service *sptr, const char *context, const char *cmd);
extern const char *service_set_access(struct service *sptr, const char *cmd, const char *access);

extern void service_bind_command(struct service *, struct command *);
extern void service_unbind_command(struct service *, struct command *);

extern void service_named_bind_command(const char *, struct command *);
extern void service_named_unbind_command(const char *, struct command *);
extern void servtree_update(void *dummy);

#endif
