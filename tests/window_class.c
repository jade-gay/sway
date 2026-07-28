#include <assert.h>
#include <string.h>
#include "sway/tree/window_class.h"

int main(void) {
	assert(strcmp(window_class_resolve("org.example.Game", NULL),
			"org.example.Game") == 0);
	assert(strcmp(window_class_resolve(NULL, "game.exe"), "game.exe") == 0);
	assert(strcmp(window_class_resolve("ignored", "xwayland"),
			"xwayland") == 0);
	assert(window_class_resolve(NULL, NULL) == NULL);
	return 0;
}
