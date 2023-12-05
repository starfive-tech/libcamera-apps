/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * drm_preview.cpp - DRM-based preview window.
 */

#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <libcamera/formats.h>

#include "core/options.hpp"

#include "preview.hpp"

class DRMObject
{
public:
	DRMObject(int drmfd, uint32_t id, uint32_t type) 
		: id_(id), type_(type){
		drmModeObjectProperties *properties = drmModeObjectGetProperties(drmfd, id, type);
		if (!properties)
			return;

		drmModePropertyPtr property;
		for (uint32_t i = 0; i < properties->count_props; i++) {
			property = drmModeGetProperty(drmfd, properties->props[i]);
			property_[std::string(property->name)] = property->prop_id;
			drmModeFreeProperty(property);
		}

		drmModeFreeObjectProperties(properties);
	}

	uint32_t getID() const {return id_;};

public:
	std::unordered_map<std::string, uint32_t> property_;

private:
	uint32_t id_;
	uint32_t type_;
};

class AtomicRequest
{
public:
	enum Flags {
		FlagAllowModeset = (1 << 0),
		FlagAsync = (1 << 1),
		FlagTestOnly = (1 << 2),
	};

	AtomicRequest() : valid_(true) {
		request_ = drmModeAtomicAlloc();
		if (!request_)
			valid_ = false;
	}

	~AtomicRequest() {
		if (request_)
			drmModeAtomicFree(request_);
	}

	bool isValid() const { return valid_; }

	int addProperty(int drmfd, const DRMObject *object, const std::string &propertyName,
			uint64_t value) {

		if (!valid_)
			return -EINVAL;
		
		auto it = object->property_.find(propertyName);
		if(it == object->property_.end())
			return -EINVAL;

		if(drmModeAtomicAddProperty(request_, object->getID(), it->second, value) < 0) {
			std::string errMsg = "drmModeAtomicAddProperty() failed -- object: " +
				std::to_string(object->getID()) + ", property: " + std::to_string(it->second);
			throw std::runtime_error(errMsg);
		}

		return 0;
	}
	
	int commit(int drmfd, unsigned int flags = 0) {
		if(drmModeAtomicCommit(drmfd, request_, flags, NULL) < 0)
			throw std::runtime_error("drmModeAtomicCommit() failed");
		return 0;
	}

private:
	AtomicRequest(const AtomicRequest &) = delete;
	AtomicRequest(const AtomicRequest &&) = delete;
	AtomicRequest &operator=(const AtomicRequest &) = delete;
	AtomicRequest &operator=(const AtomicRequest &&) = delete;

	bool valid_;
	drmModeAtomicReq *request_;
};

class DrmPreview : public Preview
{
public:
	DrmPreview(Options const *options);
	~DrmPreview();
	// Display the buffer. You get given the fd back in the BufferDoneCallback
	// once its available for re-use.
	virtual void Show(int fd, libcamera::Span<uint8_t> span, StreamInfo const &info) override;
	// Reset the preview window, clearing the current buffers and being ready to
	// show new ones.
	virtual void Reset() override;
	// Return the maximum image size allowed.
	virtual void MaxImageSize(unsigned int &w, unsigned int &h) const override
	{
		w = max_image_width_;
		h = max_image_height_;
	}

private:
	struct Buffer
	{
		Buffer() : fd(-1) {}
		int fd;
		size_t size;
		StreamInfo info;
		uint32_t bo_handle;
		unsigned int fb_handle;
	};
	void makeBuffer(int fd, size_t size, StreamInfo const &info, Buffer &buffer);
	void findCrtc();
	void findPlane();
	void fitDisplaySize();
	int drmfd_;
	int conId_;
	drmModeConnector *con_;
	uint32_t crtcId_;
	int crtcIdx_;
	uint32_t planeId_;
	unsigned int out_fourcc_;
	unsigned int x_;
	unsigned int y_;
	unsigned int width_;
	unsigned int height_;
	unsigned int screen_width_;
	unsigned int screen_height_;
	unsigned int display_x_;
	unsigned int display_y_;
	unsigned int display_w_;
	unsigned int display_h_;
	std::map<int, Buffer> buffers_; // map the DMABUF's fd to the Buffer
	int last_fd_;
	unsigned int max_image_width_;
	unsigned int max_image_height_;
	bool first_time_;

	std::unique_ptr<DRMObject> connector_;
	std::unique_ptr<DRMObject> crtc_;
};

#define ERRSTR strerror(errno)

void DrmPreview::findCrtc()
{
	int i;
	drmModeRes *res = drmModeGetResources(drmfd_);
	if (!res)
		throw std::runtime_error("drmModeGetResources failed: " + std::string(ERRSTR));

	if (res->count_crtcs <= 0)
		throw std::runtime_error("drm: no crts");

	max_image_width_ = res->max_width;
	max_image_height_ = res->max_height;

	if (!conId_)
	{
		LOG(2, "No connector ID specified.  Choosing default from list:");

		for (i = 0; i < res->count_connectors; i++)
		{
			drmModeConnector *con = drmModeGetConnector(drmfd_, res->connectors[i]);
			drmModeEncoder *enc = NULL;
			drmModeCrtc *crtc = NULL;

			if (con->encoder_id)
			{
				enc = drmModeGetEncoder(drmfd_, con->encoder_id);
			} else
			{
				enc = drmModeGetEncoder(drmfd_, con->encoders[0]);
			}

			if (enc->crtc_id)
			{
				crtc = drmModeGetCrtc(drmfd_, enc->crtc_id);
			} else
			{
				crtc = drmModeGetCrtc(drmfd_, res->crtcs[0]);
			}

			if (!conId_ && crtc)
			{
				conId_ = con->connector_id;
				crtcId_ = crtc->crtc_id;
			}

			if (crtc)
			{
				screen_width_ = crtc->width;
				screen_height_ = crtc->height;
			}

			LOG(2, "Connector " << con->connector_id << " (crtc " << (crtc ? crtc->crtc_id : 0) << "): type "
								<< con->connector_type << ", " << (crtc ? crtc->width : 0) << "x"
								<< (crtc ? crtc->height : 0) << (conId_ == (int)con->connector_id ? " (chosen)" : ""));
		}

		if (!conId_)
			throw std::runtime_error("No suitable enabled connector found");
	}

	crtcIdx_ = -1;

	for (i = 0; i < res->count_crtcs; ++i)
	{
		if (crtcId_ == res->crtcs[i])
		{
			crtcIdx_ = i;
			break;
		}
	}

	if (crtcIdx_ == -1)
	{
		drmModeFreeResources(res);
		throw std::runtime_error("drm: CRTC " + std::to_string(crtcId_) + " not found");
	}

	if (res->count_connectors <= 0)
	{
		drmModeFreeResources(res);
		throw std::runtime_error("drm: no connectors");
	}

	drmModeConnector *c;
	c = drmModeGetConnector(drmfd_, conId_);
	if (!c)
	{
		drmModeFreeResources(res);
		throw std::runtime_error("drmModeGetConnector failed: " + std::string(ERRSTR));
	}

	if (!c->count_modes)
	{
		drmModeFreeConnector(c);
		drmModeFreeResources(res);
		throw std::runtime_error("connector supports no mode");
	}

	con_ = c;

	if (options_->fullscreen || width_ == 0 || height_ == 0)
	{
		drmModeCrtc *crtc = drmModeGetCrtc(drmfd_, crtcId_);
		x_ = crtc->x;
		y_ = crtc->y;
		width_ = crtc->width;
		height_ = crtc->height;
		drmModeFreeCrtc(crtc);
	}
}

void DrmPreview::findPlane()
{
	drmModePlaneResPtr planes;
	drmModePlanePtr plane;
	unsigned int i;
	unsigned int j;

	planes = drmModeGetPlaneResources(drmfd_);
	if (!planes)
		throw std::runtime_error("drmModeGetPlaneResources failed: " + std::string(ERRSTR));

	try
	{
		for (i = 0; i < planes->count_planes; ++i)
		{
			plane = drmModeGetPlane(drmfd_, planes->planes[i]);
			if (!planes)
				throw std::runtime_error("drmModeGetPlane failed: " + std::string(ERRSTR));

			if (!(plane->possible_crtcs & (1 << crtcIdx_)))
			{
				drmModeFreePlane(plane);
				continue;
			}

			for (j = 0; j < plane->count_formats; ++j)
			{
				if (plane->formats[j] == out_fourcc_)
				{
					break;
				}
			}

			if (j == plane->count_formats)
			{
				drmModeFreePlane(plane);
				continue;
			}

			planeId_ = plane->plane_id;

			drmModeFreePlane(plane);
			break;
		}
		planeId_ = planes->planes[0];
	}
	catch (std::exception const &e)
	{
		drmModeFreePlaneResources(planes);
		throw;
	}

	drmModeFreePlaneResources(planes);
}

DrmPreview::DrmPreview(Options const *options) : Preview(options), last_fd_(-1), first_time_(true)
{
	//drmfd_ = drmOpen("vc4", NULL);
	drmfd_ = drmOpen("starfive", NULL);
	if (drmfd_ < 0)
		throw std::runtime_error("drmOpen failed: " + std::string(ERRSTR));

	drmSetClientCap(drmfd_, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

	x_ = options_->preview_x;
	y_ = options_->preview_y;
	width_ = options_->preview_width;
	height_ = options_->preview_height;
	screen_width_ = 0;
	screen_height_ = 0;

	try
	{
		if (!drmIsMaster(drmfd_))
			throw std::runtime_error("DRM preview unavailable - not master");

		conId_ = 0;
		findCrtc();
		//out_fourcc_ = DRM_FORMAT_YUV420;
		out_fourcc_ = DRM_FORMAT_NV12;
		findPlane();

		drmSetClientCap(drmfd_, DRM_CLIENT_CAP_ATOMIC, 1);
		connector_ = std::make_unique<DRMObject>(drmfd_, conId_, DRM_MODE_OBJECT_CONNECTOR);
		crtc_ = std::make_unique<DRMObject>(drmfd_, crtcId_, DRM_MODE_OBJECT_CRTC);
	}
	catch (std::exception const &e)
	{
		close(drmfd_);
		throw;
	}
}

DrmPreview::~DrmPreview()
{
	close(drmfd_);
}

// DRM doesn't seem to have userspace definitions of its enums, but the properties
// contain enum-name-to-value tables. So the code below ends up using strings and
// searching for name matches. I suppose it works...

static void get_colour_space_info(std::optional<libcamera::ColorSpace> const &cs, char const *&encoding,
								  char const *&range)
{
	static char const encoding_601[] = "601", encoding_709[] = "709";
	static char const range_limited[] = "limited", range_full[] = "full";
	encoding = encoding_601;
	range = range_limited;

	if (cs == libcamera::ColorSpace::Sycc)
		range = range_full;
	else if (cs == libcamera::ColorSpace::Smpte170m)
		/* all good */;
	else if (cs == libcamera::ColorSpace::Rec709)
		encoding = encoding_709;
	else
		LOG(1, "DrmPreview: unexpected colour space " << libcamera::ColorSpace::toString(cs));
}

static int drm_set_property(int fd, int plane_id, char const *name, char const *val)
{
	drmModeObjectPropertiesPtr properties = nullptr;
	drmModePropertyPtr prop = nullptr;
	int ret = -1;
	properties = drmModeObjectGetProperties(fd, plane_id, DRM_MODE_OBJECT_PLANE);

	for (unsigned int i = 0; i < properties->count_props; i++)
	{
		int prop_id = properties->props[i];
		prop = drmModeGetProperty(fd, prop_id);
		if (!prop)
			continue;

		if (!drm_property_type_is(prop, DRM_MODE_PROP_ENUM) || !strstr(prop->name, name))
		{
			drmModeFreeProperty(prop);
			prop = nullptr;
			continue;
		}

		// We have found the right property from its name, now search the enum table
		// for the numerical value that corresponds to the value name that we have.
		for (int j = 0; j < prop->count_enums; j++)
		{
			if (!strstr(prop->enums[j].name, val))
				continue;

			ret = drmModeObjectSetProperty(fd, plane_id, DRM_MODE_OBJECT_PLANE, prop_id, prop->enums[j].value);
			if (ret < 0)
				LOG(1, "DrmPreview: failed to set value " << val << " for property " << name);
			goto done;
		}

		LOG(1, "DrmPreview: failed to find value " << val << " for property " << name);
		goto done;
	}

	LOG(1, "DrmPreview: failed to find property " << name);
done:
	if (prop)
		drmModeFreeProperty(prop);
	if (properties)
		drmModeFreeObjectProperties(properties);
	return ret;
}

static void setup_colour_space(int fd, int plane_id, std::optional<libcamera::ColorSpace> const &cs)
{
	char const *encoding, *range;
	get_colour_space_info(cs, encoding, range);

	drm_set_property(fd, plane_id, "COLOR_ENCODING", encoding);
	drm_set_property(fd, plane_id, "COLOR_RANGE", range);
}

void DrmPreview::fitDisplaySize()
{
	if((int64_t)width_ * screen_height_ > (int64_t)screen_width_ * height_) {
		display_x_ = 0, display_w_ = screen_width_;
		display_h_ = ((int64_t)screen_width_ * height_) / (int64_t)width_;
		display_y_ = (screen_height_ - display_h_) >> 1;
	} else {
		display_y_ = 0, display_h_ = screen_height_;
		display_w_ = ((int64_t)width_ * screen_height_) / (int64_t)height_;
		display_x_ = (screen_width_ - display_w_) >> 1;
	}
}

void DrmPreview::makeBuffer(int fd, size_t size, StreamInfo const &info, Buffer &buffer)
{
	unsigned int width = info.width, height = info.height, stride = info.stride;
	int64_t targetArea = 0;
	if (first_time_)
	{
		first_time_ = false;

		setup_colour_space(drmfd_, planeId_, info.colour_space);
	}

	buffer.fd = fd;
	buffer.size = size;
	buffer.info = info;

	if(!width_ || !height)
		width_ = info.width, height_ = info.height;
	targetArea = (int64_t)width_ * height_;

	if (drmPrimeFDToHandle(drmfd_, fd, &buffer.bo_handle))
		throw std::runtime_error("drmPrimeFDToHandle failed for fd " + std::to_string(fd));

	if (out_fourcc_ == DRM_FORMAT_YUV420) {
		uint32_t offsets[4] = { 0, stride * height, stride * height + (stride / 2) * (height / 2) };
		uint32_t pitches[4] = { stride, stride / 2, stride / 2 };
		uint32_t bo_handles[4] = { buffer.bo_handle, buffer.bo_handle, buffer.bo_handle };

		if (drmModeAddFB2(drmfd_, width, height, out_fourcc_, bo_handles, pitches, offsets, &buffer.fb_handle, 0))
			throw std::runtime_error("YUV420 drmModeAddFB2 failed: " + std::string(ERRSTR));
	} else if (out_fourcc_ == DRM_FORMAT_NV12 || out_fourcc_ == DRM_FORMAT_NV21) {
		uint32_t offsets[4] = { 0, stride * height};
		uint32_t pitches[4] = { stride, stride};
		uint32_t bo_handles[4] = { buffer.bo_handle, buffer.bo_handle};

		if (drmModeAddFB2(drmfd_, width, height, out_fourcc_, bo_handles, pitches, offsets, &buffer.fb_handle, 0))
			throw std::runtime_error("NV12/21 drmModeAddFB2 failed: " + std::string(ERRSTR));
	} else
		throw std::runtime_error("drmModeAddFB2 failed: " + std::string(ERRSTR));

	/* find preferred mode */
	drmModeModeInfo *modeptr = NULL, *preferred = NULL;
	int64_t minDiff = -1;
	for (int m = 0; m < con_->count_modes; m++) {
		int64_t area, areaDiff;
		modeptr = &con_->modes[m];

		area = (int64_t)modeptr->hdisplay * modeptr->vdisplay;
		areaDiff = abs(targetArea - area);
		if(-1 == minDiff)
			minDiff = abs(targetArea - area), preferred = modeptr;
		else {
			if(areaDiff < minDiff)
				minDiff = areaDiff, preferred = modeptr;
		}

		if (!minDiff) {
			preferred = modeptr;
			std::cout << "find the matched mode, modes index= "
				<< m << ", " << width << "x" << height << std::endl;
			break;
		}
	}

	screen_width_ = preferred->hdisplay;
	screen_height_ = preferred->vdisplay;
	fitDisplaySize();

	AtomicRequest ar;
	uint32_t blob_id;

	drmModeCreatePropertyBlob(drmfd_, preferred, sizeof(con_->modes[0]), &blob_id);
	ar.addProperty(drmfd_, connector_.get(), "CRTC_ID", crtcId_);
	ar.addProperty(drmfd_, crtc_.get(), "ACTIVE", 1);
	ar.addProperty(drmfd_, crtc_.get(), "MODE_ID", blob_id);
	ar.commit(drmfd_, DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_ATOMIC_ALLOW_MODESET);

	// set default
	//if (drmModeSetCrtc(drmfd_, crtcId_, buffer.fb_handle, 0, 0,
	//        (uint32_t *)&conId_, 1, preferred)) {
	//	throw std::runtime_error("drmModeSetCrtc() failed");
	//}
}

void DrmPreview::Show(int fd, libcamera::Span<uint8_t> span, StreamInfo const &info)
{
	Buffer &buffer = buffers_[fd];
	if (buffer.fd == -1)
		makeBuffer(fd, span.size(), info, buffer);

	if (drmModeSetPlane(drmfd_, planeId_, crtcId_, buffer.fb_handle, 0, display_x_, display_y_, display_w_, display_h_, 0, 0,
						buffer.info.width << 16, buffer.info.height << 16))
		throw std::runtime_error("drmModeSetPlane failed: " + std::string(ERRSTR));
	if (last_fd_ >= 0)
		done_callback_(last_fd_);
	last_fd_ = fd;
}

void DrmPreview::Reset()
{
	for (auto &it : buffers_)
	{
		drmModeRmFB(drmfd_, it.second.fb_handle);
		// Apparently a "bo_handle" is a "gem" thing, and it needs closing. It feels like there
		// ought be an API to match "drmPrimeFDToHandle" for this, but I can only find an ioctl.
		drm_gem_close gem_close = {};
		gem_close.handle = it.second.bo_handle;
		if (drmIoctl(drmfd_, DRM_IOCTL_GEM_CLOSE, &gem_close) < 0)
			// I have no idea what this would mean, so complain and try to carry on...
			LOG(1, "DRM_IOCTL_GEM_CLOSE failed");
	}
	buffers_.clear();
	last_fd_ = -1;
	first_time_ = true;
}

Preview *make_drm_preview(Options const *options)
{
	return new DrmPreview(options);
}
