/*
	File: SoftwareRendering.cpp
	Author: Philip Haynes
*/

#include "SoftwareRendering.hpp"
#include "IO/Log.hpp"

#ifdef __unix
	#define AZCORE_IO_NO_XLIB
	#include "IO/Linux/WindowData.hpp"
	#include <xcb/shm.h>
	#include <xcb/xcb_image.h>
	#include <sys/ipc.h>
	#include <sys/shm.h>
#elif defined(_WIN32)
	#include "IO/Win32/WindowData.hpp"
#else
	#error "Software Rendering hasn't been implemented for this platform."
#endif

namespace AzCore {

// io::Log cout("SoftwareRendering.log");

inline void blend(u8 &dst, u8 src, u8 alpha) {
	u16 tempDst = (u16)dst * ((u16)255 - (u16)alpha);
	u16 tempSrc = (u16)src * (u16)alpha;
	dst = (tempDst + tempSrc) >> 8;
}
inline void ColorPixelBlended(u8* buffer, Color in) {
	blend(buffer[0], in.b, in.a);
	blend(buffer[1], in.g, in.a);
	blend(buffer[2], in.r, in.a);
	buffer[3] = 255;
}
inline void ColorPixel(u8 *buffer, Color in) {
	buffer[0] = in.b;
	buffer[1] = in.g;
	buffer[2] = in.r;
	buffer[3] = 255;
}
inline void DarkenPixel(u8* buffer, u8 amount) {
	buffer[0] = max(0, buffer[0]-amount);
	buffer[1] = max(0, buffer[1]-amount);
	buffer[2] = max(0, buffer[2]-amount);
	buffer[3] = 255;
}

bool CheckBounds(vec2i &p1, vec2i &p2, i32 width, i32 height) {
	if (p1.x > p2.x) Swap(p1.x, p2.x);
	if (p1.y > p2.y) Swap(p1.y, p2.y);
	if (p1.x >= width) return false;
	if (p1.y >= height) return false;
	if (p2.x < 0) return false;
	if (p2.y < 0) return false;
	if (p1.x < 0) p1.x = 0;
	if (p1.y < 0) p1.y = 0;
	if (p2.x >= width) p2.x = width-1;
	if (p2.y >= height) p2.y = height-1;
	return true;
}

void SoftwareRenderer::ColorPixel(i32 x, i32 y, Color color) {
	u8 *pixel = &framebuffer[y*stride + x*depth];
	AzCore::ColorPixel(pixel, color);
}

void SoftwareRenderer::DarkenBox(vec2i p1, vec2i p2, u8 amount) {
	if (!CheckBounds(p1, p2, width, height)) return;
	for (i32 y = p1.y; y <= p2.y; y++) {
		u8 *line = &framebuffer[y*stride];
		for (i32 x = p1.x; x <= p2.x; x++) {
			DarkenPixel(line + x*depth, amount);
		}
	}
}
void SoftwareRenderer::DrawBox(vec2i p1, vec2i p2, Color color) {
	if (!CheckBounds(p1, p2, width, height)) return;
	for (i32 y = p1.y; y <= p2.y; y++) {
		u8 *line = &framebuffer[y*stride];
		for (i32 x = p1.x; x <= p2.x; x++) {
			AzCore::ColorPixel(line + x*depth, color);
		}
	}
}
void SoftwareRenderer::DrawBoxBlended(vec2i p1, vec2i p2, Color color) {
	if (!CheckBounds(p1, p2, width, height)) return;
	for (i32 y = p1.y; y <= p2.y; y++) {
		u8 *line = &framebuffer[y*stride];
		for (i32 x = p1.x; x <= p2.x; x++) {
			ColorPixelBlended(line + x*depth, color);
		}
	}
}

void SoftwareRenderer::DrawImage(vec2i p1, Image *image) {
	vec2i p2 = p1 + vec2i(image->width-1, image->height-1);
	if (!CheckBounds(p1, p2, width, height)) return;
	i32 imgY = 0;
	for (i32 y = p1.y; y <= p2.y; y++) {
		i32 imgX = 0;
		u8 *line = &framebuffer[y*stride];
		u8 *imgLine = &image->pixels[imgY * image->stride];
		for (i32 x = p1.x; x <= p2.x; x++) {
			Color color = *((Color*)&imgLine[imgX*image->channels]);
			AzCore::ColorPixel(line + x*depth, color);
			imgX++;
		}
		imgY++;
	}
}
void SoftwareRenderer::DrawImageBlended(vec2i p1, Image *image) {
	vec2i p2 = p1 + vec2i(image->width-1, image->height-1);
	if (!CheckBounds(p1, p2, width, height)) return;
	i32 imgY = 0;
	for (i32 y = p1.y; y <= p2.y; y++) {
		i32 imgX = 0;
		u8 *line = &framebuffer[y*stride];
		u8 *imgLine = &image->pixels[imgY * image->stride];
		for (i32 x = p1.x; x <= p2.x; x++) {
			Color color = *((Color*)&imgLine[imgX*image->channels]);
			ColorPixelBlended(line + x*depth, color);
			imgX++;
		}
		imgY++;
	}
}

#ifdef __unix

struct SWData {
	xcb_image_t *image;
	xcb_shm_segment_info_t segInfo;
	xcb_shm_seg_t shmseg;
	xcb_gcontext_t gc;
};

bool QueryShm(xcb_connection_t *connection) {
	xcb_shm_query_version_cookie_t cookie;
	xcb_shm_query_version_reply_t *reply;
	cookie = xcb_shm_query_version(connection);
	reply = xcb_shm_query_version_reply(connection, cookie, nullptr);
	if (reply) {
		free(reply);
		return true;
	}
	return false;
}

bool CreateShmImage(SoftwareRenderer &swr, io::Window *window) {
	xcb_connection_t *connection = window->data->connection;
	swr.data->image = xcb_image_create_native(window->data->connection, window->width, window->height, XCB_IMAGE_FORMAT_Z_PIXMAP, window->data->windowDepth, 0, 0xffffffff, 0);

	swr.stride = swr.data->image->stride;
	swr.depth = swr.data->image->bpp/8;
	// cout.PrintLn("Width: ", swr.width, ", Height: ", swr.height, ", Depth: ", swr.depth, ", Stride: ", swr.stride);

	swr.data->segInfo.shmid = shmget(IPC_PRIVATE, swr.data->image->stride * swr.data->image->height, IPC_CREAT | 0600);
	swr.data->segInfo.shmaddr = swr.data->image->data = (u8*)shmat(swr.data->segInfo.shmid, nullptr, 0);
	swr.data->segInfo.shmseg = xcb_generate_id(connection);

	xcb_void_cookie_t cookie;
	cookie = xcb_shm_attach_checked(window->data->connection, swr.data->segInfo.shmseg, swr.data->segInfo.shmid, 0);
	if (xcb_generic_error_t *err = xcb_request_check(window->data->connection, cookie)) {
		swr.error = Stringify("Failed to shm_attach: op: ", (u32)err->error_code, ", major: ", (u32)err->major_code, ", minor: ", (u32)err->minor_code);
		free(err);
		return false;
	}
	swr.framebuffer = (u8*)swr.data->image->data;
	return true;
}

void DestroyShmImage(SWData *data, io::Window *window) {
	xcb_shm_detach(window->data->connection, data->shmseg);
	xcb_image_destroy(data->image);
	shmdt(data->segInfo.shmaddr);
	shmctl(data->segInfo.shmid, IPC_RMID, nullptr);
}

bool SoftwareRenderer::Init() {
	if (!window->open) return false;
	width = window->width;
	height = window->height;
	xcb_connection_t *connection = window->data->connection;
	if (!QueryShm(connection)) return false;

	data->gc = xcb_generate_id(connection);
	xcb_create_gc(connection, data->gc, window->data->window, 0, 0);

	if (!CreateShmImage(*this, window)) return false;

	initted = true;
	return true;
}

bool SoftwareRenderer::Update() {
	if (window->width != width || window->height != height) {
		DestroyShmImage(data, window);
		width = window->width;
		height = window->height;
		if (!CreateShmImage(*this, window)) return false;
	}
	return true;
}
bool SoftwareRenderer::Present() {
	xcb_connection_t *connection = window->data->connection;
	xcb_image_shm_put(connection, window->data->window, data->gc, data->image, data->segInfo, 0, 0, 0, 0, width, height, false);
	xcb_flush(connection);
	return true;
}
bool SoftwareRenderer::Deinit() {
	xcb_connection_t *connection = window->data->connection;
	DestroyShmImage(data, window);
	xcb_free_gc(connection, data->gc);
	initted = false;
	return true;
}

bool SoftwareRenderer::FramebufferToImage(Image *dst) {
	dst->Alloc(width, height, 3);
	return dst->Copy(framebuffer, width, height, depth, Image::BGRA, stride, 255);
}

#elif defined(_WIN32)

struct SWData {
	HDC hdc;
	HDC mdc;
	HBITMAP hbitmap;
	HGDIOBJ oldObject;
};

bool CreateFramebufferImage(SoftwareRenderer &swr, io::Window *window) {
	SWData *data = swr.data;
	swr.width = window->width;
	swr.height = window->height;
	if (data->mdc) {
		DeleteDC(data->mdc);
	}
	if (data->hbitmap) {
		DeleteObject(data->hbitmap);
	}
	size_t bitmapInfoSize = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
	LPBITMAPINFO info = (LPBITMAPINFO)malloc(bitmapInfoSize);
	memset(info, 0, bitmapInfoSize);
	info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	{ // Get depth
		HBITMAP hbm = CreateCompatibleBitmap(data->hdc, 1, 1);
		GetDIBits(data->hdc, hbm, 0, 0, nullptr, info, DIB_RGB_COLORS);
		GetDIBits(data->hdc, hbm, 0, 0, nullptr, info, DIB_RGB_COLORS);
		DeleteObject(hbm);

		swr.depth = 4;
		if (info->bmiHeader.biCompression == BI_BITFIELDS) {
			swr.depth = info->bmiHeader.biBitCount / 8;
			// TODO: Handle various image formats
			// Right now we assume BGRA
		} else {
			// We'll determine the format
			memset(info, 0, bitmapInfoSize);
			info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			info->bmiHeader.biPlanes = 1;
			info->bmiHeader.biBitCount = 32;
			info->bmiHeader.biCompression = BI_RGB;
		}
	}

	swr.stride = align(swr.width * swr.depth, 4);
	info->bmiHeader.biWidth = swr.width;
	// Negative means top-down
	info->bmiHeader.biHeight = -swr.height;
	info->bmiHeader.biSizeImage = swr.height * swr.stride;

	data->mdc = CreateCompatibleDC(data->hdc);
	data->hbitmap = CreateDIBSection(data->hdc, info, DIB_RGB_COLORS, (void**)&swr.framebuffer, nullptr, 0);

	if (!data->hbitmap) {
		swr.error = "Failed to create DIB";
		DeleteDC(data->mdc);
		data->mdc = 0;
		return false;
	}
	data->oldObject = SelectObject(data->mdc, data->hbitmap);

	return true;
}

void DestroyFramebufferImage(SWData *data, io::Window *window) {
	if (data->mdc) {
		SelectObject(data->mdc, data->oldObject);
		DeleteDC(data->mdc);
		data->mdc = 0;
	}
	if (data->hbitmap) {
		DeleteObject(data->hbitmap);
		data->hbitmap = 0;
	}
}

bool SoftwareRenderer::Init() {
	if (!window->open) return false;
	width = window->width;
	height = window->height;
	data->hdc = GetDC(window->data->window);
	if (!CreateFramebufferImage(*this, window)) {
		ReleaseDC(window->data->window, data->hdc);
		return false;
	}
	initted = true;
	return true;
}

bool SoftwareRenderer::Update() {
	if (window->width != width || window->height != height) {
		DestroyFramebufferImage(data, window);
		width = window->width;
		height = window->height;
		if (!CreateFramebufferImage(*this, window)) return false;
	}
	return true;
}
bool SoftwareRenderer::Present() {
	BitBlt(data->hdc, 0, 0, width, height, data->mdc, 0, 0, SRCCOPY);
	return true;
}
bool SoftwareRenderer::Deinit() {
	DestroyFramebufferImage(data, window);
	ReleaseDC(window->data->window, data->hdc);
	initted = false;
	return true;
}

bool SoftwareRenderer::FramebufferToImage(Image *dst) {
	dst->Alloc(width, height, 3);
	return dst->Copy(framebuffer, width, height, depth, Image::BGRA, stride, 255);
}

#endif

SoftwareRenderer::SoftwareRenderer(io::Window *inWindow) : window(inWindow), initted(false) {
	data = new SWData;
}
SoftwareRenderer::~SoftwareRenderer() {
	if (initted)
		Deinit();
	delete data;
}

} // namespace AzCore
