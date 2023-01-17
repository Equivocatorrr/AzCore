/*
	File: Wayland.cpp
	Author: Philip Haynes
	Separating out all our Wayland code, because there is a LOT of it.
*/

#ifndef AZCORE_WAYLAND_CPP
#define AZCORE_WAYLAND_CPP

#include "../Window.hpp"
#include "../../io.hpp"
#include "../../keycodes.hpp"
#include "WindowData.hpp"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

namespace AzCore {

namespace io {

namespace wl::events {

static void xdgWMBasePing(void *data, xdg_wm_base *xdgWMBase, u32 serial) {
	xdg_wm_base_pong(xdgWMBase, serial);
}

static const xdg_wm_base_listener xdgWMBaseListener = {
	xdgWMBasePing
};

static void xdgSurfaceConfigure(void *data, xdg_surface *xdgSurface, u32 serial) {
	xdg_surface_ack_configure(xdgSurface, serial);
}

static const xdg_surface_listener xdgSurfaceListener = {
	xdgSurfaceConfigure
};

static void xdgToplevelConfigure(void *data, xdg_toplevel *xdgToplevel, i32 width, i32 height, wl_array *states) {
	Window *window = (Window*)data;
	cout.PrintLn("configure with width ", width, " and height ", height);
	if (width != 0 && height != 0) {
		window->width = width;
		window->height = height;
		window->resized = true;
	}
}

static void xdgToplevelClose(void *data, xdg_toplevel *xdgToplevel) {
	Window *window = (Window*)data;
	window->quit = true;
}

static void xdgToplevelConfigureBounds(void *data, xdg_toplevel *xdgToplevel, i32 width, i32 height) {
	// Window *window = (Window*)data;
	cout.PrintLn("Max window bounds: ", width, ", ", height);
}

static void xdgToplevelWMCapabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities) {
	// Do something
}

static const xdg_toplevel_listener xdgToplevelListener = {
	.configure = xdgToplevelConfigure,
	.close = xdgToplevelClose,
	.configure_bounds = xdgToplevelConfigureBounds,
	.wm_capabilities = xdgToplevelWMCapabilities
};

static void seatHandleCapabilities(void *data, wl_seat *seat, u32 caps) {
	if (caps & WL_SEAT_CAPABILITY_POINTER) {
		cout.PrintLn("Display has a pointer.");
	}
	if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		cout.PrintLn("Display has a keyboard.");
	}
	if (caps & WL_SEAT_CAPABILITY_TOUCH) {
		cout.PrintLn("Display has a touch screen.");
	}
}

static const wl_seat_listener seatListener = {
	seatHandleCapabilities
};

static void globalRegistryAdd(void *data, wl_registry *registry, u32 id, const char *interface, u32 version) {
	Window *window = (Window*)data;
	cout.PrintLn("Got a registry add event for ", interface, " id ", id);
	if (equals(interface, wl_compositor_interface.name)) {
		window->data->wayland.compositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	} else if (equals(interface, xdg_wm_base_interface.name)) {
		window->data->wayland.wmBase = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(window->data->wayland.wmBase, &xdgWMBaseListener, window);
	} else if (equals(interface, wl_seat_interface.name)) {
		window->data->wayland.seat = (wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, 1);
		wl_seat_add_listener(window->data->wayland.seat, &seatListener, nullptr);
	} else if (equals(interface, wl_shm_interface.name)) {
		window->data->wayland.shm = (wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, 1);
	}
}

static void globalRegistryRemove(void *data, wl_registry *registry, u32 id) {
	cout.PrintLn("Got a registry remove event for ", id);
}

static const wl_registry_listener registryListener = {
	globalRegistryAdd,
	globalRegistryRemove
};

} // namespace wl::events

i32 createAnonymousFile(i32 size) {
	RandomNumberGenerator rng;
	String path = getenv("XDG_RUNTIME_DIR");
	String shmName;
	i32 tries = 0;
	i32 fd;
	do {
		shmName = Stringify(path, "/wayland-shm-", random(100000, 999999, &rng));
		fd = memfd_create(shmName.data, MFD_CLOEXEC);
		if (fd >= 0) {
			unlink(shmName.data);
			break;
		}
		tries++;
	} while (tries < 100 && errno == EEXIST);
	if (tries == 100) {
		return -1;
	}
	i32 ret;
	do {
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

bool windowOpenWayland(Window *window) {
	WindowData *data = window->data;
	i16 &x = window->x;
	i16 &y = window->y;
	u16 &width = window->width;
	u16 &height = window->height;
	String &name = window->name;
	bool &open = window->open;
	// u16 &dpi = window->dpi;
	// Connect to the display named by $WAYLAND_DISPLAY if it's defined
	// Or "wayland-0" if $WAYLAND_DISPLAY is not defined
	if (nullptr == (data->wayland.display = wl_display_connect(nullptr))) {
		error = "Failed to open Wayland display";
		return false;
	}
	wl_registry *registry = wl_display_get_registry(data->wayland.display);
	wl_registry_add_listener(registry, &wl::events::registryListener, (void*)window);
	wl_display_dispatch(data->wayland.display);
	wl_display_roundtrip(data->wayland.display);

	if (data->wayland.compositor == nullptr) {
		error = "Can't find compositor";
		return false;
	}

	if (nullptr == (data->wayland.surface = wl_compositor_create_surface(data->wayland.compositor))) {
		error = "Can't create surface";
		return false;
	}
	

	if (data->wayland.wmBase == nullptr) {
		error = "We don't have an xdg_wm_base";
		return false;
	}
	
	if (nullptr == (data->wayland.xdgSurface = xdg_wm_base_get_xdg_surface(data->wayland.wmBase, data->wayland.surface))) {
		error = "Can't create an xdg_surface";
		return false;
	}
	
	xdg_surface_add_listener(data->wayland.xdgSurface, &wl::events::xdgSurfaceListener, window);
	
	if (nullptr == (data->wayland.xdgToplevel = xdg_surface_get_toplevel(data->wayland.xdgSurface))) {
		error = "Can't create an xdg_toplevel";
		return false;
	}
	String windowClass = "WindowClass" + name;
	xdg_toplevel_set_app_id(data->wayland.xdgToplevel, windowClass.data);
	xdg_toplevel_set_title(data->wayland.xdgToplevel, name.data);
	xdg_toplevel_add_listener(data->wayland.xdgToplevel, &wl::events::xdgToplevelListener, window);

	if (data->wayland.seat == nullptr) {
		error = "We don't have a Wayland seat";
		return false;
	}

	wl_shm_pool *pool;
	i32 stride = width * 4;
	i32 size = stride * height;
	i32 fd = createAnonymousFile(size);
	if (fd < 0) {
		error = "Failed to create fd for shm";
		return false;
	}
	u32 *shm_data = (u32*)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shm_data == MAP_FAILED) {
		close(fd);
		error = "Failed to map shm_data";
		return false;
	}
	for (i32 i = 0; i < width*height; i++) {
		shm_data[i] = 0xff000080;
	}

	pool = wl_shm_create_pool(data->wayland.shm, fd, size);
	data->wayland.buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);


	wl_surface_attach(data->wayland.surface, data->wayland.buffer, 0, 0);

	data->wayland.region = wl_compositor_create_region(data->wayland.compositor);
	wl_region_add(data->wayland.region, x, y, width, height);
	wl_surface_set_opaque_region(data->wayland.surface, data->wayland.region);
	wl_surface_commit(data->wayland.surface);

	open = true;
	return true;
}

void windowFullscreenWayland(Window *window) {
	// TODO: Implement this
}

void windowResizeWayland(Window *window) {
	// TODO: implement this
}

bool windowUpdateWayland(Window *window, bool &changeFullscreen) {
	WindowData *data = window->data;
	// Input *input = window->input;
	// u16 &width = window->width;
	// u16 &height = window->height;
	// bool &resized = window->resized;
	// bool &focused = window->focused;
	if (wl_display_dispatch(data->wayland.display) < 0) return false;
	return !window->quit;
}

} // namespace io

} // namespace AzCore

#endif // AZCORE_WAYLAND_CPP
