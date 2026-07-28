#include "sway/commands.h"
#include "util.h"

struct cmd_results *cmd_force_no_accel(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "force_no_accel", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	config->force_no_accel =
		parse_boolean(argv[0], config->force_no_accel);
	return cmd_results_new(CMD_SUCCESS, NULL);
}
