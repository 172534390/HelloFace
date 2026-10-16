#pragma once
#include "gallery.h"
namespace hiface_linux_demo {
struct ImageFaceMatch {
    float x = 0, y = 0, width = 0, height = 0;
    HiFaceStatus capture_status = HIFACE_STATUS_OK, search_status = HIFACE_STATUS_OK;
    Image preview;
    std::vector<GalleryEntry> matches;
    std::string label;
    bool matched() const { return capture_status == HIFACE_STATUS_OK && search_status == HIFACE_STATUS_OK && !matches.empty(); }
};
struct ImageSearchReport {
    std::vector<ImageFaceMatch> faces;
    uint64_t library_count = 0;
    uint32_t matched = 0;
    bool cancelled = false;
};
// Android image search policy: at most ten faces, largest first; Top-1 over the
// whole library at the shared default identity threshold. No camera/liveness gates.
// Results own their pixels/strings, and never retain borrowed SDK pointers.
ImageSearchReport searchImage(Session& session, Database& database, const Image& image,
    const std::function<bool()>& cancelled = {});
}
