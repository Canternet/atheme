/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include "atheme.h"
#include "groupserv.h"

static void gs_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_drop = { "DROP", N_("Drops a group registration."), AC_AUTHENTICATED, 2, gs_cmd_drop, { .path = "groupserv/drop" } };

static void gs_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *name = parv[0];
	char *key = parv[1];
	char fullcmd[512];
	char key0[80], key1[80];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (*name != '!')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (!(mg = mygroup_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), name);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (si->su != NULL)
	{
		const char *const challenge = create_weak_challenge(si, entity(mg)->name);

		if (! challenge)
		{
			(void) command_fail(si, fault_internalerror, _("Failed to create challenge"));
			return;
		}

		if (! key)
		{
			(void) snprintf(fullcmd, sizeof fullcmd, "/%s%s DROP %s %s", (ircd->uses_rcommand == false) ?
			                "msg " : "", si->service->disp, entity(mg)->name, challenge);

			(void) command_success_nodata(si, _("To avoid accidental use of this command, this operation "
			                                    "has to be confirmed. Please confirm by replying with "
			                                    "\2%s\2"), fullcmd);
			return;
		}

		if (strcmp(challenge, key) != 0)
		{
			(void) command_fail(si, fault_badparams, _("Invalid key for \2%s\2."), "DROP");
			return;
		}
	}

	logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", entity(mg)->name);
	remove_group_chanacs(mg);
	hook_call_group_drop(mg);
	object_unref(mg);
	command_success_nodata(si, _("The group \2%s\2 has been dropped."), name);
	return;
}


static void
mod_init(module_t *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_drop);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_drop);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/drop", MODULE_UNLOAD_CAPABILITY_OK)
