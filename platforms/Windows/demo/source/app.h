#pragma once
#include "camera.h"
#include "data_exchange.h"
#include "demo_service.h"
#include <windows.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace hiface_demo {
constexpr UINT kFrameReady = WM_APP + 1;
enum class Page { Detection, Reference, Verification, Liveness, Search, ImageSearch, Comparison, Gallery };
struct Settings {
    Page page = Page::Search;
    SearchOptions search;
    FlowOptions flow;
    unsigned detection_max = 3, camera_index = 0, width = 640, height = 480;
    bool detailed = true, mirror = false;
    int rotation = 0;
    std::wstring camera_link;
    std::string id = "person-001", group, tag, filter_group, filter_tag;
};
struct Snapshot {
    Settings settings;
    std::shared_ptr<const Image> preview, image_a, image_b, aligned_a, aligned_b, captured;
    Update update;
    std::shared_ptr<const Image> search_photo; // BGRA for the native view
    std::filesystem::path search_file;
    std::shared_ptr<const ImageSearchResult> image_search;
    std::vector<Person> people;
    uint64_t total = 0, filtered = 0;
    uint32_t offset = 0;
    uint64_t completed_command = 0, search_generation = 0;
    bool error = false;
    bool camera_open = false, busy = false, flow_active = false, paused = false, has_reference = false, fatal = false;
    double capture_fps = 0, analysis_fps = 0, inference_ms = 0;
};
enum class Action { Apply, OpenCamera, CloseCamera, StartFlow, Enroll, Cancel, Pause,
    LoadA, LoadB, Compare, SetImageReference, SetGalleryReference, ImportReference,
    ExportFeature, ExportPreview, ImageEnroll, RefreshGallery, PreviousPage, NextPage,
    DeletePerson, UpdatePerson, ImportJson, ExportJson, ImportImages, SearchPhoto, SearchPhotoAgain, ReleaseMemory };
struct Command {
    Action action = Action::Apply;
    Settings settings;
    std::vector<std::filesystem::path> files;
    uint64_t generation = 0, sequence = 0;
};
class App {
public:
    HWND window = nullptr;
    std::filesystem::path database;
    std::vector<CameraDevice> cameras; // UI thread only
    std::atomic<uint64_t> generation{0};
    std::atomic<bool> notification_pending{false};
    ~App();
    void start();
    void stop();
    uint64_t enqueue(Command command);
    std::shared_ptr<const Snapshot> current();
private:
    std::atomic<bool> stopping_{false};
    std::atomic<bool> active_{false};
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable changed_;
    std::deque<Command> commands_;
    uint64_t next_command_ = 0;
    std::shared_ptr<const Snapshot> snapshot_ = std::make_shared<Snapshot>();
    void publish(const Snapshot& state);
    void run();
};
std::shared_ptr<const Image> bgraImage(const Image& image);
}
