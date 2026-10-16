#pragma once
#include "sdk_helpers.h"
#include <string>
#include <vector>
namespace hiface_linux_demo {
struct CameraFrame {
    int width = 0, height = 0;
    std::vector<uint8_t> rgba, i420;
    HiFaceYuvRangeV1 range = HIFACE_YUV_RANGE_VIDEO;
    HiFaceYuvMatrixV1 matrix = HIFACE_YUV_MATRIX_BT601;
    HiFaceYuv420FrameV1 view() const;
};
// Pixel adapters only; all face analysis and flow decisions remain in Core.
void yuyvFrame(const uint8_t* bytes, size_t size, int width, int height, int stride,
              HiFaceYuvRangeV1 range, HiFaceYuvMatrixV1 matrix, CameraFrame& output);
void rgbaFrame(const Image& image, CameraFrame& output);
class Camera {
public:
    Camera() = default;
    ~Camera();
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    void open(const std::string& device, int width = 640, int height = 480, int fps = 30, bool prefer_mjpeg = false);
    void close() noexcept;
    bool read(CameraFrame& frame);
    const std::string& description() const { return description_; }
    bool ready() const { return streaming_; }
private:
    struct Buffer { void* data; size_t size; };
    int fd_ = -1, width_ = 0, height_ = 0, stride_ = 0;
    uint32_t format_ = 0;
    bool streaming_ = false;
    HiFaceYuvRangeV1 range_ = HIFACE_YUV_RANGE_VIDEO;
    HiFaceYuvMatrixV1 matrix_ = HIFACE_YUV_MATRIX_BT601;
    std::vector<Buffer> buffers_;
    std::string description_;
};
}
