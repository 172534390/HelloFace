#include "app.h"
#include <algorithm>
#include <cstdio>

namespace hiface_linux_demo {
DesktopApp::DesktopApp(std::string models, std::string database, std::string device, bool smoke, std::string assets)
    : worker_([this, models, database, device, smoke, assets] { run(models, database, device, smoke, assets); }) {}
DesktopApp::~DesktopApp() { stopping_ = true; if (worker_.joinable()) worker_.join(); }
void DesktopApp::post(Command command) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (command.type == "image-search" || command.type == "cancel-image-search") {
        command.request_id = ++photo_generation_;
        commands_.erase(std::remove_if(commands_.begin(), commands_.end(), [](const Command& queued) { return queued.type == "image-search" || queued.type == "cancel-image-search"; }), commands_.end());
        auto pending = std::make_shared<ViewState>(*state_);
        pending->photo_search_generation = command.request_id; pending->photo_search_busy = pending->busy = true; pending->failed = false;
        pending->search_photo.reset(); pending->photo_search.reset(); pending->search_photo_path.clear();
        pending->status = command.type == "image-search" ? "Queued photo search..." : "Cancelling photo search..."; state_ = std::move(pending);
        commands_.push_back(std::move(command)); return;
    }
    // Bound repeated key presses while a model call is in flight.
    if (commands_.size() < 32) commands_.push_back(std::move(command));
}
std::shared_ptr<const ViewState> DesktopApp::snapshot() { std::lock_guard<std::mutex> lock(mutex_); return state_; }
void DesktopApp::run(const std::string& models, const std::string& database, std::string device, bool smoke, const std::string& assets) {
    auto publish = [&](std::shared_ptr<ViewState> state) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state->ready && state->photo_search_generation != photo_generation_) {
            state->photo_search_generation = photo_generation_; state->photo_search_busy = state->busy = true;
            state->search_photo.reset(); state->photo_search.reset(); state->search_photo_path.clear(); state->failed = false; state->status = state_->status;
        }
        state_ = std::move(state);
    };
    try {
        const auto data = std::filesystem::path(database).has_parent_path() ? std::filesystem::path(database).parent_path() : std::filesystem::path(".");
        std::filesystem::create_directories(data);
        // Only this worker creates or calls Native resources; teardown runs on it too.
        FaceWorkflow workflow(models, database);
        bool failed = false;
        uint64_t settings_attempt = 0, photo_generation = 0;
        Camera camera;
        std::shared_ptr<CameraFrame> frame;
        std::string page = "camera", gallery_id, gallery_group, gallery_tag;
        uint32_t gallery_offset = 0;
        int next_index = 0;
        int64_t fps_start = monotonicMilliseconds(), last_frame = fps_start, last_publish = 0;
        uint32_t frames = 0; double fps = 0;
        if (device.rfind("/dev/video", 0) == 0) try { next_index = std::stoi(device.substr(10)); } catch (...) {}
        auto connect = [&] {
            workflow.stop(); camera.close(); frame.reset();
            last_frame = monotonicMilliseconds(); fps_start = last_frame; frames = 0; fps = 0;
            if (smoke) {
                frame = std::make_shared<CameraFrame>(); rgbaFrame(Image{640, 480, std::vector<uint8_t>(640 * 480 * 4, 128)}, *frame);
                return;
            }
            const auto& s = workflow.settings(); camera.open(device, s.camera_width, s.camera_height, s.camera_fps, s.camera_mjpeg);
            workflow.status = "Camera connected. Look at the camera.";
        };
        try { connect(); } catch (const DemoError& error) { workflow.status = error.message; failed = true; }
        auto gallery = [&] { workflow.loadGallery(gallery_offset, gallery_id, gallery_group, gallery_tag); };
        while (!stopping_) {
            std::vector<Command> commands;
            { std::lock_guard<std::mutex> lock(mutex_); commands.swap(commands_); }
            for (const auto& command : commands) {
                const bool photo_command = command.type == "image-search" || command.type == "cancel-image-search";
                if (photo_command && command.request_id != photo_generation_) continue;
                if (photo_command) photo_generation = command.request_id;
                if (command.type == "settings") ++settings_attempt;
                auto working = std::make_shared<ViewState>(*snapshot());
                working->busy = true; working->failed = false; working->status = command.type == "image-search" ? "Detecting and searching every face..." : command.type == "image" ? "Processing photo" : command.type == "import" ? "Reading import source" : "Applying request";
                publish(working);
                try {
                    failed = false;
                    const auto& v = command.values;
                    if (workflow.importing()) require(command.type == "cancel-import" || command.type == "gallery",
                        "Stop the import before another operation");
                    if (command.type == "page") {
                        workflow.stop(); page = v.at(0);
                        if (page == "camera") connect(); else camera.close();
                        if (page == "gallery") gallery();
                        if (page == "image-search") workflow.status = workflow.photoSearchSummary();
                    } else if (command.type == "image-search") {
                        page = "image-search"; camera.close(); frame.reset();
                        workflow.searchPhoto(v.at(0), [&] { return stopping_ || command.request_id != photo_generation_; });
                    } else if (command.type == "cancel-image-search") { workflow.clearPhotoSearch(); workflow.status = "Photo search cancelled.";
                    } else if (command.type == "action") {
                        require(smoke || camera.ready(), "Connect a camera first"); workflow.action(v.at(0).at(0));
                    } else if (command.type == "connect") { device = v.at(0); connect(); }
                    else if (command.type == "next") {
                        bool connected = false;
                        for (int i = 0; i < 64; ++i) {
                            next_index = (next_index + 1) % 64; device = "/dev/video" + std::to_string(next_index);
                            if (!std::filesystem::exists(device)) continue;
                            try { connect(); connected = true; break; } catch (const DemoError&) {}
                        }
                        if (!connected) workflow.status = "No usable camera. Connect the USB device to Ubuntu and check /dev/video* permissions.";
                    } else if (command.type == "mirror") workflow.toggleMirror();
                    else if (command.type == "pause") workflow.pauseSearch();
                    else if (command.type == "settings") { workflow.applySettings(parseSettings(command.fields)); }
                    else if (command.type == "enroll") {
                        require(smoke || camera.ready(), "Connect a camera first"); workflow.setEnrollment(v.at(0), v.at(1), v.at(2)); workflow.action('E');
                    } else if (command.type == "image") workflow.imageOperation(v.at(0), v.at(1), v.at(2), v.at(3), v.at(4), v.at(5));
                    else if (command.type == "gallery") {
                        gallery_offset = static_cast<uint32_t>(std::stoul(v.at(0))); gallery_id = v.at(1); gallery_group = v.at(2); gallery_tag = v.at(3); gallery();
                    } else if (command.type == "delete") { workflow.deleteEntry(v.at(0)); gallery(); workflow.status = "Deleted " + v.at(0); }
                    else if (command.type == "metadata") { workflow.updateEntry(v.at(0), v.at(1), v.at(2)); gallery(); }
                    else if (command.type == "reference") workflow.selectReference(v.at(0));
                    else if (command.type == "clear-reference") workflow.clearReference();
                    else if (command.type == "export") workflow.exportGallery(v.at(0), gallery_group, gallery_tag);
                    else if (command.type == "import") workflow.startImport(v.at(0), v.at(1) == "images", v.at(2));
                    else if (command.type == "cancel-import") { workflow.cancelImport(); const auto status = workflow.status; gallery(); workflow.status = status; }
                } catch (const DemoError& error) { failed = true; workflow.status = error.message; std::fprintf(stderr, "[Demo] %s\n", error.message.c_str()); }
                catch (const std::exception& error) { failed = true; workflow.status = error.what(); }
                catch (...) { failed = true; workflow.status = "Unexpected command failure"; }
            }
            if (workflow.importing()) {
                try {
                    workflow.stepImport();
                    if (!workflow.importing()) { const auto status = workflow.status; gallery(); workflow.status = status; }
                } catch (const DemoError& error) { failed = true; workflow.status += " | " + error.message; }
                catch (const std::exception& error) { failed = true; workflow.cancelImport(); workflow.status = error.what(); }
            }
            if (page == "camera" && !workflow.importing()) {
                bool received = smoke;
                auto next = smoke ? frame : std::make_shared<CameraFrame>();
                if (!smoke) try { received = camera.read(*next); }
                catch (const DemoError& error) { failed = true; workflow.stop(); camera.close(); frame.reset(); workflow.status = error.message; }
                if (received && next) {
                    frame = next; last_frame = monotonicMilliseconds(); ++frames;
                    try { workflow.process(*frame); }
                    catch (const DemoError& error) { failed = true; workflow.stop(); workflow.status = error.message; }
                }
            }
            const auto now = monotonicMilliseconds();
            if (now - fps_start >= 1000) { fps = frames * 1000.0 / (now - fps_start); frames = 0; fps_start = now; }
            if (now - last_publish >= 30 || !commands.empty()) {
                auto state = std::make_shared<ViewState>();
                state->ready = true; state->importing = workflow.importing(); state->connected = smoke || camera.ready();
                state->failed = failed; state->settings_attempt = settings_attempt;
                state->photo_search_generation = photo_generation;
                state->search_photo = workflow.search_photo; state->photo_search = workflow.photo_search; state->search_photo_path = workflow.search_photo_path;
                state->import_saved = workflow.import_saved; state->import_failed = workflow.import_failed; state->import_processed = workflow.import_processed; state->import_total = workflow.import_total;
                state->status = workflow.status; state->camera = smoke ? "Synthetic frame / desktop smoke test" : camera.ready() ? camera.description() : "Camera disconnected";
                state->device = device; state->reference = workflow.reference_id; state->data_directory = data.string();
                const auto fixture = std::filesystem::path(assets) / "features_10000.json";
                if (std::filesystem::exists(fixture)) state->demo_features = fixture.string();
                state->mode = workflow.mode(); state->count = workflow.gallery_count; state->filtered_count = workflow.filtered_count; state->gallery_offset = workflow.gallery_offset;
                state->settings = workflow.settings(); state->milliseconds = workflow.inference_ms; state->fps = fps; state->frame = frame;
                if (page == "camera" && camera.ready() && now - last_frame > 3000)
                    state->status = "Camera is connected but no frames arrived for 3 seconds. Check VMware USB passthrough or reconnect.";
                state->boxes = workflow.boxes;
                if (page == "camera" && (now - last_frame > 500 || state->mode == 'P')) state->boxes.clear();
                state->gallery = workflow.gallery_rows; state->image_first = workflow.image_first; state->image_second = workflow.image_second; state->aligned_preview = workflow.aligned_preview;
                state->image_matches = workflow.image_matches;
                publish(state); last_publish = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        workflow.cancelImport(); workflow.stop();
    } catch (const DemoError& error) { auto state = std::make_shared<ViewState>(); state->failed = true; state->status = "SDK initialization failed: " + error.message; publish(state); }
    catch (const std::exception& error) { auto state = std::make_shared<ViewState>(); state->failed = true; state->status = error.what(); publish(state); }
    catch (...) { auto state = std::make_shared<ViewState>(); state->failed = true; state->status = "Unexpected worker failure"; publish(state); }
}
}
