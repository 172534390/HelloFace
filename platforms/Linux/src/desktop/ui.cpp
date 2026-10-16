#include "ui.h"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>
namespace hiface_linux_demo {
namespace {
constexpr uint32_t background = 0xf5f5f7, surface = 0xffffff, ink = 0x242029, muted = 0x69636f, border = 0xe1e5eb;
constexpr uint32_t accent = 0x077a63, tint = 0xe6f4ef, danger = 0xb33d37, dark = 0x172233;
Rect fitImage(Rect frame, int width, int height) {
    if (width <= 0 || height <= 0) return {};
    const double scale = std::min(double(frame.w) / width, double(frame.h) / height);
    const int w = std::max(1, int(width * scale)), h = std::max(1, int(height * scale));
    return {frame.x + (frame.w - w) / 2, frame.y + (frame.h - h) / 2, w, h};
}
std::string number(double value, int decimals = 1) { std::ostringstream s; s << std::fixed << std::setprecision(decimals) << value; return s.str(); }
const char* modeName(char mode) {
    switch (mode) { case 'C': return "Capture"; case 'L': return "Liveness"; case 'V': return "Verify"; case 'E': return "Enroll"; case 'S': return "Search"; case 'P': return "Search paused"; default: return "Detection"; }
}
std::string friendly(const std::string& text) {
    if (text.find("IMAGE_NO_FACE") != std::string::npos) return "No face found. Choose a clear photo with a visible face.";
    if (text.find("IMAGE_FACE_TOO_SMALL") != std::string::npos) return "The face is too small. Choose a photo with a face at least 100 pixels wide and high.";
    if (text.find("ALREADY_EXISTS") != std::string::npos) return "This face ID already exists. Choose another ID or edit its entry in Face library.";
    return text;
}
}
DemoUi::DemoUi(Measure measure) : measure_(std::move(measure)) {
    if (!measure_) measure_ = [](const std::string& value, Font font) {
        int width = 0; for (unsigned char c : value) if ((c & 0xc0) != 0x80) width += c < 128 ? (font == Font::Title ? 12 : font == Font::Small ? 6 : 7) : (font == Font::Title ? 22 : 14); return width;
    };
}
void DemoUi::text(int x, int y, std::string value, uint32_t color, int max_width, Font font) {
    for (auto& c : value) if (static_cast<unsigned char>(c) < 32) c = ' ';
    if (max_width <= 0) return;
    if (measure_(value, font) > max_width) {
        if (measure_("...", font) > max_width) return;
        while (!value.empty() && measure_(value + "...", font) > max_width) {
            size_t end = value.size() - 1; while (end && (static_cast<unsigned char>(value[end]) & 0xc0) == 0x80) --end; value.resize(end);
        }
        value += "...";
    }
    scene_.text(x, y, value, color, font, measure_(value, font));
}
void DemoUi::wrap(int x, int y, const std::string& value, uint32_t color, int width, int max_lines, Font font) {
    if (width <= 0 || max_lines <= 0) return;
    std::istringstream words(value); std::string word, line; std::vector<std::string> lines;
    while (words >> word) {
        if (!line.empty() && measure_(line + ' ' + word, font) > width) { lines.push_back(std::move(line)); line.clear(); }
        // IDs and paths may have no spaces. Split only at UTF-8 code point boundaries.
        while (measure_(word, font) > width && int(lines.size()) <= max_lines) {
            size_t end = 0, next = 0;
            while (next < word.size()) {
                ++next; while (next < word.size() && (static_cast<unsigned char>(word[next]) & 0xc0) == 0x80) ++next;
                if (measure_(word.substr(0, next), font) > width) break;
                end = next;
            }
            if (!end) end = next;
            lines.push_back(word.substr(0, end)); word.erase(0, end);
        }
        if (int(lines.size()) > max_lines) break;
        if (!line.empty()) line += ' '; line += word;
    }
    if (!line.empty()) lines.push_back(std::move(line));
    const bool truncated = int(lines.size()) > max_lines;
    for (int i = 0; i < std::min(max_lines, int(lines.size())); ++i)
        text(x, y + i * 19, lines[i] + (truncated && i == max_lines - 1 ? "..." : ""), color, width, font);
}
void DemoUi::card(Rect r) {
    scene_.box({r.x, r.y + 2, r.w, r.h}, 0xe9ecf0, 12); scene_.box(r, border, 12); scene_.box({r.x + 1, r.y + 1, r.w - 2, r.h - 2}, surface, 11);
}
void DemoUi::button(Rect r, const std::string& label, const std::string& action, int style, bool enabled) {
    const bool hover = enabled && r.contains(pointer_x_, pointer_y_);
    const bool primary = style == 1, selected = style == 2, destructive = style == 3, navigation = style == 4;
    uint32_t fill = primary ? accent : selected ? tint : navigation ? 0x204438 : surface;
    uint32_t line = primary || navigation ? fill : selected ? accent : border;
    uint32_t color = primary || navigation ? 0xffffff : destructive ? danger : selected ? accent : ink;
    if (hover) fill = primary ? 0x05624f : navigation ? 0x2b574b : destructive ? 0xffefed : 0xedf5f1;
    if (!enabled) { fill = 0xf0f1f4; line = 0xe5e7ec; color = 0xa0a5af; }
    scene_.box(r, keyboard_focus_ == action ? accent : line, 8); scene_.box({r.x + 1, r.y + 1, r.w - 2, r.h - 2}, fill, 7);
    const int label_width = measure_(label, Font::Body);
    text(r.x + std::max(9, (r.w - label_width) / 2), r.y + r.h / 2 + 5, label, color, r.w - 18);
    const auto bounds = r.intersect(scene_.clip); if (bounds.w > 0 && bounds.h > 0) controls_.push_back({bounds, action, "", enabled});
}
void DemoUi::field(Rect r, const std::string& key, const std::string& label, const std::string& initial, bool enabled) {
    auto& value = values_.emplace(key, initial).first->second;
    text(r.x, r.y + 12, label, muted, r.w, Font::Small);
    const Rect input{r.x, r.y + 21, r.w, 34};
    scene_.box(input, invalid_field_ == key ? danger : focused_ == key ? accent : border, 7);
    scene_.box({input.x + 1, input.y + 1, input.w - 2, input.h - 2}, enabled ? surface : 0xf3f4f6, 6);
    auto shown = value; size_t shown_start = 0;
    if (focused_ == key) {
        auto position = std::min(cursor_, value.size()); shown.insert(position, "|");
        while (!shown.empty() && measure_(shown.substr(0, std::min(position + 1, shown.size())), Font::Body) > input.w - 22) {
            size_t end = 1; while (end < shown.size() && (static_cast<unsigned char>(shown[end]) & 0xc0) == 0x80) ++end; shown.erase(0, end); shown_start += end; position -= std::min(position, end);
        }
        while (!shown.empty() && measure_(shown, Font::Body) > input.w - 20) {
            size_t end = shown.size() - 1; while (end && (static_cast<unsigned char>(shown[end]) & 0xc0) == 0x80) --end; shown.resize(end);
        }
        if (select_all_) scene_.box({input.x + 5, input.y + 5, std::min(input.w - 10, measure_(shown, Font::Body) + 6), input.h - 10}, 0xc6e9dc, 3);
    }
    text(input.x + 10, input.y + 22, shown, enabled ? ink : muted, input.w - 20);
    const auto bounds = input.intersect(scene_.clip); if (bounds.w && bounds.h) controls_.push_back({bounds, "", key, enabled, shown_start});
}
void DemoUi::toggle(int x, int y, int width, const std::string& key, const std::string& label, bool enabled) {
    const bool on = values_[key] == "1";
    text(x, y + 19, label, enabled ? ink : 0x9da3ad, width - 64);
    const Rect r{x + width - 48, y, 44, 26}; scene_.box(r, enabled ? on ? accent : 0xd5dae1 : 0xe9ecf0, 13);
    scene_.box({r.x + (on ? 22 : 4), r.y + 4, 18, 18}, surface, 9);
    if (keyboard_focus_ == "toggle:" + key) scene_.box({x, y + 28, width, 2}, accent);
    controls_.push_back({{x, y, width, 30}, "toggle:" + key, "", enabled});
}
void DemoUi::empty(Rect r, const std::string& title, const std::string& hint) {
    const bool preview = r.h > 150;
    scene_.box(r, 0x101a29, 8);
    if (preview) {
        scene_.box({r.x + r.w / 2 - 24, r.y + r.h / 2 - 56, 48, 40}, 0x293d51, 10);
        scene_.box({r.x + r.w / 2 - 10, r.y + r.h / 2 - 46, 20, 20}, 0x172638, 10);
    }
    text(r.x + std::max(12, (r.w - measure_(title, Font::Body)) / 2), r.y + r.h / 2 + 12, title, 0xdce5ee, r.w - 24);
    text(r.x + std::max(12, (r.w - measure_(hint, Font::Small)) / 2), r.y + r.h / 2 + 38, hint, 0x90a2b7, r.w - 24, Font::Small);
}
void DemoUi::picture(Rect r, const Image* image, const CameraFrame* camera, bool mirror, const std::vector<Box>& boxes) {
    const int iw = image ? image->width : camera ? camera->width : 0, ih = image ? image->height : camera ? camera->height : 0;
    const auto* data = image ? &image->rgba : camera ? &camera->rgba : nullptr;
    if (!data || data->empty() || iw <= 0 || ih <= 0) { empty(r, "No image yet", "Choose a photo or capture a face"); return; }
    scene_.box(r, 0x101a29, 8);
    const double scale = std::min(double(r.w) / iw, double(r.h) / ih);
    const Rect image_rect = fitImage(r, iw, ih);
    scene_.image(image_rect, data->data(), iw, ih, mirror);
    const auto old_clip = scene_.clip; scene_.clip = scene_.clip.intersect(image_rect);
    for (const auto& box : boxes) {
        const int x = image_rect.x + int((mirror ? iw - box.x - box.width : box.x) * scale), y = image_rect.y + int(box.y * scale);
        const int bw = std::max(1, int(box.width * scale)), bh = std::max(1, int(box.height * scale)), stroke = box.selected ? 4 : 2; const auto color = box.matched ? 0x79e3c0u : 0xf3c981u;
        scene_.box({x, y, bw, stroke}, color); scene_.box({x, y + bh - stroke, bw, stroke}, color); scene_.box({x, y, stroke, bh}, color); scene_.box({x + bw - stroke, y, stroke, bh}, color);
        const int label_width = std::min(image_rect.w, std::min(std::max(bw, 160), measure_(box.label, Font::Small) + 16));
        const int label_x = std::clamp(x, image_rect.x, image_rect.x + image_rect.w - label_width), label_y = std::max(image_rect.y, y - 25);
        scene_.box({label_x, label_y, label_width, 23}, dark, 4);
        text(label_x + 7, label_y + 16, box.label, color, label_width - 14, Font::Small);
        for (const auto& point : box.points) if (point.x >= 0 && point.x < iw && point.y >= 0 && point.y < ih)
            scene_.box({image_rect.x + int((mirror ? iw - point.x : point.x) * scale), image_rect.y + int(point.y * scale), 2, 2}, 0x8edcfa);
    }
    scene_.clip = old_clip;
}
void DemoUi::camera(const ViewState& s, Rect b) {
    card({b.x, b.y, b.w, 86}); field({b.x + 16, b.y + 13, 190, 56}, "device", "Camera device", s.device, !s.busy);
    button({b.x + 220, b.y + 34, 98, 34}, "Connect", "connect", 1, content_enabled_);
    button({b.x + 328, b.y + 34, 98, 34}, "Next camera", "next", 0, content_enabled_);
    scene_.box({b.x + 448, b.y + 31, 8, 8}, s.connected ? accent : 0xa0a5af, 4);
    text(b.x + 466, b.y + 39, s.connected ? "Camera connected" : "Camera disconnected", s.connected ? accent : muted, b.w - 482);
    text(b.x + 448, b.y + 61, s.camera, muted, b.w - 464, Font::Small);
    const int top = b.y + 104, h = b.h - 104, side = 246, pw = b.w - side - 16;
    card({b.x, top, pw, h}); text(b.x + 16, top + 28, "Live preview", ink, pw - 126);
    button({b.x + pw - 102, top + 12, 86, 30}, s.settings.mirror ? "Mirror on" : "Mirror off", "mirror", s.settings.mirror ? 2 : 0, content_enabled_);
    const Rect preview{b.x + 12, top + 52, pw - 24, h - 146};
    if (s.frame && s.connected) picture(preview, nullptr, s.frame.get(), s.settings.mirror, s.boxes);
    else empty(preview, "Connect a camera to begin", "VMware: connect the USB camera to Ubuntu");
    const int footer = top + h - 80;
    if (s.aligned_preview) picture({b.x + 16, footer + 8, 58, 58}, s.aligned_preview.get(), nullptr, false, {});
    else scene_.box({b.x + 16, footer + 8, 58, 58}, 0xedf1f5, 8);
    text(b.x + 86, footer + 23, "Verification reference", muted, pw - 192, Font::Small);
    text(b.x + 86, footer + 46, s.reference.empty() ? "No reference selected" : s.reference, s.reference.empty() ? muted : accent, pw - 194);
    button({b.x + pw - 88, footer + 20, 72, 32}, "Clear", "clear-reference", 0, content_enabled_ && !s.reference.empty());
    const int sx = b.x + pw + 16; card({sx, top, side, h}); text(sx + 16, top + 28, "Workflow", ink, side - 32);
    text(sx + 16, top + 51, "Choose a task, then start", muted, side - 32, Font::Small);
    const char* labels[] = {"Detection", "Capture", "Liveness", "Verify", "Enroll", "Search"};
    for (int i = 0; i < 6; ++i) {
        const std::string task(1, "DCLVES"[i]);
        button({sx + 16 + (i % 2) * 111, top + 68 + (i / 2) * 42, 103, 34}, labels[i], "task:" + task, selected_task_ == task ? 2 : 0, content_enabled_);
    }
    const bool has_ref = selected_task_ != "V" || !s.reference.empty();
    button({sx + 16, top + 208, side - 32, 38}, "Start " + std::string(modeName(selected_task_[0])), "start-task", 1, content_enabled_ && s.connected && has_ref);
    button({sx + 16, top + 256, side - 32, 34}, s.mode == 'P' ? "Resume search" : "Pause search", "pause", 0, content_enabled_ && (s.mode == 'S' || s.mode == 'P'));
    button({sx + 16, top + 300, side - 32, 34}, "Cancel workflow", "mode:X", 0, content_enabled_ && s.mode != 'D');
    if (h > 394) {
        if (!has_ref) { if (h > 430) text(sx + 16, top + 352, "Verification needs a reference", muted, side - 32, Font::Small); button({sx + 16, top + h - 48, side - 32, 32}, "Choose reference", "page:gallery", 0, content_enabled_); }
        else wrap(sx + 16, top + 367, "Running: " + std::string(modeName(s.mode)) + ". Actions and search options are in Settings.", muted, side - 32, 3, Font::Small);
    }
}
void DemoUi::photos(const ViewState& s, Rect b) {
    const int half = (b.w - 48) / 2;
    card({b.x, b.y, b.w, 144});
    field({b.x + 16, b.y + 14, half - 82, 56}, "photo-a", "Photo A", "", content_enabled_);
    button({b.x + 16 + half - 72, b.y + 35, 72, 34}, "Browse", "browse:photo-a", 0, content_enabled_);
    field({b.x + 32 + half, b.y + 14, half - 82, 56}, "photo-b", "Photo B for comparison", "", content_enabled_);
    button({b.x + b.w - 88, b.y + 35, 72, 34}, "Browse", "browse:photo-b", 0, content_enabled_);
    const char* labels[] = {"Detect", "Compare A / B", "Set reference", "Enroll photo", "Search"};
    const char* actions[] = {"detect", "compare", "reference", "enroll", "search"}; const int bw = (b.w - 64) / 5;
    for (int i = 0; i < 5; ++i) button({b.x + 16 + i * (bw + 8), b.y + 92, bw, 36}, labels[i], std::string("image:") + actions[i], i == 1 ? 1 : 0,
        content_enabled_ && !values_["photo-a"].empty() && (i != 1 || !values_["photo-b"].empty()));
    const int y = b.y + 160, w = (b.w - 16) / 2, h = b.h - 160;
    for (int i = 0; i < 2; ++i) card({b.x + i * (w + 16), y, w, h});
    text(b.x + 16, y + 28, "Photo A", ink, w - 32);
    text(b.x + w + 32, y + 28, s.image_second ? "Photo B" : s.image_matches.empty() ? "Aligned face" : "Search results", ink, w - 32);
    const Rect left{b.x + 12, y + 48, w - 24, h - 60}, right{b.x + w + 28, y + 48, w - 24, h - 60};
    picture(left, s.image_first.get(), nullptr, false, s.boxes);
    auto rr = right; if (!s.image_matches.empty()) rr.h = std::max(64, rr.h - 142);
    picture(rr, s.image_second ? s.image_second.get() : s.aligned_preview.get(), nullptr, false, {});
    for (size_t i = 0; i < s.image_matches.size(); ++i) {
        const int row = rr.y + rr.h + 15 + int(i) * 24; const auto& match = s.image_matches[i];
        text(right.x + 6, row + 11, match.id, ink, right.w - 88, Font::Small); text(right.x + right.w - 68, row + 11, number(match.similarity, 3), accent, 64, Font::Small);
    }
}
void DemoUi::imageSearch(const ViewState& s, Rect b) {
    if (image_search_generation_ != s.photo_search_generation) { image_search_generation_ = s.photo_search_generation; selected_image_face_ = image_face_offset_ = 0; }
    const bool picker_enabled = s.ready && !s.importing && (!s.busy || s.photo_search_busy);
    card({b.x, b.y, b.w, 116});
    field({b.x + 16, b.y + 14, b.w - 326, 56}, "search-path", "Photo to search (Enter to search)", "", picker_enabled);
    button({b.x + b.w - 298, b.y + 35, 78, 34}, "Browse", "browse:search-path", values_["search-path"].empty() ? 1 : 0, picker_enabled);
    button({b.x + b.w - 210, b.y + 35, 98, 34}, "Search", "image-search", 1, picker_enabled && !values_["search-path"].empty());
    button({b.x + b.w - 102, b.y + 35, 86, 34}, "Cancel", "cancel-image-search", 0, s.photo_search_busy);
    text(b.x + 16, b.y + 96, "All groups | Top-1 " + number(HIFACE_DEFAULT_VERIFICATION_THRESHOLD, 2) + " | up to 10 faces | no liveness", muted, b.w - 188, Font::Small);
    button({b.x + b.w - 154, b.y + 78, 138, 26}, "Face library", "page:gallery", 0, content_enabled_);
    const int y = b.y + 132, h = b.h - 132, side = 260, w = b.w - side - 16, sx = b.x + w + 16;
    const int count = s.photo_search ? int(s.photo_search->faces.size()) : 0;
    image_face_rows_ = std::max(2, std::min(10, (h - 94) / 76));
    image_result_area_ = {sx, y + 48, side, h - 48};
    selected_image_face_ = std::clamp(selected_image_face_, 0, std::max(0, count - 1));
    image_face_offset_ = (selected_image_face_ / image_face_rows_) * image_face_rows_;
    card({b.x, y, w, h}); text(b.x + 16, y + 28, "Selected photo", ink, w - 138);
    if (s.search_photo) text(b.x + w - 116, y + 28, std::to_string(s.search_photo->width) + " x " + std::to_string(s.search_photo->height), muted, 100, Font::Small);
    const Rect preview{b.x + 12, y + 48, w - 24, h - (count ? 156 : 60)};
    const Rect fitted = s.search_photo ? fitImage(preview, s.search_photo->width, s.search_photo->height) : Rect{};
    std::vector<Box> boxes;
    if (count) for (int i = 0; i < count; ++i) {
        const auto& f = s.photo_search->faces[i];
        boxes.push_back({f.x, f.y, f.width, f.height, "#" + std::to_string(i+1), f.matched(), {}, i == selected_image_face_});
        if (s.search_photo) {
            const double scale = std::min(double(preview.w)/s.search_photo->width, double(preview.h)/s.search_photo->height);
            image_face_hits_.push_back({Rect{fitted.x + int(f.x*scale), fitted.y + int(f.y*scale), std::max(1,int(f.width*scale)), std::max(1,int(f.height*scale))}.intersect(fitted), i});
        }
    }
    if (s.search_photo) picture(preview, s.search_photo.get(), nullptr, false, boxes);
    else empty(preview, s.photo_search_busy ? "Searching photo..." : "Choose a photo to search", "Browse to select a local photo. No camera needed.");
    if (count) {
        controls_.push_back({preview, "image-photo", "", content_enabled_});
        const auto& selected = s.photo_search->faces[selected_image_face_];
        const bool technical_error = (selected.capture_status != HIFACE_STATUS_OK && selected.capture_status != HIFACE_STATUS_IMAGE_FACE_TOO_SMALL) || selected.search_status != HIFACE_STATUS_OK;
        const uint32_t tone = selected.matched() ? accent : technical_error ? danger : 0x9b6b16;
        const int top = y + h - 92;
        scene_.box({b.x+16, top-5, w-32, 1}, border);
        const std::string selected_status = selected.matched() ? "Matched" : technical_error ? "Error" : selected.capture_status == HIFACE_STATUS_IMAGE_FACE_TOO_SMALL ? "Small face" : "No match";
        text(b.x+16, top+14, "Face " + std::to_string(selected_image_face_+1) + " of " + std::to_string(count) + "  /  " + selected_status, tone, w-32, Font::Small);
        wrap(b.x+16, top+38, selected.label, ink, w-32, 2);
        const auto metadata = selected.matched() ? "Group: " + (selected.matches.front().group.empty() ? "-" : selected.matches.front().group) + "   Tag: " + (selected.matches.front().tag.empty() ? "-" : selected.matches.front().tag) : "Click a face or use arrow keys to change selection.";
        text(b.x+16, top+77, metadata, muted, w-32, Font::Small);
    }
    card({sx, y, side, h});
    text(sx + 16, y + 28, "Faces: " + std::to_string(count), ink, 106);
    text(sx + 128, y + 28, "Matched: " + std::to_string(s.photo_search ? s.photo_search->matched : 0), accent, side - 144, Font::Small);
    if (!count) {
        wrap(sx + 16, y + 75, s.photo_search_busy ? "Detecting faces and searching the library. You can choose another photo or cancel." : s.photo_search ? "No faces found. Try a clear, upright photo." : "Choose one photo to search every detected face.", muted, side - 32, 5);
        if (s.photo_search && s.photo_search->library_count == 0) wrap(sx + 16, y + 186, "Your library is empty. Open Face library to enroll or import features.", 0x9b6b16, side - 32, 4, Font::Small);
        return;
    }
    for (int i = image_face_offset_; i < std::min(count, image_face_offset_ + image_face_rows_); ++i) {
        const auto& f = s.photo_search->faces[i]; const Rect row{sx + 12, y + 48 + (i-image_face_offset_) * 76, side - 24, 70};
        const bool selected = i == selected_image_face_;
        const bool failed = (f.capture_status != HIFACE_STATUS_OK && f.capture_status != HIFACE_STATUS_IMAGE_FACE_TOO_SMALL) || f.search_status != HIFACE_STATUS_OK;
        const uint32_t tone = f.matched() ? accent : failed ? danger : 0x9b6b16;
        scene_.box(row, selected || keyboard_focus_ == "image-face:" + std::to_string(i) ? accent : border, 8);
        scene_.box({row.x+1, row.y+1, row.w-2, row.h-2}, selected ? tint : row.contains(pointer_x_,pointer_y_) ? 0xf2f7f5 : surface, 7);
        if (!f.preview.rgba.empty()) picture({row.x+7, row.y+7, 56, 56}, &f.preview, nullptr, false, {});
        else { scene_.box({row.x+7, row.y+7, 56, 56}, 0xe9edf2, 6); text(row.x+29, row.y+42, "?", muted, 22); }
        text(row.x+73, row.y+20, "Face " + std::to_string(i+1), muted, 64, Font::Small);
        scene_.box({row.x+row.w-82,row.y+7,74,18}, f.matched() ? tint : failed ? 0xffeeeb : 0xfff4dc, 4);
        text(row.x+row.w-77,row.y+20,f.matched() ? "Matched" : failed ? "Error" : f.capture_status==HIFACE_STATUS_IMAGE_FACE_TOO_SMALL ? "Small face" : "No match",tone,64,Font::Small);
        wrap(row.x+73, row.y+40, f.label, tone, row.w-83, 2, Font::Small);
        controls_.push_back({row, "image-face:" + std::to_string(i), "", content_enabled_});
    }
    button({sx+12, y+h-36, 76, 26}, "Previous", "image-faces-prev:" + std::to_string(image_face_rows_), 0, content_enabled_ && image_face_offset_ > 0);
    text(sx+98, y+h-18, std::to_string(image_face_offset_+1) + "-" + std::to_string(std::min(count,image_face_offset_+image_face_rows_)) + " / " + std::to_string(count), muted, 68, Font::Small);
    button({sx+side-80, y+h-36, 68, 26}, "Next", "image-faces-next:" + std::to_string(image_face_rows_), 0, content_enabled_ && image_face_offset_+image_face_rows_ < count);
}
void DemoUi::library(const ViewState& s, Rect b) {
    if (s.importing) library_tab_ = "transfer";
    button({b.x, b.y, 150, 34}, "People", "library:people", library_tab_ == "people" ? 2 : 0, !s.importing && !s.busy);
    button({b.x + 160, b.y, 168, 34}, "Import / export", "library:transfer", library_tab_ == "transfer" ? 2 : 0, !s.importing && !s.busy);
    text(b.x + 346, b.y + 23, std::to_string(s.count) + " saved faces", muted, b.w - 346, Font::Small);
    b.y += 50; b.h -= 50;
    if (library_tab_ == "transfer") {
        card({b.x, b.y, b.w, b.h}); text(b.x + 20, b.y + 32, "Move your face library", ink, b.w - 40);
        wrap(b.x + 20, b.y + 57, "Import Android-compatible HF1 JSON or a local image folder. Export keeps the original HF1 features.", muted, b.w - 40, 2, Font::Small);
        field({b.x + 20, b.y + 94, b.w - 132, 56}, "exchange-path", "JSON file or image directory", s.data_directory + "/features.json", !s.importing && !s.busy);
        button({b.x + b.w - 96, b.y + 115, 76, 34}, "Browse", "browse:exchange-path", 0, content_enabled_);
        const bool path = !values_["exchange-path"].empty(); const int bw = (b.w - 56) / 3;
        button({b.x + 20, b.y + 172, bw, 36}, "Import HF1 JSON", "import-features", 1, content_enabled_ && path);
        button({b.x + 28 + bw, b.y + 172, bw, 36}, "Import image folder", "import-images", 0, content_enabled_ && path);
        button({b.x + 36 + bw * 2, b.y + 172, bw, 36}, "Export JSON", "export", 0, content_enabled_ && path);
        const int py = b.y + 240; text(b.x + 20, py, s.importing ? "Import in progress" : "Import progress", ink, b.w - 40);
        const double fraction = s.import_total ? std::min(1.0, double(s.import_processed) / s.import_total) : 0;
        scene_.box({b.x + 20, py + 18, b.w - 40, 8}, 0xe9eef1, 4); if (fraction > 0) scene_.box({b.x + 20, py + 18, int((b.w - 40) * fraction), 8}, accent, 4);
        text(b.x + 20, py + 56, std::to_string(s.import_processed) + " / " + std::to_string(s.import_total) + " processed   |   Saved " + std::to_string(s.import_saved) + "   Skipped " + std::to_string(s.import_failed), muted, b.w - 40, Font::Small);
        button({b.x + 20, py + 76, 154, 34}, "Stop import", "cancel-import", 3, s.importing && !s.busy);
        if (!s.demo_features.empty()) button({b.x + 190, py + 76, 210, 34}, "Import sample (10k)", "import-demo", 0, content_enabled_);
        if (b.h > 408) wrap(b.x + 20, b.y + b.h - 42, "Imports add or replace matching IDs. Stopping preserves completed batches. Export uses the last loaded group/tag filter.", muted, b.w - 40, 2, Font::Small);
        return;
    }
    card({b.x, b.y, b.w, 84}); const int fw = (b.w - 144) / 3;
    field({b.x + 16, b.y + 10, fw, 56}, "query-id", "Exact face ID", "", content_enabled_);
    field({b.x + 24 + fw, b.y + 10, fw, 56}, "query-group", "Group (empty = all)", "", content_enabled_);
    field({b.x + 32 + fw * 2, b.y + 10, fw, 56}, "query-tag", "Tag (empty = all)", "", content_enabled_);
    button({b.x + b.w - 96, b.y + 31, 80, 34}, "Find", "gallery-load", 1, content_enabled_);
    const int top = b.y + 100, side = 224, w = b.w - side - 16, h = b.h - 100;
    card({b.x, top, w, h}); text(b.x + 16, top + 27, "Faces", ink, w - 128); text(b.x + w - 104, top + 27, std::to_string(s.filtered_count) + " in scope", muted, 92, Font::Small);
    const int idw = (w - 32) / 2, gw = (w - 32) / 4;
    scene_.box({b.x + 1, top + 42, w - 2, 28}, 0xf4f6f8);
    text(b.x + 16, top + 61, "Face ID", muted, idw - 8, Font::Small); text(b.x + 16 + idw, top + 61, "Group", muted, gw - 8, Font::Small); text(b.x + 16 + idw + gw, top + 61, "Tag", muted, gw - 8, Font::Small);
    const int rh = std::min(28, std::max(18, (h - 124) / 10));
    for (size_t i = 0; i < s.gallery.size(); ++i) {
        const auto& row = s.gallery[i]; const Rect rr{b.x + 8, top + 74 + int(i) * rh, w - 16, rh};
        const bool selected = values_["selected-id"] == row.id;
        scene_.box(rr, selected ? tint : rr.contains(pointer_x_, pointer_y_) ? 0xf1f6f3 : i % 2 ? 0xf9fafb : surface, 4);
        text(b.x + 16, rr.y + rh / 2 + 5, row.id, selected ? accent : ink, idw - 8, Font::Small);
        text(b.x + 16 + idw, rr.y + rh / 2 + 5, row.group, muted, gw - 8, Font::Small); text(b.x + 16 + idw + gw, rr.y + rh / 2 + 5, row.tag, muted, gw - 8, Font::Small);
        if (keyboard_focus_ == "gallery-row:" + std::to_string(i)) scene_.box({rr.x, rr.y + rr.h - 2, rr.w, 2}, accent);
        controls_.push_back({rr, "gallery-row:" + std::to_string(i), "", content_enabled_});
    }
    if (s.gallery.empty()) { text(b.x + 24, top + 119, "No faces to display", ink, w - 48); wrap(b.x + 24, top + 147, "Find a face ID, change the filters, or import a library.", muted, w - 48, 2, Font::Small); }
    button({b.x + 12, top + h - 44, 86, 30}, "Previous", "gallery-prev", 0, content_enabled_ && values_["query-id"].empty() && s.gallery_offset > 0);
    button({b.x + w - 86, top + h - 44, 74, 30}, "Next", "gallery-next", 0, content_enabled_ && values_["query-id"].empty() && s.gallery_offset + 10 < s.filtered_count);
    text(b.x + 108, top + h - 23, "Offset " + std::to_string(s.gallery_offset), muted, w - 204, Font::Small);
    const int sx = b.x + w + 16; card({sx, top, side, h}); text(sx + 16, top + 28, "Selected face", ink, side - 32);
    const bool selected = !values_["selected-id"].empty(); text(sx + 16, top + 55, selected ? values_["selected-id"] : "Select a row to edit", selected ? accent : muted, side - 32, Font::Small);
    field({sx + 16, top + 72, side - 32, 56}, "selected-group", "Group", "", content_enabled_ && selected);
    field({sx + 16, top + 136, side - 32, 56}, "selected-tag", "Tag", "", content_enabled_ && selected);
    button({sx + 16, top + 208, side - 32, 34}, "Save metadata", "metadata", 1, content_enabled_ && selected);
    button({sx + 16, top + 252, side - 32, 34}, "Use as reference", "reference", 0, content_enabled_ && selected);
    button({sx + 16, top + h - 48, side - 32, 32}, "Delete face", "delete", 3, content_enabled_ && selected);
}
void DemoUi::resetDraft(const DemoSettings& settings) { for (const auto& f : settingsFields(settings)) values_["setting:" + f.key] = f.value; invalid_field_.clear(); }
bool DemoUi::settingsDirty(const ViewState& state) const { for (const auto& f : settingsFields(state.settings)) { const auto it = values_.find("setting:" + f.key); if (it != values_.end() && it->second != f.value) return true; } return false; }
void DemoUi::settings(const ViewState& s, Rect b) {
    const char* tabs[] = {"detection", "flow", "search", "camera"}; const char* labels[] = {"Detection", "Liveness / verify", "Search", "Camera"}; const int tw = (b.w - 24) / 4;
    for (int i = 0; i < 4; ++i) button({b.x + i * (tw + 8), b.y, tw, 34}, labels[i], std::string("settings-tab:") + tabs[i], settings_tab_ == tabs[i] ? 2 : 0, !s.busy);
    const Rect r{b.x, b.y + 50, b.w, b.h - 112}; card(r); const int x = r.x + 20, y = r.y + 22, w = r.w - 40, half = (w - 20) / 2;
    auto f = [&](const char* key, const char* label, int px, int py, int width, bool enabled = true) { field({px, py, width, 56}, std::string("setting:") + key, label, "", !s.busy && enabled); };
    auto t = [&](const char* key, const char* label, int py, bool enabled = true) { toggle(x, py, w, std::string("setting:") + key, label, !s.busy && enabled); };
    if (settings_tab_ == "detection") {
        text(x, y + 12, "Face detection", ink, w); wrap(x, y + 39, "Choose how many faces to return and whether to show detailed landmarks and pose.", muted, w, 2, Font::Small);
        f("detection_faces", "Maximum faces (1..10)", x, y + 78, half); t("detailed", "468 landmarks and head pose", y + 170);
        wrap(x, y + 224, "Detailed analysis uses more CPU. Capture and liveness automatically prepare the capabilities they need.", muted, w, 3, Font::Small);
    } else if (settings_tab_ == "flow") {
        text(x, y + 12, "Actions and verification", ink, w); text(x, y + 39, "No actions = silent liveness. Up to four steps; repeats are allowed.", muted, w, Font::Small);
        const int aw = (w - 24) / 4; const char* actions[] = {"blink", "nod", "shake", "mouth"}; const char* names[] = {"Blink", "Nod head", "Shake head", "Open mouth"};
        const auto& sequence = values_["setting:actions"]; const int count = sequence.empty() ? 0 : 1 + std::count(sequence.begin(), sequence.end(), ',');
        for (int i = 0; i < 4; ++i) button({x + i * (aw + 8), y + 58, aw, 32}, names[i], std::string("add-action:") + actions[i], 0, !s.busy && count < 4);
        text(x, y + 120, sequence.empty() ? "Sequence: silent liveness" : "Sequence: " + sequence, sequence.empty() ? muted : accent, w - 144);
        button({x + w - 130, y + 101, 60, 30}, "Undo", "actions-undo", 0, !s.busy && count > 0); button({x + w - 62, y + 101, 62, 30}, "Clear", "actions-clear", 0, !s.busy && count > 0);
        t("verification_liveness", "Silent liveness during verification", y + 156); t("allow_multiple", "Allow additional faces in a flow", y + 202);
        f("verification_threshold", "Verification threshold (0..1)", x, y + 252, half);
    } else if (settings_tab_ == "search") {
        const bool multi = values_["setting:search_multi"] == "1";
        button({x, y, half, 34}, "1:N Single face", "search-single", multi ? 0 : 2, !s.busy); button({x + half + 20, y, half, 34}, "M:N Multiple faces", "search-multi", multi ? 2 : 0, !s.busy);
        t("search_liveness", multi ? "Liveness is unavailable in M:N" : "Silent liveness during search", y + 50, !multi);
        const int third = (w - 24) / 3;
        f("search_threshold", "Threshold (0.75..0.85)", x, y + 96, third); f("search_faces", "M:N faces (1..5)", x + third + 12, y + 96, third, multi); f("search_timeout", "Round timeout ms (1000..3000)", x + third * 2 + 24, y + 96, third);
        f("group", "Group (empty = all)", x, y + 172, half); f("tag", "Tag (empty = all)", x + half + 20, y + 172, half);
        f("search_interval", "Success interval ms (0..3000)", x, y + 248, half);
        wrap(x + half + 20, y + 275, "Group and tag match exactly. A liveness rejection always overrides an identity match.", muted, half, 3, Font::Small);
    } else {
        text(x, y + 12, "Camera preferences", ink, w); text(x, y + 39, "Reconnect the camera after applying video settings.", muted, w, Font::Small);
        for (int i = 0; i < 3; ++i) button({x + i * 158, y + 60, 148, 32}, i == 0 ? "640 x 480" : i == 1 ? "1280 x 720" : "1920 x 1080", "resolution:" + std::to_string(i), 0, !s.busy);
        const int third = (w - 24) / 3; f("camera_width", "Width (even)", x, y + 114, third); f("camera_height", "Height (even)", x + third + 12, y + 114, third); f("camera_fps", "Requested FPS (1..60)", x + third * 2 + 24, y + 114, third);
        t("camera_mjpeg", "Prefer MJPEG over YUYV", y + 204); t("mirror", "Mirror preview", y + 254);
    }
    button({b.x, b.y + b.h - 44, 170, 38}, settings_pending_ ? "Applying..." : "Apply settings", "settings-apply", 1, content_enabled_ && !settings_pending_ && settingsDirty(s));
    button({b.x + 182, b.y + b.h - 44, 154, 38}, "Reset draft", "settings-defaults", 0, !s.busy);
    text(b.x + 354, b.y + b.h - 20, settingsDirty(s) ? "Unsaved changes" : "Settings are saved", settingsDirty(s) ? 0x9b6b16 : muted, b.w - 354, Font::Small);
}
void DemoUi::emit(std::string type, std::vector<std::string> values) { commands_.push_back({std::move(type), std::move(values), {}}); notice_.clear(); invalid_field_.clear(); }
std::vector<Command> DemoUi::takeCommands() { std::vector<Command> commands; commands.swap(commands_); return commands; }
void DemoUi::beginOverlay() {
    overlay_return_focus_ = keyboard_focus_; focused_.clear(); select_all_ = false; notice_.clear(); invalid_field_.clear();
}
void DemoUi::dismissOverlay() {
    confirming_ = browsing_ = enrolling_ = false; notice_.clear(); invalid_field_.clear(); browser_error_.clear(); select_all_ = false;
    keyboard_focus_ = overlay_return_focus_; focused_ = keyboard_focus_.rfind("field:", 0) == 0 ? keyboard_focus_.substr(6) : ""; cursor_ = values_[focused_].size();
}
void DemoUi::confirm(Command pending, const std::string& title, const std::string& message) {
    beginOverlay(); pending_ = std::move(pending); confirm_title_ = title; confirm_message_ = message; confirming_ = true; keyboard_focus_ = "cancel";
}
void DemoUi::selectImageFace(int index, const ViewState& state) {
    if (state.busy || state.importing || !state.photo_search || state.photo_search->faces.empty()) return;
    selected_image_face_ = std::clamp(index, 0, int(state.photo_search->faces.size()) - 1);
    image_face_offset_ = selected_image_face_ / image_face_rows_ * image_face_rows_;
    focused_.clear(); keyboard_focus_ = "image-face:" + std::to_string(selected_image_face_); select_all_ = false;
}
void DemoUi::scroll(int steps, int x, int y, const ViewState& state) {
    if (page_ == "image-search" && !confirming_ && !browsing_ && !enrolling_ && image_result_area_.contains(x,y)) selectImageFace(selected_image_face_ + steps, state);
}
void DemoUi::submitField(const ViewState& state) {
    if (browsing_) { handle("browser-go", state); return; }
    if (!state.ready || state.importing || (state.busy && !(page_ == "image-search" && state.photo_search_busy))) return;
    const auto field = focused_;
    std::string action;
    if (enrolling_) action = "enrollment-start";
    else if (field == "search-path") action = "image-search";
    else if (field == "device") action = "connect";
    else if (field.rfind("query-",0) == 0) action = "gallery-load";
    else if (field == "selected-group" || field == "selected-tag") { if (!values_["selected-id"].empty()) action = "metadata"; }
    else if (field.rfind("setting:",0) == 0 && !settings_pending_ && settingsDirty(state)) action = "settings-apply";
    else if (field == "photo-a" && !values_["photo-a"].empty()) action = "image:detect";
    else if (field == "photo-b" && !values_["photo-a"].empty() && !values_["photo-b"].empty()) action = "image:compare";
    if (action.empty()) { focused_.clear(); return; }
    // Use current field values: typing and Enter can arrive before the next paint.
    focused_.clear(); keyboard_focus_ = action; handle(action, state);
    if (!invalid_field_.empty()) { focused_ = invalid_field_; keyboard_focus_ = "field:" + focused_; cursor_ = values_[focused_].size(); }
}
void DemoUi::page(const std::string& next, const ViewState& state, bool force) {
    if (next == page_) return;
    if (state.importing || state.busy) { notice_ = "Finish or stop the current operation before changing pages."; return; }
    if (!force && next != page_ && page_ == "settings" && settingsDirty(state)) {
        confirm({"ui-page", {next}, {}}, "Discard changes?", "Your settings have not been applied. Discard the draft and leave this page?"); return;
    }
    page_ = next; focused_.clear(); keyboard_focus_.clear(); select_all_ = false; emit("page", {page_});
    if (next == "settings") resetDraft(state.settings);
}
void DemoUi::requestClose(const ViewState& state) {
    if (confirming_ || browsing_ || enrolling_) dismissOverlay();
    if (state.importing) confirm({"ui-quit", {}, {}}, "Stop import and exit?", "Completed batches are already saved. The current batch will finish before the app closes.");
    else if (page_ == "settings" && settingsDirty(state)) confirm({"ui-quit", {}, {}}, "Discard draft and exit?", "Your settings have not been applied. Saved face data will be kept.");
    else running_ = false;
}
void DemoUi::readDirectory() {
    browser_entries_.clear(); browser_offset_ = 0; browser_error_.clear(); std::error_code error;
    for (std::filesystem::directory_iterator it(browser_directory_, error), end; !error && it != end; it.increment(error)) {
        auto extension = it->path().extension().string(); std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
        std::error_code status;
        if (it->is_directory(status) || browser_target_ == "exchange-path" && extension == ".json" || browser_target_ != "exchange-path"
            && (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".bmp" || extension == ".ppm")) browser_entries_.push_back(*it);
    }
    if (error) { browser_entries_.clear(); browser_error_ = "Cannot read folder: " + error.message(); }
    std::sort(browser_entries_.begin(), browser_entries_.end(), [](const auto& a, const auto& b) { std::error_code ea, eb; const bool ad = a.is_directory(ea), bd = b.is_directory(eb); return ad != bd ? ad : a.path().filename().string() < b.path().filename().string(); });
    values_["browser-path"] = browser_directory_;
}
void DemoUi::browse(const std::string& target) {
    beginOverlay(); browser_target_ = target; browsing_ = true; const std::filesystem::path value = values_[target]; std::error_code error;
    if (std::filesystem::is_directory(value,error)) browser_directory_ = value.string();
    else if (value.has_parent_path() && std::filesystem::is_directory(value.parent_path(),error)) browser_directory_ = value.parent_path().string();
    else { const char* home_dir = std::getenv("HOME"); browser_directory_ = home_dir ? home_dir : "/"; }
    readDirectory(); focused_ = "browser-path"; keyboard_focus_ = "field:browser-path"; cursor_ = values_[focused_].size(); select_all_ = true;
}
void DemoUi::handle(const std::string& action, const ViewState& state) {
    if (action.rfind("image-face:", 0) == 0) { selectImageFace(std::stoi(action.substr(11)), state); return; }
    if (action.rfind("image-faces-prev:", 0) == 0) { selectImageFace(std::max(0, image_face_offset_ - image_face_rows_), state); return; }
    if (action.rfind("image-faces-next:", 0) == 0) { selectImageFace(image_face_offset_ + image_face_rows_, state); return; }
    if (action == "image-photo") {
        int selected = -1; int64_t area = INT64_MAX;
        for (const auto& hit : image_face_hits_) if (hit.first.contains(pointer_x_,pointer_y_) && int64_t(hit.first.w)*hit.first.h < area) { area = int64_t(hit.first.w)*hit.first.h; selected = hit.second; }
        if (selected >= 0) selectImageFace(selected,state); return;
    }
    if (action == "quit") { requestClose(state); return; }
    if (action == "cancel") { dismissOverlay(); return; }
    if (action == "confirm") {
        const auto command = pending_; dismissOverlay();
        if (command.type == "ui-quit") running_ = false;
        else if (command.type == "ui-page") page(command.values.at(0), state, true);
        else { commands_.push_back(command); notice_.clear(); }
        return;
    }
    if (action.rfind("page:", 0) == 0) { page(action.substr(5), state); return; }
    if (action.rfind("browse:", 0) == 0) { browse(action.substr(7)); return; }
    if (action == "browser-go") { browser_directory_ = values_["browser-path"]; readDirectory(); return; }
    if (action == "browser-up") { browser_directory_ = std::filesystem::path(browser_directory_).parent_path().string(); if (browser_directory_.empty()) browser_directory_ = "/"; readDirectory(); return; }
    if (action == "browser-prev") { browser_offset_ = std::max(0, browser_offset_ - 8); return; }
    if (action == "browser-next") { if (browser_offset_ + 8 < int(browser_entries_.size())) browser_offset_ += 8; return; }
    if (action == "browser-directory") { values_[browser_target_] = browser_directory_; dismissOverlay(); return; }
    if (action.rfind("browser-row:", 0) == 0) {
        const auto& entry = browser_entries_.at(std::stoul(action.substr(12)));
        std::error_code error; const bool directory = entry.is_directory(error);
        if (error) { browser_entries_.clear(); browser_offset_ = 0; browser_error_ = "Cannot read file: " + error.message(); return; }
        if (directory) { browser_directory_ = entry.path().string(); readDirectory(); }
        else { const bool search = browser_target_ == "search-path"; values_[browser_target_] = entry.path().string(); dismissOverlay(); if (search) emit("image-search", {values_["search-path"]}); }
        return;
    }
    if (action.rfind("library:", 0) == 0) { library_tab_ = action.substr(8); return; }
    if (action.rfind("settings-tab:", 0) == 0) { settings_tab_ = action.substr(13); return; }
    if (action.rfind("toggle:", 0) == 0) {
        auto& value = values_[action.substr(7)]; value = value == "1" ? "0" : "1"; invalid_field_.clear(); return;
    }
    if (action.rfind("task:", 0) == 0) { selected_task_ = action.substr(5); return; }
    if (action == "search-single" || action == "search-multi") {
        const bool multi = action == "search-multi"; values_["setting:search_multi"] = multi ? "1" : "0"; values_["setting:search_liveness"] = multi ? "0" : "1"; return;
    }
    if (action.rfind("resolution:", 0) == 0) {
        const int index = std::stoi(action.substr(11)); const char* widths[] = {"640", "1280", "1920"}; const char* heights[] = {"480", "720", "1080"}; values_["setting:camera_width"] = widths[index]; values_["setting:camera_height"] = heights[index]; return;
    }
    if (action.rfind("add-action:", 0) == 0) {
        auto& sequence = values_["setting:actions"]; if (!sequence.empty()) sequence += ','; sequence += action.substr(11); return;
    }
    if (action == "actions-clear") { values_["setting:actions"].clear(); return; }
    if (action == "actions-undo") { auto& sequence = values_["setting:actions"]; const auto comma = sequence.find_last_of(','); sequence = comma == std::string::npos ? "" : sequence.substr(0, comma); return; }
    if (action == "settings-defaults") { resetDraft(DemoSettings{}); return; }
    if (!state.ready) { notice_ = "The SDK is still loading. Please wait."; return; }
    if (action == "image-search" && values_["search-path"].empty()) { invalid_field_ = "search-path"; notice_ = "Choose a photo or enter its path before searching."; return; }
    if (action == "image-search" || action == "cancel-image-search") { emit(action, action == "image-search" ? std::vector<std::string>{values_["search-path"]} : std::vector<std::string>{}); return; }
    if (action == "image:search") { values_["search-path"] = values_["photo-a"]; page("image-search", state); emit("image-search", {values_["search-path"]}); return; }
    if (action == "start-task") { handle("mode:" + selected_task_, state); return; }
    if (action == "mode:E" || action == "image:enroll") {
        beginOverlay(); enrolling_ = true; photo_enrollment_ = action == "image:enroll"; focused_ = photo_enrollment_ ? "photo-id" : "enroll-id"; cursor_ = values_[focused_].size(); keyboard_focus_ = "field:" + focused_; return;
    }
    if (action == "enrollment-start") {
        if (photo_enrollment_ && values_["photo-id"].empty()) { invalid_field_ = focused_ = "photo-id"; keyboard_focus_ = "field:photo-id"; cursor_ = 0; notice_ = "Enter a face ID before enrolling this photo."; return; }
        if (photo_enrollment_) emit("image", {"enroll", values_["photo-a"], "", values_["photo-id"], values_["photo-group"], values_["photo-tag"]});
        else emit("enroll", {values_["enroll-id"], values_["enroll-group"], values_["enroll-tag"]});
        dismissOverlay(); return;
    }
    if (action.rfind("mode:", 0) == 0) emit("action", {action.substr(5)});
    else if (action == "connect") emit("connect", {values_["device"]});
    else if (action == "mirror" || action == "next" || action == "pause" || action == "cancel-import") emit(action);
    else if (action == "clear-reference") confirm({"clear-reference", {}, {}}, "Clear reference?", "Remove the saved verification reference. Face library entries and captured photos will be kept.");
    else if (action.rfind("image:", 0) == 0) emit("image", {action.substr(6), values_["photo-a"], values_["photo-b"], "", "", ""});
    else if (action == "gallery-load" || action == "gallery-prev" || action == "gallery-next") {
        uint32_t offset = action == "gallery-load" ? 0 : state.gallery_offset;
        if (action == "gallery-prev") offset = offset >= 10 ? offset - 10 : 0;
        if (action == "gallery-next") offset += 10;
        emit("gallery", {std::to_string(offset), values_["query-id"], values_["query-group"], values_["query-tag"]});
    } else if (action.rfind("gallery-row:", 0) == 0) {
        const auto& row = state.gallery.at(std::stoul(action.substr(12))); values_["selected-id"] = row.id; values_["selected-group"] = row.group; values_["selected-tag"] = row.tag;
    } else if (action == "reference") emit("reference", {values_["selected-id"]});
    else if (action == "metadata") emit("metadata", {values_["selected-id"], values_["selected-group"], values_["selected-tag"]});
    else if (action == "delete") confirm({"delete", {values_["selected-id"]}, {}}, "Delete this face?", "Face ID: " + values_["selected-id"] + ". This entry will be removed from the library.");
    else if (action == "export") emit("export", {values_["exchange-path"]});
    else if (action == "import-demo") confirm({"import", {state.demo_features, "features", ""}, {}}, "Import sample library?", "Import 10000 demo features. Matching face IDs will be replaced; completed batches remain if stopped.");
    else if (action == "import-features" || action == "import-images") confirm({"import", {values_["exchange-path"], action == "import-images" ? "images" : "features", values_["query-group"]}, {}}, "Import into your library?", "Entries with matching IDs will be replaced. Stopping the import keeps completed batches.");
    else if (action == "settings-apply") {
        Command command{"settings", {}, settingsFields(state.settings)};
        for (auto& f : command.fields) f.value = values_["setting:" + f.key];
        try { parseSettings(command.fields); }
        catch (const DemoError& error) {
            notice_ = error.message;
            for (const auto& f : command.fields) if (error.message.find(f.key) != std::string::npos) {
                invalid_field_ = focused_ = "setting:" + f.key; keyboard_focus_ = "field:" + focused_; cursor_ = values_[focused_].size();
                settings_tab_ = f.key.rfind("search_",0) == 0 || f.key == "group" || f.key == "tag" ? "search" : f.key.rfind("camera_",0) == 0 || f.key == "mirror" ? "camera" : f.key == "detection_faces" || f.key == "detailed" ? "detection" : "flow";
                break;
            }
            return;
        }
        commands_.push_back(std::move(command)); settings_pending_ = true; settings_attempt_ = state.settings_attempt; notice_.clear(); focused_.clear();
    }
}
void DemoUi::edit(const std::string& text_value, const std::string& field) {
    if (field.empty()) return; auto& value = values_[field]; std::string clean;
    for (unsigned char c : text_value) if (c >= 32 && c != 127) clean += char(c);
    const bool replace = select_all_ && focused_ == field;
    if ((replace ? 0 : value.size()) + clean.size() > 4096) { notice_ = "Input is limited to 4096 bytes."; return; }
    if (replace) { value.clear(); cursor_ = 0; select_all_ = false; }
    const auto position = focused_ == field ? std::min(cursor_, value.size()) : value.size(); value.insert(position, clean); if (focused_ == field) cursor_ = position + clean.size(); invalid_field_.clear(); notice_.clear();
}
void DemoUi::paste(const std::string& text_value, const std::string& field) {
    if (field != focused_) return;
    for (const auto& control : controls_) if (control.field == field && control.enabled) { edit(text_value, field); return; }
}
std::string DemoUi::selectedText() const { const auto it = values_.find(focused_); return select_all_ && it != values_.end() ? it->second : ""; }
void DemoUi::click(int x, int y, const ViewState& state) {
    pointer(x, y);
    try {
        for (auto it = controls_.rbegin(); it != controls_.rend(); ++it) if (it->bounds.contains(x, y)) {
            if (!it->enabled) return;
            keyboard_focus_ = it->id(); select_all_ = false;
            if (!it->field.empty()) {
                const bool had_caret = focused_ == it->field; const size_t previous_cursor = cursor_; focused_ = it->field;
                const auto& value = values_[focused_]; size_t at = std::min(it->text_start,value.size());
                const int wanted = x-it->bounds.x-10; int previous_x = 0;
                while (at < value.size()) {
                    size_t next = at+1; while (next < value.size() && (static_cast<unsigned char>(value[next]) & 0xc0) == 0x80) ++next;
                    const int px = measure_(value.substr(it->text_start,next-it->text_start),Font::Body) + (had_caret && previous_cursor < next ? measure_("|",Font::Body) : 0);
                    if (wanted < (previous_x+px)/2) break;
                    at = next; previous_x = px;
                }
                cursor_ = at;
            }
            else { focused_.clear(); handle(it->action, state); }
            return;
        }
        focused_.clear(); keyboard_focus_.clear(); select_all_ = false;
    } catch (const DemoError& error) { notice_ = error.message; }
    catch (const std::exception& error) { notice_ = error.what(); }
}
void DemoUi::key(Key key_value, const std::string& value, bool shift, const ViewState& state) {
    if (key_value == Key::Escape) {
        if (confirming_ || browsing_ || enrolling_) handle("cancel", state);
        else if (page_ == "image-search" && state.photo_search_busy) { focused_.clear(); keyboard_focus_.clear(); emit("cancel-image-search"); }
        else if (!focused_.empty()) { focused_.clear(); keyboard_focus_.clear(); select_all_ = false; }
        else if (page_ == "camera" && state.ready && !state.busy) emit("action", {"X"});
        return;
    }
    if (key_value >= Key::Page1 && key_value <= Key::Page5) {
        if (!confirming_ && !browsing_ && !enrolling_) { const char* pages[] = {"camera", "images", "gallery", "settings", "image-search"}; page(pages[int(key_value) - int(Key::Page1)], state); }
        return;
    }
    if (key_value == Key::Tab) {
        std::vector<const Control*> active; for (const auto& c : controls_) if (c.enabled) active.push_back(&c);
        if (active.empty()) return;
        auto it = std::find_if(active.begin(), active.end(), [&](const Control* c) { return c->id() == keyboard_focus_; });
        const size_t index = it == active.end() ? shift ? active.size() - 1 : 0 : (it - active.begin() + (shift ? active.size() - 1 : 1)) % active.size();
        keyboard_focus_ = active[index]->id(); focused_ = active[index]->field; cursor_ = values_[focused_].size(); select_all_ = false; return;
    }
    if (!focused_.empty()) {
        const auto active = std::find_if(controls_.begin(), controls_.end(), [&](const Control& control) { return control.field == focused_ && control.enabled; });
        if (active == controls_.end()) return;
        auto& text_value = values_[focused_]; cursor_ = std::min(cursor_, text_value.size());
        auto previous = [&] { if (cursor_) { --cursor_; while (cursor_ && (static_cast<unsigned char>(text_value[cursor_]) & 0xc0) == 0x80) --cursor_; } };
        auto next = [&] { if (cursor_ < text_value.size()) { ++cursor_; while (cursor_ < text_value.size() && (static_cast<unsigned char>(text_value[cursor_]) & 0xc0) == 0x80) ++cursor_; } };
        if (key_value == Key::SelectAll) { select_all_ = true; return; }
        if (key_value == Key::Text) { edit(value, focused_); return; }
        if (key_value == Key::Enter) { submitField(state); return; }
        if (select_all_ && (key_value == Key::Backspace || key_value == Key::Delete)) { text_value.clear(); cursor_ = 0; select_all_ = false; return; }
        if (key_value == Key::Backspace) { const auto old = cursor_; previous(); text_value.erase(cursor_, old - cursor_); }
        else if (key_value == Key::Delete) { const auto old = cursor_; next(); text_value.erase(old, cursor_ - old); cursor_ = old; }
        else if (key_value == Key::Left) { if (select_all_) cursor_ = 0; else previous(); }
        else if (key_value == Key::Right) { if (select_all_) cursor_ = text_value.size(); else next(); }
        else if (key_value == Key::Home) cursor_ = 0; else if (key_value == Key::End) cursor_ = text_value.size();
        select_all_ = false; return;
    }
    if (key_value == Key::Enter || key_value == Key::Text && value == " ") {
        for (const auto& c : controls_) if (c.enabled && c.id() == keyboard_focus_ && c.field.empty()) { const auto action = c.action; handle(action, state); return; }
    }
    if (confirming_ || browsing_ || enrolling_) return;
    if (page_ == "image-search" && !state.busy) {
        if (key_value == Key::Left || key_value == Key::Up) { selectImageFace(selected_image_face_-1,state); return; }
        if (key_value == Key::Right || key_value == Key::Down) { selectImageFace(selected_image_face_+1,state); return; }
        if (key_value == Key::Home) { selectImageFace(0,state); return; }
        if (key_value == Key::End) { selectImageFace(state.photo_search ? int(state.photo_search->faces.size())-1 : 0,state); return; }
        if (key_value == Key::PreviousPage || key_value == Key::NextPage) { selectImageFace(selected_image_face_ + (key_value == Key::PreviousPage ? -image_face_rows_ : image_face_rows_),state); return; }
    }
    if (key_value == Key::Text && value.size() == 1 && page_ == "camera" && content_enabled_) {
        const char c = char(std::toupper(static_cast<unsigned char>(value[0])));
        if (std::string("DCLVESX").find(c) != std::string::npos && state.connected) handle(std::string("mode:") + c, state);
        else if (c == 'M') handle("mirror", state); else if (c == 'N') handle("next", state);
        else if (c == 'P' && (state.mode == 'S' || state.mode == 'P')) handle("pause", state); else if (c == 'Q') requestClose(state);
    }
}
void DemoUi::browser() {
    controls_.clear(); const int w = std::min(760, scene_.width - 80), h = std::min(548, scene_.height - 80), x = (scene_.width - w) / 2, y = (scene_.height - h) / 2;
    scene_.box({0, 0, scene_.width, scene_.height}, 0x253342); card({x, y, w, h}); text(x + 22, y + 34, browser_target_ == "exchange-path" ? "Choose a JSON file or folder" : "Choose a photo", ink, w - 44, Font::Title);
    field({x + 22, y + 58, w - 218, 56}, "browser-path", "Directory", browser_directory_);
    button({x + w - 184, y + 79, 74, 34}, "Open", "browser-go"); button({x + w - 100, y + 79, 78, 34}, "Parent", "browser-up");
    const int row_y = y + 136, rh = 34;
    for (int i = 0; i < 8 && browser_offset_ + i < int(browser_entries_.size()); ++i) {
        const int index = browser_offset_ + i; const auto& e = browser_entries_[index]; std::error_code error; const bool dir = e.is_directory(error);
        const Rect r{x + 22, row_y + i * rh, w - 44, rh - 2}; scene_.box(r, r.contains(pointer_x_, pointer_y_) ? tint : i % 2 ? 0xf7f9fa : surface, 4);
        text(r.x + 12, r.y + 21, e.path().filename().string() + (dir ? "/" : ""), dir ? accent : ink, r.w - 24);
        if (keyboard_focus_ == "browser-row:" + std::to_string(index)) scene_.box({r.x, r.y + r.h - 2, r.w, 2}, accent);
        controls_.push_back({r, "browser-row:" + std::to_string(index), "", true});
    }
    if (!browser_error_.empty()) {
        scene_.box({x+22,row_y,w-44,116},0xffeeeb,8);
        wrap(x+36,row_y+27,browser_error_,danger,w-72,3);
        text(x+36,row_y+96,"Change the directory above or use Parent to go back.",muted,w-72,Font::Small);
    } else if (browser_entries_.empty()) {
        text(x + 32, row_y + 30, "No matching files in this folder", ink, w - 64);
        wrap(x+32,row_y+62,browser_target_ == "exchange-path" ? "Choose a folder or a .json feature file." : "Supported photos: JPEG, PNG, BMP and PPM.",muted,w-64,2,Font::Small);
    }
    const int bottom = y + h - 94;
    button({x + 22, bottom, 90, 30}, "Previous", "browser-prev", 0, browser_offset_ > 0); button({x + 120, bottom, 78, 30}, "Next", "browser-next", 0, browser_offset_ + 8 < int(browser_entries_.size()));
    text(x + 220, bottom + 20, std::to_string(browser_entries_.size()) + " items", muted, w - 250, Font::Small);
    if (browser_target_ == "exchange-path") button({x + 22, y + h - 48, 190, 34}, "Use this directory", "browser-directory", 1, browser_error_.empty());
    button({x + w - 112, y + h - 48, 90, 34}, "Cancel", "cancel");
}
void DemoUi::modal(const ViewState& state) {
    if (browsing_) { browser(); return; }
    if (!confirming_ && !enrolling_) return;
    controls_.clear(); const int w = std::min(590, scene_.width - 100), h = enrolling_ ? 358 : 250, x = (scene_.width - w) / 2, y = (scene_.height - h) / 2;
    scene_.box({0, 0, scene_.width, scene_.height}, 0x253342); card({x, y, w, h});
    text(x + 24, y + 38, enrolling_ ? "Enroll a face" : confirm_title_, ink, w - 48, Font::Title);
    if (enrolling_) {
        const auto prefix = photo_enrollment_ ? "photo-" : "enroll-";
        field({x + 24, y + 68, w - 48, 56}, std::string(prefix) + "id", photo_enrollment_ ? "Face ID (required)" : "Face ID (empty = automatic)");
        field({x + 24, y + 140, (w - 64) / 2, 56}, std::string(prefix) + "group", "Group");
        field({x + 40 + (w - 64) / 2, y + 140, (w - 64) / 2, 56}, std::string(prefix) + "tag", "Tag");
        wrap(x + 24, y + 235, notice_.empty() ? "Duplicate IDs are rejected. The new face is saved after capture succeeds." : notice_, notice_.empty() ? muted : danger, w - 48, 2, Font::Small);
        button({x + w - 224, y + h - 62, 200, 38}, photo_enrollment_ ? "Enroll photo" : "Start enrollment", "enrollment-start", 1, content_enabled_);
    } else {
        wrap(x + 24, y + 82, confirm_message_, muted, w - 48, 5);
        button({x + w - 184, y + h - 62, 160, 38}, pending_.type == "ui-quit" ? "Exit" : "Confirm", "confirm", pending_.type == "delete" ? 3 : 1, !state.busy);
    }
    button({x + 24, y + h - 62, 108, 38}, "Cancel", "cancel");
}
const Scene& DemoUi::render(const ViewState& state, int width, int height) {
    scene_ = {}; scene_.width = width; scene_.height = height; scene_.clip = {0, 0, width, height}; controls_.clear(); image_face_hits_.clear(); image_result_area_ = {};
    content_enabled_ = state.ready && !state.busy && !state.importing;
    if (state.ready && !initialized_) { initialized_ = true; values_["device"] = state.device; values_["exchange-path"] = state.data_directory + "/features.json"; resetDraft(state.settings); }
    if (settings_pending_ && state.settings_attempt > settings_attempt_) { settings_pending_ = false; if (!state.failed) resetDraft(state.settings); }
    scene_.box({0, 0, width, height}, background); const int sidebar = 176, cx = sidebar + 24, cw = width - cx - 24;
    scene_.box({0, 0, sidebar, height}, dark);
    scene_.box({20, 24, 36, 36}, accent, 10); text(27, 48, "HF", 0xffffff, 28);
    text(67, 42, "HiFace", 0xffffff, 94); text(67, 59, "Linux Demo", 0x9badbf, 100, Font::Small);
    const char* pages[] = {"camera", "images", "gallery", "settings", "image-search"}; const char* labels[] = {"Camera", "Photos / 1:1", "Face library", "Settings", "Image search"};
    const char* subtitles[] = {"Live camera", "Compare photos", "Manage identities", "Preferences", "Search every face"};
    for (int i = 0; i < 5; ++i) {
        const Rect r{12, 112 + i * 68, sidebar - 24, 54}; const bool active = page_ == pages[i], enabled = !state.busy && !state.importing;
        scene_.box(r, active ? 0x24483e : enabled && r.contains(pointer_x_, pointer_y_) ? 0x223247 : dark, 9);
        if (active) scene_.box({r.x, r.y + 12, 3, 30}, 0x79e3c0, 1);
        if (keyboard_focus_ == std::string("page:") + pages[i]) scene_.box({r.x + 15, r.y + r.h - 3, r.w - 30, 2}, 0x79e3c0);
        text(27, r.y + 23, labels[i], enabled || active ? 0xf1f6fa : 0x78879b, sidebar - 48);
        text(27, r.y + 42, subtitles[i], 0x9badbf, sidebar - 48, Font::Small);
        controls_.push_back({r, std::string("page:") + pages[i], "", enabled});
    }
    text(22, height - 104, state.ready ? "SDK ready" : state.failed ? "SDK unavailable" : "Loading SDK...", state.ready ? 0x79e3c0 : state.failed ? 0xff9696 : 0xaebfd1, sidebar - 44, Font::Small);
    button({16, height - 70, sidebar - 32, 36}, "Exit Demo", "quit", 4);
    int page_index = 0; for (int i = 0; i < 5; ++i) if (page_ == pages[i]) page_index = i;
    const char* descriptions[] = {"Preview, capture, verify and search faces", "Compare, enroll or search local photos", "Find identities and manage your face library", "Configure detection, flows, search and video", "Find every face in one photo"};
    text(cx, 40, labels[page_index], ink, cw - 300, Font::Title); text(cx, 67, descriptions[page_index], muted, cw - 300, Font::Small);
    const bool photo_page = page_ == "image-search";
    const std::string metrics[] = {photo_page ? state.photo_search_busy ? "..." : state.photo_search ? std::to_string(state.photo_search->faces.size()) + " faces" : "--" : number(state.fps) + " FPS",
        photo_page && (state.photo_search_busy || !state.photo_search) ? "--" : number(state.milliseconds) + " ms", std::to_string(photo_page && state.photo_search ? state.photo_search->library_count : state.count) + " faces"};
    for (int i = 0; i < 3; ++i) { const int x = width - 304 + i * 96; text(x, 35, metrics[i], i == 2 ? accent : ink, 90); text(x, 57, i == 0 ? photo_page ? "Detected" : "Analysis loop" : i == 1 ? photo_page ? "Photo search" : "Last analysis" : "Library", muted, 90, Font::Small); }
    const Rect body{cx, 104, cw, height - 202};
    if (page_ == "camera") camera(state, body); else if (page_ == "images") photos(state, body); else if (page_ == "gallery") library(state, body); else if (page_ == "image-search") imageSearch(state, body); else settings(state, body);
    const bool rejected = state.status.find("Rejected") != std::string::npos || state.status.find("Not matched") != std::string::npos || state.status.find("timeout") != std::string::npos;
    const bool error = state.failed || !notice_.empty();
    const bool warning = !error && !state.busy && (rejected || photo_page && state.photo_search && (state.photo_search->library_count == 0 || !state.photo_search->faces.empty() && state.photo_search->matched == 0));
    const uint32_t status_color = error ? danger : warning ? 0x9b6b16 : state.busy ? 0x365f86 : accent;
    scene_.box({cx, height - 78, cw, 58}, error ? 0xffeeeb : warning ? 0xfff4dc : state.busy ? 0xeaf1f8 : 0xe8f3ee, 9);
    scene_.box({cx + 14, height - 58, 7, 7}, status_color, 3);
    wrap(cx + 32, height - 53, friendly(notice_.empty() ? state.busy ? "Working... " + state.status : state.status : notice_), status_color, cw - 48, 2, Font::Small);
    modal(state); return scene_;
}
void DemoUi::pageForSmoke(unsigned index) { const char* names[] = {"camera", "images", "gallery", "settings", "image-search"}; page_ = names[index % 5]; }
}
