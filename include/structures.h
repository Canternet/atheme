/*
 * Copyright (C) 2018 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Structures used throughout the project (forward declarations).
 */

#ifndef STRUCTURES_H
#define STRUCTURES_H

// XXX remove me, or move me to include/hook.h
typedef struct server_ server_t;
typedef struct myuser_ myuser_t;
typedef struct mynick_ mynick_t;
typedef struct mychan_ mychan_t;

// Defined in include/account.h
struct chanacs;
struct groupacs;
struct mygroup;
struct svsignore;

// Defined in include/channels.h
struct chanban;
struct channel;
struct chanuser;

// Defined in include/connection.h
struct connection;

// Defined in include/crypto.h
struct crypt_impl;

// Defined in include/culture.h
struct language;

// Defined in include/database_backend.h
struct database_handle;
struct database_module;
struct database_vtable;

// Defined in include/entity.h
struct myentity;
struct myentity_iteration_state;

// Defined in include/entity-validation.h
struct entity_chanacs_validation_vtable;

// Defined in include/flags.h
struct gflags;

// Defined in include/global.h
struct me;

// Defined in include/hook.h
struct hook;

// Defined in include/httpd.h
struct path_handler;

// Defined in include/match.h
struct atheme_regex;

// Defined in include/module.h
struct module;
struct module_dependency;
struct v4_moduleheader;

// Defined in include/object.h
struct atheme_object;
struct metadata;

// Defined in include/pcommand.h
struct proto_cmd;

// Defined in include/phandler.h
struct ircd;

// Defined in include/privs.h
struct operclass;
struct soper;

// Defined in include/res.h
struct nsaddr;
struct res_dns_query;
struct res_dns_reply;

// Defined in include/sasl.h
struct sasl_core_functions;
struct sasl_mechanism;
struct sasl_message;
struct sasl_session;
struct sasl_sourceinfo;

// Defined in include/servers.h
struct tld;

// Defined in include/services.h
struct chansvs;
struct nicksvs;

// Defined in include/servtree.h
struct service;

// Defined in include/sourceinfo.h
struct sourceinfo;
struct sourceinfo_vtable;

// Defined in include/table.h
struct atheme_table;
struct atheme_table_cell;
struct atheme_table_row;

// Defined in include/taint.h
struct taint_reason;

// Defined in include/template.h
struct default_template;

// Defined in include/tools.h
struct email_canonicalizer_item;
struct logfile;

// Defined in include/uid.h
struct uid_provider;

// Defined in include/uplink.h
struct uplink;

// Defined in include/users.h
struct user;

#endif
