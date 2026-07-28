#include <assert.h>
#include <string.h>
#include "sway/tree/tag.h"

int main(void) {
	enum sway_tag_operation operation;
	const char *name;

	assert(tag_parse_spec("+game", &operation, &name));
	assert(operation == SWAY_TAG_ADD);
	assert(strcmp(name, "game") == 0);

	assert(tag_parse_spec("-game", &operation, &name));
	assert(operation == SWAY_TAG_REMOVE);
	assert(strcmp(name, "game") == 0);

	assert(tag_parse_spec("game", &operation, &name));
	assert(operation == SWAY_TAG_TOGGLE);
	assert(strcmp(name, "game") == 0);

	assert(!tag_parse_spec("", &operation, &name));
	assert(!tag_parse_spec("+", &operation, &name));
	assert(!tag_parse_spec("-", &operation, &name));
	return 0;
}
