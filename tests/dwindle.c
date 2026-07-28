#include <assert.h>
#include "sway/tree/dwindle.h"

int main(void) {
	assert(dwindle_split_is_horizontal(1600, 900));
	assert(!dwindle_split_is_horizontal(900, 1600));
	assert(dwindle_split_is_horizontal(1000, 1000));

	assert(dwindle_insert_before(0, 0, 1600, 900, 100, 800));
	assert(!dwindle_insert_before(0, 0, 1600, 900, 1500, 100));
	assert(dwindle_insert_before(0, 0, 900, 1600, 800, 100));
	assert(!dwindle_insert_before(0, 0, 900, 1600, 100, 1500));

	// Hyprland chooses insertion order using a strict width comparison, then
	// recalculates square split nodes as horizontal.
	assert(dwindle_insert_before(0, 0, 1000, 1000, 900, 100));
	assert(!dwindle_insert_before(0, 0, 1000, 1000, 100, 900));

	// force_split = 2 always inserts as the second child. Depending on the
	// split geometry, that means right or bottom.
	assert(dwindle_force_split_before(1, false));
	assert(!dwindle_force_split_before(2, true));
	assert(!dwindle_force_split_before(2, false));
	assert(dwindle_force_split_before(0, true));
	assert(!dwindle_force_split_before(0, false));

	// A newly tiled window can temporarily have no fraction. Preserve the
	// parent's split ratio instead of turning the pair into a 95/5 split.
	assert(dwindle_ratio_from_fractions(0, 0.5, 1.0) == 1.0);
	assert(dwindle_ratio_from_fractions(0.5, 0, 1.0) == 1.0);
	assert(dwindle_ratio_from_fractions(0.25, 0.75, 1.0) == 0.5);

	// Outer gaps must not trap directional moves on the source output. The
	// tiled workspace starts at 1930, but DP-3 starts at 1920.
	assert(dwindle_at_workspace_edge(1930, 948, 1930, 1900, true));
	assert(dwindle_move_focal_coordinate(
			1930, 948, 1930, 1900, 1920, 1920, 5, true) == 1919);

	// An interior focal point must cross the whole inner gap. Sampling one
	// pixel outside the gapped container would select the same branch again.
	assert(!dwindle_at_workspace_edge(1930, 948, 1930, 1900, false));
	assert(dwindle_move_focal_coordinate(
			1930, 948, 1930, 1900, 1920, 1920, 5, false) == 2884);
	assert(dwindle_move_focal_coordinate(
			963, 947, 10, 1900, 0, 1920, 5, true) == 957);
	return 0;
}
