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

static void shell_surface_ping(void *data, wl_shell_surface *shell_surface, u32 serial) {
	wl_shell_surface_pong(shell_surface, serial);
	cout.PrintLn("Ponged yer Ping, bitch");
}

static void shell_surface_configure(void *data, wl_shell_surface *shell_surface, u32 edges, i32 width, i32 height) {
	Window *window = (Window*)data;
	window->width = width;
	window->height = height;
	window->resized = true;
}

static const wl_shell_surface_listener shell_surface_listener = {
	shell_surface_ping,
	shell_surface_configure
};

static void seat_handle_capabilities(void *data, wl_seat *seat, u32 caps) {
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

static const wl_seat_listener seat_listener = {
	seat_handle_capabilities
};

static void global_registry_add(void *data, wl_registry *registry, u32 id, const char *interface, u32 version) {
	Window *window = (Window*)data;
	cout.PrintLn("Got a registry add event for ", interface, " id ", id);
	if (equals(interface, "wl_compositor")) {
		window->data->wayland.compositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	} else if (equals(interface, "wl_shell")) {
		window->data->wayland.shell = (wl_shell*)wl_registry_bind(registry, id, &wl_shell_interface, 1);
	} else if (equals(interface, "wl_seat")) {
		window->data->wayland.seat = (wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, 1);
		wl_seat_add_listener(window->data->wayland.seat, &seat_listener, nullptr);
	} else if (equals(interface, "wl_shm")) {
		window->data->wayland.shm = (wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, 1);
	}
}

static void global_registry_remove(void *data, wl_registry *registry, u32 id) {
	cout.PrintLn("Got a registry remove event for ", id);
}

static const wl_registry_listener registry_listener = {
	global_registry_add,
	global_registry_remove
};

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
	wl_registry_add_listener(registry, &registry_listener, (void*)window);
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

	if (data->wayland.shell == nullptr) {
		error = "We don't have a Wayland shell";
		return false;
	}

	if (nullptr == (data->wayland.shell_surface = wl_shell_get_shell_surface(data->wayland.shell, data->wayland.surface))) {
		error = "Can't create a shell surface";
		return false;
	}
	String windowClass = "WindowClass" + name;
	wl_shell_surface_set_class(data->wayland.shell_surface, windowClass.data);
	wl_shell_surface_set_title(data->wayland.shell_surface, name.data);
	wl_shell_surface_set_toplevel(data->wayland.shell_surface);
	wl_shell_surface_add_listener(data->wayland.shell_surface, &shell_surface_listener, window);

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
		shm_data[i] = 0xff000000;
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
	return true;
}

} // namespace io

} // namespace AzCore

#endif // AZCORE_WAYLAND_CPP
