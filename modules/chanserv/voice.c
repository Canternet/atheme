/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService VOICE functions.
 *
 * $Id: voice.c 6427 2006-09-22 19:38:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/voice", FALSE, _modinit, _moddeinit,
	"$Id: voice.c 6427 2006-09-22 19:38:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_voice(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_devoice(sourceinfo_t *si, int parc, char *parv[]);
static void cs_fcmd_voice(char *origin, char *chan);
static void cs_fcmd_devoice(char *origin, char *chan);

command_t cs_voice = { "VOICE", "Gives channel voice to a user.",
                         AC_NONE, 2, cs_cmd_voice };
command_t cs_devoice = { "DEVOICE", "Removes channel voice from a user.",
                         AC_NONE, 2, cs_cmd_devoice };

fcommand_t fc_voice = { "!voice", AC_NONE, cs_fcmd_voice };
fcommand_t fc_devoice = { "!devoice", AC_NONE, cs_fcmd_devoice };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_voice, cs_cmdtree);
        command_add(&cs_devoice, cs_cmdtree);
	fcommand_add(&fc_voice, cs_fcmdtree);
	fcommand_add(&fc_devoice, cs_fcmdtree);

	help_addentry(cs_helptree, "VOICE", "help/cservice/op_voice", NULL);
	help_addentry(cs_helptree, "DEVOICE", "help/cservice/op_voice", NULL);
}

void _moddeinit()
{
	command_delete(&cs_voice, cs_cmdtree);
	command_delete(&cs_devoice, cs_cmdtree);
	fcommand_delete(&fc_voice, cs_fcmdtree);
	fcommand_delete(&fc_devoice, cs_fcmdtree);

	help_delentry(cs_helptree, "VOICE");
	help_delentry(cs_helptree, "DEVOICE");
}

static void cs_cmd_voice(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VOICE");
		command_fail(si, fault_needmoreparams, "Syntax: VOICE <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	si->su = user_find_named(si->su->nick);
	if (!chanacs_user_has_flag(mc, si->su, CA_VOICE))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'v', CLIENT_NAME(tu));
	cu->modes |= CMODE_VOICE;

	/* TODO: Add which username had access to perform the command */
	if (tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been voiced on %s by %s", mc->name, si->su->nick);

	logcommand(chansvs.me, si->su, CMDLOG_SET, "%s VOICE %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been voiced on \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_devoice(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEVOICE");
		command_fail(si, fault_needmoreparams, "Syntax: DEVOICE <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	si->su = user_find_named(si->su->nick);
	if (!chanacs_user_has_flag(mc, si->su, CA_VOICE))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to devoice */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'v', CLIENT_NAME(tu));
	cu->modes &= ~CMODE_VOICE;

	/* TODO: Add which username had access to perform the command */
	if (tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been devoiced on %s by %s", mc->name, si->su->nick);

	logcommand(chansvs.me, si->su, CMDLOG_SET, "%s DEVOICE %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been devoiced on \2%s\2.", tu->nick, mc->name);
}

static void cs_fcmd_voice(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_VOICE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	/* figure out who we're going to voice */
	nick = strtok(NULL, " ");
	do
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find_named(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (is_internal_client(tu))
			continue;

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'v', CLIENT_NAME(tu));
		cu->modes |= CMODE_VOICE;
		logcommand(chansvs.me, u, CMDLOG_SET, "%s VOICE %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	} while ((nick = strtok(NULL, " ")) != NULL);

}

static void cs_fcmd_devoice(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_VOICE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to devoice */
	nick = strtok(NULL, " ");
	do
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find_named(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (is_internal_client(tu))
			continue;

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'v', CLIENT_NAME(tu));
		cu->modes &= ~CMODE_VOICE;
		logcommand(chansvs.me, u, CMDLOG_SET, "%s DEVOICE %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	} while ((nick = strtok(NULL, " ")) != NULL);
}

