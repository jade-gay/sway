#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/container.h"
#include "sway/tree/tag.h"
#include "sway/tree/view.h"

// tag +name  Add a tag
// tag -name  Remove a tag
// tag name   Toggle a tag

struct cmd_results *cmd_tag(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "tag", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	struct sway_container *con = config->handler_context.container;
	if (!con || !con->view) {
		return cmd_results_new(CMD_INVALID, "Only windows can have tags");
	}

	enum sway_tag_operation operation;
	const char *name;
	if (!tag_parse_spec(argv[0], &operation, &name)) {
		return cmd_results_new(CMD_INVALID,
				"Expected 'tag [+|-]<identifier>'");
	}

	bool changed = false;
	switch (operation) {
	case SWAY_TAG_ADD:
		changed = container_add_tag(con, name);
		break;
	case SWAY_TAG_REMOVE:
		changed = container_remove_tag(con, name);
		break;
	case SWAY_TAG_TOGGLE:
		changed = container_has_tag(con, name) ?
			container_remove_tag(con, name) :
			container_add_tag(con, name);
		break;
	}

	if (changed) {
		view_execute_criteria(con->view);
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
