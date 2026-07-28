#ifndef _SWAY_INPUT_POINTER_H
#define _SWAY_INPUT_POINTER_H

#include <stdbool.h>

static inline double pointer_motion_select_delta(bool force_no_accel,
		bool is_pointer, double delta, double unaccel_delta) {
	return force_no_accel && is_pointer ? unaccel_delta : delta;
}

#endif
