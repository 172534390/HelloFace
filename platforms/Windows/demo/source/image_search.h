#pragma once
#include "types.h"
#include <functional>
#include <optional>

namespace hiface_demo {
struct ImageSearchFace {
    HiFaceFaceV1 face{};
    HiFaceStatus status = HIFACE_STATUS_OK;
    Image preview; // owned aligned RGBA; empty for unsuccessful captures
    std::optional<hiface::SearchMatch> match;
    std::string label;
};
struct ImageSearchResult {
    std::vector<ImageSearchFace> faces;
    size_t matched = 0;
    bool empty_library = false;
    std::string summary;
};
// Demo policy matches Android: whole library, Top-1 / 0.80, no liveness.
// A stale request returns no result and cannot publish partial old identities.
std::optional<ImageSearchResult> resolveImageFaces(const Gallery& database,
    std::vector<hiface::ImageFaceCapture> captures, const std::function<bool()>& is_current = {});
std::optional<ImageSearchResult> searchPhoto(hiface::Client& client, const Gallery& database,
    const Image& image, const std::function<bool()>& is_current = {});
}
