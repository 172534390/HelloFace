#include "camera.h"
#include "image_decode.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
namespace hiface_linux_demo {
namespace {
int request(int fd, unsigned long code, void* value) {
    int result;
    do { result = ioctl(fd, code, value); } while (result < 0 && errno == EINTR);
    return result;
}
void cameraCheck(bool okay, const std::string& operation) {
    require(okay, operation + ": " + std::strerror(errno));
}
}
Camera::~Camera() { close(); }
void Camera::close() noexcept {
    if (streaming_) { int type = V4L2_BUF_TYPE_VIDEO_CAPTURE; request(fd_, VIDIOC_STREAMOFF, &type); }
    streaming_ = false;
    for (auto& buffer : buffers_) munmap(buffer.data, buffer.size);
    buffers_.clear();
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
}
void Camera::open(const std::string& device, int width, int height, int fps, bool prefer_mjpeg) {
    close();
    try {
        fd_ = ::open(device.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
        cameraCheck(fd_ >= 0, "Cannot open " + device + " (check video-device permission)");
        v4l2_capability capability{};
        cameraCheck(request(fd_, VIDIOC_QUERYCAP, &capability) == 0, "Query camera");
        const auto caps = capability.capabilities & V4L2_CAP_DEVICE_CAPS ? capability.device_caps : capability.capabilities;
        require((caps & V4L2_CAP_VIDEO_CAPTURE) && (caps & V4L2_CAP_STREAMING), "Device is not a streaming camera");
        bool supported = false;
        for (uint32_t wanted : prefer_mjpeg ? std::vector<uint32_t>{V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_YUYV}
                                           : std::vector<uint32_t>{V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_MJPEG}) {
            v4l2_format format{};
            format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            format.fmt.pix.width = width; format.fmt.pix.height = height;
            format.fmt.pix.pixelformat = wanted; format.fmt.pix.field = V4L2_FIELD_ANY;
            if (request(fd_, VIDIOC_S_FMT, &format) != 0 || format.fmt.pix.pixelformat != wanted) continue;
            width_ = format.fmt.pix.width; height_ = format.fmt.pix.height; format_ = wanted;
            stride_ = format.fmt.pix.bytesperline ? format.fmt.pix.bytesperline : width_ * 2;
            const auto encoding = format.fmt.pix.ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT
                ? V4L2_MAP_YCBCR_ENC_DEFAULT(format.fmt.pix.colorspace) : format.fmt.pix.ycbcr_enc;
            const auto quantization = format.fmt.pix.quantization == V4L2_QUANTIZATION_DEFAULT
                ? V4L2_MAP_QUANTIZATION_DEFAULT(false, format.fmt.pix.colorspace, encoding) : format.fmt.pix.quantization;
            require(wanted == V4L2_PIX_FMT_MJPEG || encoding == V4L2_YCBCR_ENC_601 || encoding == V4L2_YCBCR_ENC_709,
                "Camera YUV matrix must be BT.601 or BT.709");
            range_ = quantization == V4L2_QUANTIZATION_FULL_RANGE ? HIFACE_YUV_RANGE_FULL : HIFACE_YUV_RANGE_VIDEO;
            matrix_ = encoding == V4L2_YCBCR_ENC_709 ? HIFACE_YUV_MATRIX_BT709 : HIFACE_YUV_MATRIX_BT601;
            supported = true; break;
        }
        require(supported, "Camera must support YUYV or MJPEG streaming");
        require(width_ >= 2 && height_ >= 2 && !(width_ & 1) && !(height_ & 1)
            && width_ <= 1920 && height_ <= 1080, "Camera resolution exceeds 1920x1080");
        v4l2_streamparm parameters{}; parameters.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parameters.parm.capture.timeperframe = {1, static_cast<uint32_t>(fps)};
        request(fd_, VIDIOC_S_PARM, &parameters); // Driver may select another rate.
        v4l2_streamparm negotiated{}; negotiated.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        const bool has_rate = request(fd_, VIDIOC_G_PARM, &negotiated) == 0
            && negotiated.parm.capture.timeperframe.numerator && negotiated.parm.capture.timeperframe.denominator;
        v4l2_requestbuffers allocation{};
        allocation.count = 4; allocation.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; allocation.memory = V4L2_MEMORY_MMAP;
        cameraCheck(request(fd_, VIDIOC_REQBUFS, &allocation) == 0, "Allocate camera buffers");
        require(allocation.count >= 2 && allocation.count <= 32, "Invalid camera buffer count");
        buffers_.reserve(allocation.count);
        for (uint32_t i = 0; i < allocation.count; ++i) {
            v4l2_buffer buffer{}; buffer.type = allocation.type; buffer.memory = allocation.memory; buffer.index = i;
            cameraCheck(request(fd_, VIDIOC_QUERYBUF, &buffer) == 0, "Query camera buffer");
            require(buffer.length > 0 && buffer.length <= 32 * 1024 * 1024, "Invalid camera buffer length");
            auto* bytes = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buffer.m.offset);
            cameraCheck(bytes != MAP_FAILED, "Map camera buffer");
            buffers_.push_back({bytes, buffer.length});
            cameraCheck(request(fd_, VIDIOC_QBUF, &buffer) == 0, "Queue camera buffer");
        }
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        cameraCheck(request(fd_, VIDIOC_STREAMON, &type) == 0, "Start camera");
        streaming_ = true;
        description_ = device + "  " + std::to_string(width_) + "x" + std::to_string(height_) +
            (format_ == V4L2_PIX_FMT_YUYV ? " YUYV" : " MJPEG");
        if (has_rate) description_ += " " + std::to_string(double(negotiated.parm.capture.timeperframe.denominator)
            / negotiated.parm.capture.timeperframe.numerator) + " FPS";
        else description_ += " (requested " + std::to_string(fps) + " FPS)";
    } catch (...) { close(); throw; }
}
bool Camera::read(CameraFrame& frame) {
    if (!streaming_) return false;
    pollfd descriptor{fd_, POLLIN, 0};
    const int polled = poll(&descriptor, 1, 0);
    if (polled < 0 && errno == EINTR) return false;
    cameraCheck(polled >= 0 && !(descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)), "Camera disconnected");
    if (!polled) return false;
    v4l2_buffer buffer{}; buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; buffer.memory = V4L2_MEMORY_MMAP;
    if (request(fd_, VIDIOC_DQBUF, &buffer) < 0) {
        if (errno == EAGAIN) return false;
        cameraCheck(false, "Read camera buffer");
    }
    // Inference can take longer than one camera interval. Discard queued older
    // buffers and analyze the newest available frame instead of building lag.
    for (size_t i = 1; i < buffers_.size(); ++i) {
        v4l2_buffer newer{}; newer.type = buffer.type; newer.memory = buffer.memory;
        if (request(fd_, VIDIOC_DQBUF, &newer) < 0) {
            if (errno == EAGAIN) break;
            request(fd_, VIDIOC_QBUF, &buffer);
            cameraCheck(false, "Drain camera queue");
        }
        cameraCheck(request(fd_, VIDIOC_QBUF, &buffer) == 0, "Return stale camera frame");
        buffer = newer;
    }
    require(buffer.index < buffers_.size(), "Camera returned invalid buffer index");
    if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
        cameraCheck(request(fd_, VIDIOC_QBUF, &buffer) == 0, "Return damaged camera frame");
        return false;
    }
    try {
        require(buffer.bytesused > 0 && buffer.bytesused <= buffers_[buffer.index].size, "Truncated camera buffer");
        const auto* pixels = static_cast<const uint8_t*>(buffers_[buffer.index].data);
        if (format_ == V4L2_PIX_FMT_YUYV)
            yuyvFrame(pixels, buffer.bytesused, width_, height_, stride_, range_, matrix_, frame);
        else {
            const auto image = decodeJpeg(pixels, buffer.bytesused);
            require(image.width == width_ && image.height == height_, "Camera JPEG dimensions differ from negotiated video size");
            rgbaFrame(image, frame);
        }
    } catch (...) { request(fd_, VIDIOC_QBUF, &buffer); throw; }
    cameraCheck(request(fd_, VIDIOC_QBUF, &buffer) == 0, "Return camera buffer");
    return true;
}
}
