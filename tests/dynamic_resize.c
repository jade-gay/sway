#include <assert.h>
#include "sway/tree/dynamic_resize.h"

int main(void) {
	struct dynamic_resize_state state = {0};
	double width = 0, height = 0;

	assert(dynamic_resize_resolve(&state, false, true,
			700, 460, 810, 700, &width, &height) == DYNAMIC_RESIZE_NONE);
	assert(!state.active);

	assert(dynamic_resize_resolve(&state, true, true,
			700, 460, 810, 700, &width, &height) == DYNAMIC_RESIZE_APPLY);
	assert(state.active);
	assert(state.saved_width == 700);
	assert(state.saved_height == 460);
	assert(width == 810);
	assert(height == 700);

	// Switching directly from btop to Vim changes the target size without
	// replacing the original size that will eventually be restored.
	assert(dynamic_resize_resolve(&state, true, true,
			810, 700, 1220, 730, &width, &height) == DYNAMIC_RESIZE_APPLY);
	assert(state.saved_width == 700);
	assert(state.saved_height == 460);
	assert(width == 1220);
	assert(height == 730);

	assert(dynamic_resize_resolve(&state, true, false,
			1220, 730, 0, 0, &width, &height) == DYNAMIC_RESIZE_RESTORE);
	assert(!state.active);
	assert(width == 700);
	assert(height == 460);

	assert(dynamic_resize_resolve(&state, true, false,
			700, 460, 0, 0, &width, &height) == DYNAMIC_RESIZE_NONE);
	return 0;
}
