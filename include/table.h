/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Table rendering class.
 */

#ifndef ATHEME_INC_TABLE_H
#define ATHEME_INC_TABLE_H 1

struct atheme_table
{
	struct atheme_object parent;
	mowgli_list_t rows;
};

struct atheme_table_row
{
	int id;
	mowgli_list_t cells;
};

struct atheme_table_cell
{
	int width;	/* only if first row. */
	char *name;
	char *value;
};

/*
 * Creates a new table object. Use atheme_object_unref() to destroy it.
 */
struct atheme_table *table_new(const char *fmt, ...) ATHEME_FATTR_PRINTF(1, 2);

/*
 * Renders a table, each line going to callback().
 */
void table_render(struct atheme_table *t, void (*callback)(const char *line, void *data), void *data);

/*
 * Associates a value with a row.
 */
void table_cell_associate(struct atheme_table_row *r, const char *name, const char *value);

/*
 * Associates a row with a table.
 */
struct atheme_table_row *table_row_new(struct atheme_table *t);

#endif /* !ATHEME_INC_TABLE_H */
