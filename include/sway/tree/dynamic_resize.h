#ifndef _SWAY_DYNAMIC_RESIZE_H
#define _SWAY_DYNAMIC_RESIZE_H

#include <stdbool.h>

enum dynamic_resize_action {
	DYNAMIC_RESIZE_NONE,
	DYNAMIC_RESIZE_APPLY,
	DYNAMIC_RESIZE_RESTORE,
};

struct dynamic_resize_state {
	bool active;
	double saved_width;
	double saved_height;
};

static inline enum dynamic_resize_action dynamic_resize_resolve(
		struct dynamic_resize_state *state, bool eligible, bool matched,
		double current_width, double current_height,
		double target_width, double target_height,
		double *resolved_width, double *resolved_height) {
	if (!eligible) {
		return DYNAMIC_RESIZE_NONE;
	}
	if (matched) {
		if (!state->active) {
			state->saved_width = current_width;
			state->saved_height = current_height;
			state->active = true;
		}
		*resolved_width = target_width;
		*resolved_height = target_height;
		return DYNAMIC_RESIZE_APPLY;
	}
	if (state->active) {
		*resolved_width = state->saved_width;
		*resolved_height = state->saved_height;
		state->active = false;
		return DYNAMIC_RESIZE_RESTORE;
	}
	return DYNAMIC_RESIZE_NONE;
}

#endif
