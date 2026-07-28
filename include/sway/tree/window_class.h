#ifndef _SWAY_WINDOW_CLASS_H
#define _SWAY_WINDOW_CLASS_H

static inline const char *window_class_resolve(const char *app_id,
		const char *xwayland_class) {
	return xwayland_class ? xwayland_class : app_id;
}

#endif
