#ifndef _SWAY_DWINDLE_H
#define _SWAY_DWINDLE_H

#include <stdbool.h>

static inline bool dwindle_split_is_horizontal(double width, double height) {
	return width >= height;
}

static inline bool dwindle_insert_before(double x, double y,
		double width, double height, double lx, double ly) {
	bool side_by_side = width > height;
	return side_by_side ? lx < x + width / 2.0 : ly < y + height / 2.0;
}

static inline bool dwindle_force_split_before(
		int force_split, bool point_before) {
	return force_split == 1 ? true :
		force_split == 2 ? false : point_before;
}

static inline double dwindle_ratio_from_fractions(double first,
		double second, double fallback) {
	if (first <= 0 || second <= 0) {
		return fallback;
	}
	double ratio = first / (first + second) * 2.0;
	return ratio < 0.1 ? 0.1 : ratio > 1.9 ? 1.9 : ratio;
}

static inline bool dwindle_at_workspace_edge(double position, double size,
		double workspace_position, double workspace_size, bool toward_start) {
	return toward_start ? position <= workspace_position :
		position + size >= workspace_position + workspace_size;
}

static inline double dwindle_move_focal_coordinate(double position,
		double size, double workspace_position, double workspace_size,
		double output_position, double output_size, double inner_gap,
		bool toward_start) {
	if (dwindle_at_workspace_edge(position, size,
			workspace_position, workspace_size, toward_start)) {
		return toward_start ?
			output_position - 1 : output_position + output_size + 1;
	}
	return toward_start ?
		position - inner_gap - 1 : position + size + inner_gap + 1;
}

#endif
