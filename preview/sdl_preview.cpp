/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * egl_preview.cpp - X/EGL-based preview window.
 */

#include <map>
#include <string>

// Include libcamera stuff before X11, as X11 #defines both Status and None
// which upsets the libcamera headers.

#include "core/options.hpp"

#include "preview.hpp"

#include <libdrm/drm_fourcc.h>

#include <epoxy/egl.h>
#include <epoxy/gl.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <EGL/egl.h>

class SDLPreview : public Preview
{
public:
	SDLPreview(Options const *options);
	~SDLPreview();
	virtual void SetInfoText(const std::string &text) override;
	// Display the buffer. You get given the fd back in the BufferDoneCallback
	// once its available for re-use.
	virtual void Show(int fd, libcamera::Span<uint8_t> span, StreamInfo const &info) override;
	// Reset the preview window, clearing the current buffers and being ready to
	// show new ones.
	virtual void Reset() override;
	// Check if the window manager has closed the preview.
	virtual bool Quit() override;
	// Return the maximum image size allowed.
	virtual void MaxImageSize(unsigned int &w, unsigned int &h) const override
	{
		w = max_image_width_;
		h = max_image_height_;
	}

private:
	void makeWindow(char const *name, StreamInfo const &info);
	void makeBuffer(const uint8_t *img_data);
	int last_fd_;
	bool first_time_;
	// size of preview window
	int x_;
	int y_;
	int width_;
	int height_;
	unsigned int max_image_width_;
	unsigned int max_image_height_;

	SDL_Window *sdl_window_;
	SDL_Renderer *sdl_renderer_;
	SDL_Texture *sdl_texture_;
	uint32_t sdl_pitch;
	SDL_Rect sdl_rect;
};

SDLPreview::SDLPreview(Options const *options)
	: Preview(options), last_fd_(-1), first_time_(true), sdl_window_(nullptr), sdl_renderer_(nullptr),
	  sdl_texture_(nullptr), sdl_pitch(0)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
		throw std::runtime_error("Couldn't initialize SDL");

	max_image_width_ = max_image_height_ = 0;

	SDL_DisplayMode cur_disp_mode;
	SDL_DisplayMode desk_disp_mode;
	if (SDL_GetCurrentDisplayMode(0, &cur_disp_mode) == 0)
	{
		if (SDL_GetDesktopDisplayMode(0, &desk_disp_mode) == 0)
		{
			max_image_width_ = desk_disp_mode.w;
			max_image_height_ = desk_disp_mode.h;
		}
	}

	x_ = options_->preview_x;
	y_ = options_->preview_y;
	width_ = options_->preview_width;
	height_ = options_->preview_height;

	sdl_rect.x = sdl_rect.y = sdl_rect.w = sdl_rect.h = 0;
}

SDLPreview::~SDLPreview()
{
	SDL_Quit();
}

void SDLPreview::makeWindow(char const *name, StreamInfo const &info)
{
	// Default behaviour here is to use a 1024x768 window.
	if (width_ == 0 || height_ == 0)
	{
		width_ = 1024;
		height_ = 768;
	}

	sdl_window_ = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width_, height_, 0);
	if (!sdl_window_)
		throw std::runtime_error("Couldn't create SDL window");

	sdl_renderer_ = SDL_CreateRenderer(sdl_window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!sdl_renderer_)
		throw std::runtime_error("Couldn't create SDL render");

	sdl_texture_ = SDL_CreateTexture(sdl_renderer_, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING, info.width, info.height);

	sdl_pitch = info.width;

	if ((int64_t)height_ * info.width <= (int64_t)width_ * info.height)
	{
		int w = (int)(((double)info.width * height_) / (double)info.height);
		sdl_rect.w = w;
		sdl_rect.h = height_;
		sdl_rect.x = (width_ - w) >> 1;
		sdl_rect.y = 0;
	}
	else
	{
		int h = (int)(((double)info.height * width_) / (double)info.width);
		sdl_rect.w = width_;
		sdl_rect.h = h;
		sdl_rect.x = 0;
		sdl_rect.y = (height_ - h) >> 1;
	}
}

void SDLPreview::makeBuffer(const uint8_t *img_data)
{
	SDL_UpdateTexture(sdl_texture_, NULL, img_data, sdl_pitch);
	SDL_RenderClear(sdl_renderer_);
	SDL_RenderCopy(sdl_renderer_, sdl_texture_, NULL, &sdl_rect);
	SDL_SetRenderDrawColor(sdl_renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
}

void SDLPreview::SetInfoText(const std::string &text)
{
	SDL_SetWindowTitle(sdl_window_, text.c_str());
}

void SDLPreview::Show(int fd, libcamera::Span<uint8_t> span, StreamInfo const &info)
{
	const uint8_t * img_data = span.data();

	if (first_time_)
	{
		makeWindow("libcamera-app", info);
		
		first_time_ = false;
	}
	makeBuffer(img_data);
	SDL_RenderPresent(sdl_renderer_);

	if (last_fd_ >= 0)
		done_callback_(last_fd_);
	last_fd_ = fd;
}

void SDLPreview::Reset()
{
	last_fd_ = -1;
	first_time_ = true;
}

bool SDLPreview::Quit()
{
	SDL_Event event;
	while (SDL_PollEvent(&event) > 0) {
		if (event.type == SDL_QUIT) {
			return true;
		}
	}
	return false;
}

Preview *make_sdl_preview(Options const *options)
{
	return new SDLPreview(options);
}
