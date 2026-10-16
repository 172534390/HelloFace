#include "camera.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace hiface_linux_demo {
namespace {
uint8_t clip(double value) { return static_cast<uint8_t>(std::clamp(std::lround(value), 0L, 255L)); }
void preview(CameraFrame& frame) {
    const size_t count = static_cast<size_t>(frame.width) * frame.height;
    frame.rgba.resize(count * 4);
    for (int y = 0; y < frame.height; ++y) for (int x = 0; x < frame.width; ++x) {
        const size_t offset = static_cast<size_t>(y) * frame.width + x;
        const size_t chroma = static_cast<size_t>(y / 2) * (frame.width / 2) + x / 2;
        const double luma = frame.range == HIFACE_YUV_RANGE_VIDEO
            ? (frame.i420[offset] - 16) * (255.0 / 219) : frame.i420[offset];
        const double u = (frame.i420[count + chroma] - 128) *
            (frame.range == HIFACE_YUV_RANGE_VIDEO ? 255.0 / 224 : 1);
        const double v = (frame.i420[count * 5 / 4 + chroma] - 128) *
            (frame.range == HIFACE_YUV_RANGE_VIDEO ? 255.0 / 224 : 1);
        const bool bt709 = frame.matrix == HIFACE_YUV_MATRIX_BT709;
        frame.rgba[offset * 4] = clip(luma + (bt709 ? 1.5748 : 1.402) * v);
        frame.rgba[offset * 4 + 1] = clip(luma - (bt709 ? .187324 : .344136) * u - (bt709 ? .468124 : .714136) * v);
        frame.rgba[offset * 4 + 2] = clip(luma + (bt709 ? 1.8556 : 1.772) * u);
        frame.rgba[offset * 4 + 3] = 255;
    }
}
}
HiFaceYuv420FrameV1 CameraFrame::view() const {
    const size_t count = static_cast<size_t>(width) * height;
    HiFaceYuv420FrameV1 frame{};
    frame.struct_size = sizeof(frame); frame.width = width; frame.height = height;
    frame.y = {i420.data(), count, width, 1};
    frame.u = {i420.data() + count, count / 4, width / 2, 1};
    frame.v = {i420.data() + count * 5 / 4, count / 4, width / 2, 1};
    frame.range = range; frame.matrix = matrix;
    return frame;
}
void yuyvFrame(const uint8_t* bytes, size_t size, int width, int height, int stride,
              HiFaceYuvRangeV1 range, HiFaceYuvMatrixV1 matrix, CameraFrame& frame) {
    require(bytes && width >= 2 && height >= 2 && !(width & 1) && !(height & 1)
        && width <= 1920 && height <= 1080 && stride >= width * 2
        && size >= static_cast<size_t>(stride) * (height - 1) + width * 2, "Invalid YUYV frame");
    frame.width = width; frame.height = height; frame.range = range; frame.matrix = matrix;
    const size_t count = static_cast<size_t>(width) * height;
    frame.i420.resize(count * 3 / 2);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; x += 2) {
        const auto* input = bytes + static_cast<size_t>(y) * stride + x * 2;
        frame.i420[static_cast<size_t>(y) * width + x] = input[0];
        frame.i420[static_cast<size_t>(y) * width + x + 1] = input[2];
        if (!(y & 1)) {
            const size_t uv = static_cast<size_t>(y / 2) * (width / 2) + x / 2;
            frame.i420[count + uv] = (unsigned(input[1]) + input[stride + 1] + 1) / 2;
            frame.i420[count * 5 / 4 + uv] = (unsigned(input[3]) + input[stride + 3] + 1) / 2;
        }
    }
    preview(frame);
}
void rgbaFrame(const Image& image, CameraFrame& frame) {
    const int width = image.width, height = image.height;
    require(width >= 2 && height >= 2 && width <= 1920 && height <= 1080 && !(width & 1) && !(height & 1)
        && image.rgba.size() == static_cast<size_t>(width) * height * 4, "Unsupported camera image size");
    frame.width = width; frame.height = height;
    frame.range = HIFACE_YUV_RANGE_FULL; frame.matrix = HIFACE_YUV_MATRIX_BT601;
    frame.rgba = image.rgba;
    const size_t count = static_cast<size_t>(width) * height;
    frame.i420.resize(count * 3 / 2);
    for (int y = 0; y < height; y += 2) for (int x = 0; x < width; x += 2) {
        double us = 0, vs = 0;
        for (int dy = 0; dy < 2; ++dy) for (int dx = 0; dx < 2; ++dx) {
            const size_t index = static_cast<size_t>(y + dy) * width + x + dx;
            const auto* pixel = image.rgba.data() + index * 4;
            const double r = pixel[0], g = pixel[1], b = pixel[2];
            frame.i420[index] = clip(.299 * r + .587 * g + .114 * b);
            us += -.168736 * r - .331264 * g + .5 * b + 128;
            vs += .5 * r - .418688 * g - .081312 * b + 128;
        }
        const size_t index = static_cast<size_t>(y / 2) * (width / 2) + x / 2;
        frame.i420[count + index] = clip(us / 4);
        frame.i420[count * 5 / 4 + index] = clip(vs / 4);
    }
}
}
