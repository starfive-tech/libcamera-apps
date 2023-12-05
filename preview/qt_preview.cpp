/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd.
 *
 * qt_preview.cpp - Qt preview window
 */

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string.h>
#include <thread>

// This header must be before the QT headers, as the latter #defines slot and emit!
#include "core/options.hpp"

#include <QApplication>
#include <QImage>
#include <QMainWindow>
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>

#include "preview.hpp"

class MyMainWindow : public QMainWindow
{
public:
	MyMainWindow() : QMainWindow() {}
	bool quit = false;
protected:
	void closeEvent(QCloseEvent *event) override
	{
		event->ignore();
		quit = true;
	}
};

class MyWidget : public QWidget
{
public:
	MyWidget(QWidget *parent, int w, int h) : QWidget(parent), size(w, h)
	{
		for(int i = 0; i < 2; i++) {
			buffers_[i].image = QImage(size, QImage::Format_RGB888);
			buffers_[i].image.fill(0);
			availableBuffers_.push_back(&buffers_[i]);
		}
	}
	QSize size;

	struct ImageBuffer {
		QImage image;
		uint8_t frameCounter;

		ImageBuffer() : frameCounter(0) {};
	} buffers_[2];

	std::list<ImageBuffer *> freeBuffers_;
	std::list<ImageBuffer *> availableBuffers_;

	std::mutex buffers_available_mutex_;
	std::mutex buffers_free_mutex_;
protected:
	void paintEvent(QPaintEvent *) override
	{
		QPainter painter(this);
		{
			ImageBuffer *buffer = nullptr;
			{
			std::lock_guard<std::mutex> lock(buffers_available_mutex_);
			if(!availableBuffers_.size())
				return;
			buffer = availableBuffers_.back();
			availableBuffers_.pop_back();
			}

			painter.drawImage(rect(), buffer->image, buffer->image.rect());

			{
			std::lock_guard<std::mutex> lock(buffers_free_mutex_);
			freeBuffers_.push_back(buffer);
			}
		}
	}
	QSize sizeHint() const override { return size; }
};

class QtPreview : public Preview
{
public:
	QtPreview(Options const *options) : Preview(options)
	{
		window_width_ = options->preview_width;
		window_height_ = options->preview_height;
		if (window_width_ % 2 || window_height_ % 2)
			throw std::runtime_error("QtPreview: expect even dimensions");
		// This preview window is expensive, so make it small by default.
		if (window_width_ == 0 || window_height_ == 0)
			window_width_ = 512, window_height_ = 384;
		
		frameCounter_ = 0;
		// As a hint, reserve twice the binned width for our widest current camera (V3)
		tmp_stripe_.reserve(4608);
		thread_ = std::thread(&QtPreview::threadFunc, this, options);
		std::unique_lock lock(mutex_);
		while (!pane_)
			cond_var_.wait(lock);
		LOG(2, "Made Qt preview");
	}
	~QtPreview()
	{
		application_->exit();
		thread_.join();
	}
	void SetInfoText(const std::string &text) override { main_window_->setWindowTitle(QString::fromStdString(text)); }
	virtual void Show(int fd, libcamera::Span<uint8_t> span, StreamInfo const &info) override
	{
		MyWidget::ImageBuffer *buffer = nullptr;
		{
		std::lock_guard<std::mutex> lock(pane_->buffers_free_mutex_);
		if(pane_->freeBuffers_.size() > 0) {
			buffer = pane_->freeBuffers_.front();
			pane_->freeBuffers_.pop_front();
		}
		}

		if(!buffer) {
			std::lock_guard<std::mutex> lock(pane_->buffers_available_mutex_);
			if(!pane_->availableBuffers_.size()) {
				done_callback_(fd);
				return;
			}
			buffer = pane_->availableBuffers_.front();
			pane_->availableBuffers_.pop_front();
		}
		// Quick and simple nearest-neighbour-ish resampling is used here.
		// We further share U,V samples between adjacent output pixel pairs
		// (even when downscaling) to speed up the conversion.
		unsigned x_step = (info.width << 16) / window_width_;
		unsigned y_step = (info.height << 16) / window_height_;

		static const uint32_t YUV2RGB[3][9] = {
		{ 128, 0, 179, 128,  44,  91, 128, 227, 0 }, // JPEG
		{ 149, 0, 204, 149,  50, 104, 149, 258, 0 }, // SMPTE170M
		{ 149, 0, 230, 149,  27,  68, 149, 270, 0 }, // Rec709
		};
		static const int RGBOFFSET[3][3] = {
			{24960, -15232, 31104}, {28496, -17328, 35408}, {31824, -9776, 36944}
		};

		uint32_t coeffY, coeffVR, coeffUG, coeffVG, coeffUB;
		const int * rgbOffset = nullptr;
		if(info.colour_space == libcamera::ColorSpace::Smpte170m)
		{
			coeffY = YUV2RGB[1][0];
			coeffVR = YUV2RGB[1][2];
			coeffUG = YUV2RGB[1][4];
			coeffVG = YUV2RGB[1][5];
			coeffUB = YUV2RGB[1][7];
			rgbOffset = RGBOFFSET[1];
		} else if(info.colour_space == libcamera::ColorSpace::Rec709)
		{
			coeffY = YUV2RGB[2][0];
			coeffVR = YUV2RGB[2][2];
			coeffUG = YUV2RGB[2][4];
			coeffVG = YUV2RGB[2][5];
			coeffUB = YUV2RGB[2][7];
			rgbOffset = RGBOFFSET[2];
		} else
		{
			coeffY = YUV2RGB[0][0];
			coeffVR = YUV2RGB[0][2];
			coeffUG = YUV2RGB[0][4];
			coeffVG = YUV2RGB[0][5];
			coeffUB = YUV2RGB[0][7];
			rgbOffset = RGBOFFSET[0];
		}

		uint8_t const * Y_start = span.data();
		uint8_t const * UV_start = Y_start + info.height * info.stride;
		int src_ypos = y_step >> 1;
		uint32_t Y2 = 0;
		uint32_t U2 = coeffUG | (coeffUB << 16);
		uint32_t V2 = coeffVR | (coeffVG << 16);
		uint16_t * y2 = (uint16_t *)&Y2;
		uint32_t U;
		uint32_t V;
		uint16_t * u2 = (uint16_t *)&U;
		uint16_t * v2 = (uint16_t *)&V;

		for(unsigned int y = 0; y < window_height_; y++, src_ypos += y_step)
		{
			const uint8_t * src_y = Y_start + (src_ypos >> 16) * info.width;
			const uint8_t * src_uv = UV_start + (src_ypos >> 17) * info.width;
			uint8_t * dest = buffer->image.scanLine(y);
			uint32_t x_pos = x_step >> 1;

			for(unsigned int x = 0; x < window_width_; x += 2)
			{
				y2[0] = src_y[x_pos >> 16];
				x_pos += x_step;
				y2[1] = src_y[x_pos >> 16];
				U = src_uv[(x_pos >> 16) & 0xfffffffe];
				V = src_uv[(x_pos >> 16) | 1];
				x_pos += x_step;

				Y2 *= coeffY;
				U *= U2;
				V *= V2;

				int R0 = ((int)y2[0] + (int)v2[0] - rgbOffset[0]) >> 7;
				int G0 = ((int)y2[0] - (int)u2[0] - (int)v2[1] - rgbOffset[1]) >> 7;
				int B0 = ((int)y2[0] + (int)u2[1] - rgbOffset[2]) >> 7;
				int R1 = ((int)y2[1] + (int)v2[0] - rgbOffset[0]) >> 7;
				int G1 = ((int)y2[1] - (int)u2[0] - (int)v2[1] - rgbOffset[1]) >> 7;
				int B1 = ((int)y2[1] + (int)u2[1] - rgbOffset[2]) >> 7;
				*(dest++) = std::clamp(R0, 0, 255);
				*(dest++) = std::clamp(G0, 0, 255);
				*(dest++) = std::clamp(B0, 0, 255);
				*(dest++) = std::clamp(R1, 0, 255);
				*(dest++) = std::clamp(G1, 0, 255);
				*(dest++) = std::clamp(B1, 0, 255);
			}
		}

		{
		std::lock_guard<std::mutex> lock(pane_->buffers_available_mutex_);
		buffer->frameCounter = ++frameCounter_;
		pane_->availableBuffers_.push_back(buffer);
		}

		pane_->update();

		// Return the buffer to the camera system.
		done_callback_(fd);
	}
	// Reset the preview window, clearing the current buffers and being ready to
	// show new ones.
	void Reset() override {}
	// Check if preview window has been shut down.
	bool Quit() override { return main_window_->quit; }
	// There is no particular limit to image sizes, though large images will be very slow.
	virtual void MaxImageSize(unsigned int &w, unsigned int &h) const override { w = h = 0; }

private:
	void threadFunc(Options const *options)
	{
		// This acts as Qt's event loop. Really Qt prefers to own the application's event loop
		// but we've supplied our own and only want Qt for rendering. This works, but I
		// wouldn't write a proper Qt application like this.
		int argc = 0;
		char **argv = NULL;
		QApplication application(argc, argv);
		application_ = &application;
		MyMainWindow main_window;
		main_window_ = &main_window;
		MyWidget pane(&main_window, window_width_, window_height_);
		main_window.setCentralWidget(&pane);
		// Need to get the window border sizes (it seems to be unreasonably difficult...)
		main_window.move(options->preview_x + 2, options->preview_y + 28);
		main_window.show();
		pane_ = &pane;
		cond_var_.notify_one();
		application.exec();
	}
	QApplication *application_ = nullptr;
	MyMainWindow *main_window_ = nullptr;
	MyWidget *pane_ = nullptr;
	std::thread thread_;
	unsigned int window_width_, window_height_;
	std::mutex mutex_;
	std::condition_variable cond_var_;
	std::vector<uint8_t> tmp_stripe_;
	uint8_t frameCounter_;
};

Preview *make_qt_preview(Options const *options)
{
	return new QtPreview(options);
}
