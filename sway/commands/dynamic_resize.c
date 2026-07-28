#include <limits.h>
#include <stdlib.h>
#include <strings.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/criteria.h"
#include "list.h"
#include "log.h"

static bool parse_size(const char *value, int *size) {
	char *end;
	long parsed = strtol(value, &end, 10);
	if (parsed <= 0 || parsed > INT_MAX) {
		return false;
	}
	if (*end && strcasecmp(end, "px") != 0) {
		return false;
	}
	*size = parsed;
	return true;
}

struct cmd_results *cmd_dynamic_resize(int argc, char **argv) {
	struct cmd_results *error =
		checkarg(argc, "dynamic_resize", EXPECTED_EQUAL_TO, 3);
	if (error) {
		return error;
	}

	char *criteria_error = NULL;
	struct criteria *criteria = criteria_parse(argv[0], &criteria_error);
	if (!criteria) {
		error = cmd_results_new(CMD_INVALID, "%s", criteria_error);
		free(criteria_error);
		return error;
	}

	int width, height;
	if (!parse_size(argv[1], &width) || !parse_size(argv[2], &height)) {
		criteria_destroy(criteria);
		return cmd_results_new(CMD_INVALID,
				"Expected 'dynamic_resize <criteria> <width> <height>'");
	}

	criteria->type = CT_DYNAMIC_RESIZE;
	criteria->resize_width = width;
	criteria->resize_height = height;
	list_add(config->criteria, criteria);
	sway_log(SWAY_DEBUG, "dynamic_resize: '%s' -> %dx%d added",
			criteria->raw, width, height);
	return cmd_results_new(CMD_SUCCESS, NULL);
}
