#include "window.h"
#include <iostream>
#include <cstdlib>
using namespace hiface_linux_demo;
namespace {
void logMessage(HiFaceLogLevel level, const char* tag, const char* message, void*) {
    if (level >= HIFACE_LOG_WARNING) std::fprintf(stderr, "[%s] %s\n", tag, message);
}
}
int main(int argc, char** argv) {
    const bool help = argc == 2 && std::string(argv[1]) == "--help";
    if (help || argc < 3 || argc > 4 || std::string(argv[1]) != "--builtin") {
        std::cout << "Usage: hiface_desktop --builtin DATABASE [CAMERA_DEVICE|--smoke-test]\n"
                     "F1 Camera, F2 Photos / comparison, F3 Face library, F4 Settings, F5 Image search.\n"
                     "Camera shortcuts: D detect, C capture, L liveness, V verify, E enroll, S search, P pause, X stop, M mirror, N next camera, Q quit.\n";
        return help ? 0 : 2;
    }
    try {
        DesktopWindow window;
        hiface_set_log_callback_v1(logMessage, nullptr);
        const bool smoke = argc == 4 && std::string(argv[3]) == "--smoke-test";
        const char* assets_env = std::getenv("HIFACE_DEMO_ASSETS_DIR");
        std::error_code error;
        auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
        if (error) executable = std::filesystem::absolute(argv[0]);
        const auto assets = assets_env ? std::string(assets_env) : (executable.parent_path().parent_path() / "assets").string();
        DesktopApp app(argv[1], argv[2], argc == 4 && !smoke ? argv[3] : "/dev/video0", smoke, assets);
        unsigned smoke_frames = 0;
        const auto start = monotonicMilliseconds();
        while (window.open()) {
            const auto state = app.snapshot();
            for (auto command : window.events(*state)) app.post(std::move(command));
            window.paint(*state);
            if (smoke) {
                require(monotonicMilliseconds() - start < 60000, "Desktop smoke test timed out during initialization");
                if (state->ready) {
                    if (++smoke_frames % 5 == 0) window.smokePage(smoke_frames / 5);
                    if (smoke_frames == 30) { std::cout << "Desktop smoke test passed: camera, photos, gallery, settings and multi-face image search rendered\n"; return 0; }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
        return 0;
    } catch (const DemoError& error) { std::cerr << error.message << '\n'; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; }
    catch (...) { std::cerr << "Unexpected desktop failure\n"; }
    return 1;
}
