#include <assert.h>
#include "sway/input/pointer.h"

int main(void) {
	assert(pointer_motion_select_delta(false, true, 4.5, 3.0) == 4.5);
	assert(pointer_motion_select_delta(true, true, 4.5, 3.0) == 3.0);
	assert(pointer_motion_select_delta(true, false, 4.5, 3.0) == 4.5);
	return 0;
}
