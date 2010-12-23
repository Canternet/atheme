/* chanfix - channel fixing service
 * Copyright (c) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "chanfix.h"

static unsigned int count_ops(channel_t *c)
{
	unsigned int i = 0;
	mowgli_node_t *n;

	return_val_if_fail(c != NULL, 0);

	MOWGLI_ITER_FOREACH(n, c->members.head)
	{
		chanuser_t *cu = n->data;

		if (cu->modes & CSTATUS_OP)
			i++;
	}

	return i;
}

static bool chanfix_should_handle(chanfix_channel_t *cfchan, channel_t *c)
{
	mychan_t *mc;
	unsigned int n;

	return_val_if_fail(cfchan != NULL, false);

	if (c == NULL)
		return false;

	if ((mc = mychan_find(c->name)) != NULL)
		return false;

	if (MOWGLI_LIST_LENGTH(&c->members) < CHANFIX_OP_THRESHHOLD)
		return false;

	n = count_ops(c);
	/* enough ops, don't touch it */
	if (n >= CHANFIX_OP_THRESHHOLD)
		return false;
	/* only start a fix for opless channels, and consider a fix done
	 * after CHANFIX_FIX_TIME if any ops were given
	 */
	if (n > 0 && (cfchan->fix_started == 0 ||
			CURRTIME - cfchan->fix_started > CHANFIX_FIX_TIME))
		return false;

	return true;
}

static unsigned int chanfix_calculate_score(chanfix_oprecord_t *orec)
{
	unsigned int base;

	return_val_if_fail(orec != NULL, 0);

	base = orec->age;
	if (orec->entity != NULL)
		base *= CHANFIX_ACCOUNT_WEIGHT;

	return base;
}

static void chanfix_lower_ts(chanfix_channel_t *chan)
{
	channel_t *ch;
	chanuser_t *cfu;
	mowgli_node_t *n;

	ch = chan->chan;
	if (ch == NULL)
		return;

	chan->ts--;
	ch->ts = chan->ts;

	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;

		if (cu->modes & CSTATUS_OP)
			cu->modes &= ~CSTATUS_OP;
	}

	chan_lowerts(ch, chanfix->me);
	cfu = chanuser_add(ch, CLIENT_NAME(chanfix->me));
	cfu->modes |= CSTATUS_OP;

	msg(chanfix->me->nick, chan->name, "I only joined to remove modes.");

	part(chan->name, chanfix->me->nick);
}

static unsigned int chanfix_get_highscore(chanfix_channel_t *chan)
{
	unsigned int highscore = 0;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
	{
		unsigned int score;
		chanfix_oprecord_t *orec = n->data;

		score = chanfix_calculate_score(orec);
		if (score > highscore)
			highscore = score;
	}

	return highscore;
}

static unsigned int chanfix_get_threshold(chanfix_channel_t *chan)
{
	unsigned int highscore, t, threshold;

	highscore = chanfix_get_highscore(chan);

	t = CURRTIME - chan->fix_started;
	if (t > CHANFIX_FIX_TIME)
		t = CHANFIX_FIX_TIME;
	threshold = highscore * (CHANFIX_INITIAL_STEP +
			(CHANFIX_FINAL_STEP - CHANFIX_INITIAL_STEP) *
			t / CHANFIX_FIX_TIME);
	if (threshold == 0)
		threshold = 1;
	return threshold;
}

static bool chanfix_fix_channel(chanfix_channel_t *chan)
{
	channel_t *ch;
	mowgli_node_t *n;
	unsigned int threshold, opped = 0;

	ch = chan->chan;
	if (ch == NULL)
		return false;

	/* op users who have X% of the highest score. */
	threshold = chanfix_get_threshold(chan);
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		chanfix_oprecord_t *orec;
		unsigned int score;

		if (cu->user == chanfix->me)
			continue;
		if (cu->modes & CSTATUS_OP)
			continue;

		orec = chanfix_oprecord_find(chan, cu->user);
		if (orec == NULL)
			continue;

		score = chanfix_calculate_score(orec);

		if (score >= threshold)
		{
			if (opped == 0)
				join(chan->name, chanfix->me->nick);
			modestack_mode_param(chanfix->me->nick, chan->chan, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
			cu->modes |= CSTATUS_OP;
			opped++;
		}
	}

	if (opped == 0)
		return false;

	/* flush the modestacker. */
	modestack_flush_channel(ch);

	/* now report the damage */
	msg(chanfix->me->nick, chan->name, "\2%d\2 client%s should have been opped.", opped, opped != 1 ? "s" : "");

	/* fix done, leave. */
	part(chan->name, chanfix->me->nick);

	return true;
}

static bool chanfix_can_start_fix(chanfix_channel_t *chan)
{
	channel_t *ch;
	mowgli_node_t *n;
	unsigned int threshold;

	ch = chan->chan;
	if (ch == NULL)
		return false;

	threshold = chanfix_get_highscore(chan) * CHANFIX_FINAL_STEP;
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		chanfix_oprecord_t *orec;
		unsigned int score;

		if (cu->user == chanfix->me)
			continue;
		if (cu->modes & CSTATUS_OP)
			return false;

		orec = chanfix_oprecord_find(chan, cu->user);
		if (orec == NULL)
			continue;

		score = chanfix_calculate_score(orec);
		if (score >= threshold)
			return true;
	}
	return false;
}

static void chanfix_clear_bans(channel_t *ch)
{
	bool joined = false;
	mowgli_node_t *n, *tn;

	return_if_fail(ch != NULL);

	if (ch->modes & CMODE_INVITE)
	{
		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		channel_mode_va(chanfix->me, ch, 1, "-i");
	}
	if (ch->limit > 0)
	{
		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		channel_mode_va(chanfix->me, ch, 1, "-l");
	}
	if (ch->key != NULL)
	{
		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		channel_mode_va(chanfix->me, ch, 2, "-k", "*");
	}
	MOWGLI_ITER_FOREACH_SAFE(n, tn, ch->bans.head)
	{
		chanban_t *cb = n->data;

		if (cb->type != 'b')
			continue;

		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		modestack_mode_param(chanfix->me->nick, ch, MTYPE_DEL,
				'b', cb->mask);
		chanban_delete(cb);
	}
	if (!joined)
		return;

	modestack_flush_channel(ch);
	msg(chanfix->me->nick, ch->name, "I only joined to remove modes.");
	part(ch->name, chanfix->me->nick);
}

/*************************************************************************************/

void chanfix_autofix_ev(void *unused)
{
	chanfix_channel_t *chan;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(chan, &state, chanfix_channels)
	{
		if (chanfix_should_handle(chan, chan->chan))
		{
			if (chan->fix_started == 0)
			{
				if (chanfix_can_start_fix(chan))
				{
					slog(LG_INFO, "chanfix_autofix_ev(): fixing %s automatically.", chan->name);
					chan->fix_started = CURRTIME;
					/* If we are opping some users
					 * immediately, they can handle it.
					 * Otherwise, remove bans to allow
					 * users with higher scores to join.
					 */
					if (!chanfix_fix_channel(chan))
						chanfix_clear_bans(chan->chan);
				}
				else
				{
					/* No scored ops yet, remove bans
					 * to allow them to join.
					 */
					chanfix_clear_bans(chan->chan);
				}
			}
			else
			{
				/* Continue trying to give ops.
				 * If the channel is still or again opless,
				 * remove bans to allow ops to join.
				 */
				if (!chanfix_fix_channel(chan) &&
						count_ops(chan->chan) == 0)
					chanfix_clear_bans(chan->chan);
			}
		}
		else
			chan->fix_started = 0;
	}
}

/*************************************************************************************/

static void chanfix_cmd_fix(sourceinfo_t *si, int parc, char *parv[])
{
	chanfix_channel_t *chan;

	if (parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANFIX");
		command_fail(si, fault_needmoreparams, _("To fix a channel: CHANFIX <#channel>"));
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	chanfix_lower_ts(chan);

	command_success_nodata(si, _("Fix request has been acknowledged for \2%s\2."), parv[0]);
}

command_t cmd_chanfix = { "CHANFIX", N_("Manually chanfix a channel."), AC_NONE, 1, chanfix_cmd_fix, { .path = "" } };

static int chanfix_compare_records(mowgli_node_t *a, mowgli_node_t *b, void *unused)
{
	chanfix_oprecord_t *ta = a->data;
	chanfix_oprecord_t *tb = b->data;

	return tb->age - ta->age;
}

static void chanfix_cmd_scores(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	chanfix_channel_t *chan;
	int i = 0;
	unsigned int count = 20;

	if (parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SCORES");
		command_fail(si, fault_needmoreparams, _("To view CHANFIX scores for a channel: SCORES <#channel>"));
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	/* sort records by score. */
	mowgli_list_sort(&chan->oprecords, chanfix_compare_records, NULL);

	if (count > MOWGLI_LIST_LENGTH(&chan->oprecords))
		count = MOWGLI_LIST_LENGTH(&chan->oprecords);

	if (count == 0)
	{
		command_success_nodata(si, _("There are no scores in the CHANFIX database for \2%s\2."), chan->name);
		return;
	}

	command_success_nodata(si, _("Top \2%d\2 scores for \2%s\2 in the database:"), count, chan->name);

	command_success_nodata(si, "%-3s %-50s %s", _("Num"), _("Account/Hostmask"), _("Score"));
	command_success_nodata(si, "%-3s %-50s %s", "---", "--------------------------------------------------", "-----");

	MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
	{
		char buf[BUFSIZE];
		unsigned int score;
		chanfix_oprecord_t *orec = n->data;

		score = chanfix_calculate_score(orec);

		snprintf(buf, BUFSIZE, "%s@%s", orec->user, orec->host);

		command_success_nodata(si, "%-3d %-50s %d", i + 1, orec->entity ? orec->entity->name : buf, score);
	}

	command_success_nodata(si, "%-3s %-50s %s", "---", "--------------------------------------------------", "-----");
	command_success_nodata(si, _("End of \2SCORES\2 listing for \2%s\2."), chan->name);
}

command_t cmd_scores = { "SCORES", N_("List channel scores."), AC_NONE, 1, chanfix_cmd_scores, { .path = "" } };

static void chanfix_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	chanfix_oprecord_t *orec;
	chanfix_channel_t *chan;
	struct tm tm;
	char strfbuf[BUFSIZE];
	unsigned int highscore = 0;

	if (parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SCORES");
		command_fail(si, fault_needmoreparams, _("To view CHANFIX scores for a channel: SCORES <#channel>"));
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	/* sort records by score. */
	mowgli_list_sort(&chan->oprecords, chanfix_compare_records, NULL);

	command_success_nodata(si, _("Information on \2%s\2:"), chan->name);

	tm = *localtime(&chan->ts);
	strftime(strfbuf, sizeof(strfbuf) - 1, config_options.time_format, &tm);

	command_success_nodata(si, _("Creation time: %s"), strfbuf);

	if (chan->oprecords.head != NULL)
	{
		orec = chan->oprecords.head->data;
		highscore = chanfix_calculate_score(orec);
	}

	command_success_nodata(si, _("Highest score: \2%u\2"), highscore);
	command_success_nodata(si, _("Usercount    : \2%zu\2"), chan->chan ? MOWGLI_LIST_LENGTH(&chan->chan->members) : 0);
	command_success_nodata(si, _("Initial step : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"), CHANFIX_INITIAL_STEP * 100, highscore, (highscore * CHANFIX_INITIAL_STEP));
	command_success_nodata(si, _("Current step : \2%u\2"), chanfix_get_threshold(chan));
	command_success_nodata(si, _("Final step   : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"), CHANFIX_FINAL_STEP * 100, highscore, (highscore * CHANFIX_FINAL_STEP));
	command_success_nodata(si, _("Needs fixing : \2%s\2"), chanfix_should_handle(chan, chan->chan) ? "YES" : "NO");
	command_success_nodata(si, _("Now fixing   : \2%s\2"),
			chan->fix_started ? "YES" : "NO");

	command_success_nodata(si, _("\2*** End of Info ***\2"));
}

command_t cmd_info = { "INFO", N_("List information on channel."), AC_NONE, 1, chanfix_cmd_info, { .path = "" } };