#pragma once
#include "workflow.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace hiface_linux_demo {
struct Command {
    std::string type;
    std::vector<std::string> values;
    std::vector<FormField> fields;
    uint64_t request_id = 0;
};
struct ViewState {
    bool ready = false, importing = false, connected = false, busy = false, failed = false;
    uint64_t settings_attempt = 0, photo_search_generation = 0;
    bool photo_search_busy = false;
    std::shared_ptr<Image> search_photo;
    std::shared_ptr<const ImageSearchReport> photo_search;
    std::string search_photo_path;
    uint32_t import_saved = 0, import_failed = 0, import_processed = 0, import_total = 0;
    std::string status = "Loading five SDK models...", camera, device = "/dev/video0", reference, data_directory, demo_features;
    char mode = 'D';
    uint64_t count = 0, filtered_count = 0;
    uint32_t gallery_offset = 0;
    double milliseconds = 0, fps = 0;
    DemoSettings settings;
    std::shared_ptr<CameraFrame> frame;
    std::shared_ptr<Image> image_first, image_second, aligned_preview;
    std::vector<Box> boxes;
    std::vector<GalleryEntry> gallery;
    std::vector<GalleryEntry> image_matches;
};
// Owns camera, session and database on one worker. UI commands and snapshots are
// copied under a short mutex; inference, filesystem I/O and X11 never share it.
class DesktopApp {
public:
    DesktopApp(std::string models, std::string database, std::string device, bool smoke, std::string assets = {});
    ~DesktopApp();
    void post(Command command);
    std::shared_ptr<const ViewState> snapshot();
private:
    void run(const std::string& models, const std::string& database, std::string device, bool smoke, const std::string& assets);
    std::atomic<bool> stopping_{false};
    std::atomic<uint64_t> photo_generation_{0};
    std::mutex mutex_;
    std::vector<Command> commands_;
    std::shared_ptr<const ViewState> state_ = std::make_shared<ViewState>();
    std::thread worker_;
};
}
