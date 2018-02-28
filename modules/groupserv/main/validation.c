/* groupserv - group services.
 * Copyright (c) 2010 Atheme Development Group
 */

#include "groupserv_main.h"

static struct chanacs *mygroup_chanacs_match_entity(struct chanacs *ca, struct myentity *mt)
{
	mygroup_t *mg;

	mg = group(ca->entity);

	return_val_if_fail(mg != NULL, NULL);

	if (!isuser(mt))
		return NULL;

	return groupacs_find(mg, mt, GA_CHANACS, true) != NULL ? ca : NULL;
}

static bool mygroup_can_register_channel(struct myentity *mt)
{
	mygroup_t *mg;

	mg = group(mt);

	return_val_if_fail(mg != NULL, false);

	if (mg->flags & MG_REGNOLIMIT)
		return true;

	return false;
}

static bool mygroup_allow_foundership(struct myentity *mt)
{
	return true;
}

static const struct entity_chanacs_validation_vtable mygroup_chanacs_validate = {
	.match_entity = mygroup_chanacs_match_entity,
	.can_register_channel = mygroup_can_register_channel,
	.allow_foundership = mygroup_allow_foundership,
};

void mygroup_set_chanacs_validator(struct myentity *mt) {
	mt->chanacs_validate = &mygroup_chanacs_validate;
}
