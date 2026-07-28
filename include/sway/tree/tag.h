#ifndef _SWAY_TAG_H
#define _SWAY_TAG_H

#include <stdbool.h>

enum sway_tag_operation {
	SWAY_TAG_ADD,
	SWAY_TAG_REMOVE,
	SWAY_TAG_TOGGLE,
};

static inline bool tag_parse_spec(const char *spec,
		enum sway_tag_operation *operation, const char **name) {
	if (!spec || !*spec) {
		return false;
	}

	switch (*spec) {
	case '+':
		*operation = SWAY_TAG_ADD;
		++spec;
		break;
	case '-':
		*operation = SWAY_TAG_REMOVE;
		++spec;
		break;
	default:
		*operation = SWAY_TAG_TOGGLE;
		break;
	}

	if (!*spec) {
		return false;
	}

	*name = spec;
	return true;
}

#endif
