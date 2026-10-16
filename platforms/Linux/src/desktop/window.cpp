#include "window.h"
#include "ui.h"
#include <X11/Xlib.h>
#include <algorithm>
#include <array>
#include <clocale>
#include <cstdlib>
#include <dlfcn.h>
namespace hiface_linux_demo {
struct DesktopWindow::Impl {
    void* library = nullptr; Display* display = nullptr; Window window = 0; Pixmap back_buffer = 0; GC gc = nullptr; Visual* visual = nullptr; XImage* image = nullptr;
    std::array<XFontSet, 3> fonts{}; XIM input_method = nullptr; XIC input_context = nullptr;
    Atom delete_window = 0, clipboard = 0, utf8 = 0, paste_property = 0, targets = 0, atom_type = 0, string_type = 0;
    int depth = 0, width = 1060, height = 740, buffer_width = 0, buffer_height = 0;
    std::string paste_target, clipboard_text;
#define FN(name) decltype(&name) name##_fn = nullptr
    FN(XOpenDisplay); FN(XCloseDisplay); FN(XDefaultScreen); FN(XDisplayWidth); FN(XDisplayHeight); FN(XDefaultVisual); FN(XDefaultDepth); FN(XRootWindow);
    FN(XCreateSimpleWindow); FN(XDestroyWindow); FN(XMapWindow); FN(XResizeWindow); FN(XStoreName); FN(XSelectInput);
    FN(XCreateGC); FN(XFreeGC); FN(XPending); FN(XNextEvent); FN(XSetForeground); FN(XFillRectangle); FN(XFillArc); FN(XDrawString);
    FN(XCreateImage); FN(XPutImage); FN(XFlush); FN(XInternAtom); FN(XSetWMProtocols); FN(XCreatePixmap); FN(XFreePixmap); FN(XCopyArea);
    FN(XCreateFontSet); FN(XFreeFontSet); FN(XFreeStringList); FN(Xutf8DrawString); FN(Xutf8TextExtents);
    FN(XOpenIM); FN(XCloseIM); FN(XCreateIC); FN(XDestroyIC); FN(XSetICFocus); FN(XUnsetICFocus); FN(Xutf8LookupString); FN(XFilterEvent);
    FN(XConvertSelection); FN(XGetWindowProperty); FN(XDeleteProperty); FN(XFree); FN(XSetSelectionOwner); FN(XChangeProperty); FN(XSendEvent);
    FN(XSetClipRectangles); FN(XSetClipMask);
#undef FN
    int (*lookup)(XKeyEvent*, char*, int, KeySym*, void*) = nullptr;
    DemoUi ui{[this](const std::string& text, Font font) { return measure(text, font); }};
    void initialize() {
        std::setlocale(LC_CTYPE, ""); library = dlopen("libX11.so.6", RTLD_NOW | RTLD_LOCAL); require(library, "X11 runtime missing: install libx11-6");
#define LOAD(name) name##_fn = reinterpret_cast<decltype(name##_fn)>(dlsym(library, #name)); require(name##_fn, "Missing X11 function: " #name)
        LOAD(XOpenDisplay); LOAD(XCloseDisplay); LOAD(XDefaultScreen); LOAD(XDisplayWidth); LOAD(XDisplayHeight); LOAD(XDefaultVisual); LOAD(XDefaultDepth); LOAD(XRootWindow);
        LOAD(XCreateSimpleWindow); LOAD(XDestroyWindow); LOAD(XMapWindow); LOAD(XResizeWindow); LOAD(XStoreName); LOAD(XSelectInput);
        LOAD(XCreateGC); LOAD(XFreeGC); LOAD(XPending); LOAD(XNextEvent); LOAD(XSetForeground); LOAD(XFillRectangle); LOAD(XFillArc); LOAD(XDrawString);
        LOAD(XCreateImage); LOAD(XPutImage); LOAD(XFlush); LOAD(XInternAtom); LOAD(XSetWMProtocols); LOAD(XCreatePixmap); LOAD(XFreePixmap); LOAD(XCopyArea);
        LOAD(XCreateFontSet); LOAD(XFreeFontSet); LOAD(XFreeStringList); LOAD(Xutf8DrawString); LOAD(Xutf8TextExtents);
        LOAD(XOpenIM); LOAD(XCloseIM); LOAD(XCreateIC); LOAD(XDestroyIC); LOAD(XSetICFocus); LOAD(XUnsetICFocus); LOAD(Xutf8LookupString); LOAD(XFilterEvent);
        LOAD(XConvertSelection); LOAD(XGetWindowProperty); LOAD(XDeleteProperty); LOAD(XFree); LOAD(XSetSelectionOwner); LOAD(XChangeProperty); LOAD(XSendEvent);
        LOAD(XSetClipRectangles); LOAD(XSetClipMask);
#undef LOAD
        lookup = reinterpret_cast<decltype(lookup)>(dlsym(library, "XLookupString")); require(lookup, "Missing X11 keyboard support");
        display = XOpenDisplay_fn(nullptr); require(display, "Cannot open DISPLAY. Start from an Ubuntu desktop terminal (X11 or XWayland).");
        const int screen = XDefaultScreen_fn(display); visual = XDefaultVisual_fn(display, screen); depth = XDefaultDepth_fn(display, screen); require(depth == 24 || depth == 32, "This Demo requires a 24/32-bit desktop visual");
        width = std::max(960, std::min(width, XDisplayWidth_fn(display,screen)-48));
        height = std::max(700, std::min(height, XDisplayHeight_fn(display,screen)-80));
        window = XCreateSimpleWindow_fn(display, XRootWindow_fn(display, screen), 20, 20, width, height, 0, 0, pixel(0xf5f5f7)); require(window, "Cannot create window");
        XStoreName_fn(display, window, "HiFace - Linux Demo"); XSelectInput_fn(display, window, ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask | StructureNotifyMask | FocusChangeMask | PropertyChangeMask);
        delete_window = XInternAtom_fn(display, "WM_DELETE_WINDOW", False); clipboard = XInternAtom_fn(display, "CLIPBOARD", False); utf8 = XInternAtom_fn(display, "UTF8_STRING", False);
        paste_property = XInternAtom_fn(display, "HIFACE_PASTE", False); targets = XInternAtom_fn(display, "TARGETS", False); atom_type = XInternAtom_fn(display, "ATOM", False); string_type = XInternAtom_fn(display, "STRING", False);
        XSetWMProtocols_fn(display, window, &delete_window, 1); gc = XCreateGC_fn(display, window, 0, nullptr); require(gc, "Cannot create graphics context");
        const char* patterns[] = {"-*-helvetica-medium-r-normal--14-*-*-*-*-*-*-*,fixed", "-*-helvetica-medium-r-normal--12-*-*-*-*-*-*-*,fixed", "-*-helvetica-bold-r-normal--22-*-*-*-*-*-*-*,fixed"};
        for (size_t i = 0; i < fonts.size(); ++i) { char** missing = nullptr; int count = 0; char* fallback = nullptr; fonts[i] = XCreateFontSet_fn(display, patterns[i], &missing, &count, &fallback); if (missing) XFreeStringList_fn(missing); }
        input_method = XOpenIM_fn(display, nullptr, nullptr, nullptr);
        if (input_method) input_context = XCreateIC_fn(input_method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window, XNFocusWindow, window, static_cast<char*>(nullptr));
        XMapWindow_fn(display, window); XFlush_fn(display);
    }
    ~Impl() {
        if (image) image->f.destroy_image(image); if (back_buffer) XFreePixmap_fn(display, back_buffer);
        if (input_context) XDestroyIC_fn(input_context); if (input_method) XCloseIM_fn(input_method);
        for (auto font : fonts) if (font) XFreeFontSet_fn(display, font);
        if (gc) XFreeGC_fn(display, gc); if (window) XDestroyWindow_fn(display, window); if (display) XCloseDisplay_fn(display); if (library) dlclose(library);
    }
    unsigned long pixel(uint32_t rgb) const {
        if (visual->red_mask == 0xff0000 && visual->green_mask == 0xff00 && visual->blue_mask == 0xff) return rgb;
        auto channel = [](uint32_t v, unsigned long mask) { if (!mask) return 0UL; unsigned shift = 0; while (!((mask >> shift) & 1)) ++shift; return ((v * (mask >> shift) + 127) / 255 << shift) & mask; };
        return channel((rgb >> 16) & 255, visual->red_mask) | channel((rgb >> 8) & 255, visual->green_mask) | channel(rgb & 255, visual->blue_mask);
    }
    int measure(const std::string& text, Font font) const {
        if (fonts[size_t(font)]) { XRectangle ink{}, logical{}; Xutf8TextExtents_fn(fonts[size_t(font)], text.c_str(), text.size(), &ink, &logical); return logical.width; }
        return int(text.size()) * 7;
    }
    void box(const Draw& d) {
        const auto r = d.bounds; if (r.w <= 0 || r.h <= 0) return; const int radius = std::min({d.radius, r.w / 2, r.h / 2});
        XSetForeground_fn(display, gc, pixel(d.color));
        if (!radius) { XFillRectangle_fn(display, back_buffer, gc, r.x, r.y, r.w, r.h); return; }
        XFillRectangle_fn(display, back_buffer, gc, r.x + radius, r.y, r.w - radius * 2, r.h); XFillRectangle_fn(display, back_buffer, gc, r.x, r.y + radius, r.w, r.h - radius * 2);
        XFillArc_fn(display, back_buffer, gc, r.x, r.y, radius * 2, radius * 2, 90 * 64, 90 * 64);
        XFillArc_fn(display, back_buffer, gc, r.x + r.w - radius * 2, r.y, radius * 2, radius * 2, 0, 90 * 64);
        XFillArc_fn(display, back_buffer, gc, r.x, r.y + r.h - radius * 2, radius * 2, radius * 2, 180 * 64, 90 * 64);
        XFillArc_fn(display, back_buffer, gc, r.x + r.w - radius * 2, r.y + r.h - radius * 2, radius * 2, radius * 2, 270 * 64, 90 * 64);
    }
    void picture(const Draw& d) {
        const int w = d.bounds.w, h = d.bounds.h; if (!d.pixels || w < 1 || h < 1) return;
        if (!image || image->width != w || image->height != h) {
            if (image) { image->f.destroy_image(image); image = nullptr; }
            image = XCreateImage_fn(display, visual, depth, ZPixmap, 0, nullptr, w, h, 32, 0); require(image && image->bits_per_pixel == 32, "Unsupported X11 image format");
            image->data = static_cast<char*>(std::calloc(image->bytes_per_line, image->height)); require(image->data, "Cannot allocate preview image");
        }
        for (int y = 0; y < h; ++y) {
            auto* row = reinterpret_cast<uint32_t*>(image->data + size_t(y) * image->bytes_per_line);
            for (int x = 0; x < w; ++x) {
                int sx = x * d.source_width / w; if (d.mirror) sx = d.source_width - 1 - sx;
                const auto* p = d.pixels + (size_t(y * d.source_height / h) * d.source_width + sx) * 4;
                row[x] = pixel(uint32_t(p[0]) << 16 | uint32_t(p[1]) << 8 | p[2]);
            }
        }
        XPutImage_fn(display, back_buffer, gc, image, 0, 0, d.bounds.x, d.bounds.y, w, h);
    }
    void paint(const ViewState& state) {
        if (!window || !ui.open()) return;
        if (!back_buffer || width != buffer_width || height != buffer_height) { if (back_buffer) XFreePixmap_fn(display, back_buffer); back_buffer = XCreatePixmap_fn(display, window, width, height, depth); require(back_buffer, "Cannot create back buffer"); buffer_width = width; buffer_height = height; }
        const auto& scene = ui.render(state, width, height);
        for (const auto& d : scene.draws) {
            const auto clip = d.clip.intersect({0, 0, width, height}); if (!clip.w || !clip.h) continue;
            XRectangle rectangle{static_cast<short>(clip.x), static_cast<short>(clip.y), static_cast<unsigned short>(clip.w), static_cast<unsigned short>(clip.h)};
            XSetClipRectangles_fn(display, gc, 0, 0, &rectangle, 1, Unsorted);
            if (d.kind == Draw::Box) box(d);
            else if (d.kind == Draw::Image) picture(d);
            else { XSetForeground_fn(display, gc, pixel(d.color)); if (fonts[size_t(d.font)]) Xutf8DrawString_fn(display, back_buffer, fonts[size_t(d.font)], gc, d.bounds.x, d.bounds.y, d.text.c_str(), d.text.size()); else XDrawString_fn(display, back_buffer, gc, d.bounds.x, d.bounds.y, d.text.c_str(), d.text.size()); }
        }
        XSetClipMask_fn(display, gc, None); XCopyArea_fn(display, back_buffer, window, gc, 0, 0, width, height, 0, 0); XFlush_fn(display);
    }
    void key(XKeyEvent& event, const ViewState& state) {
        char buffer[256]{}; KeySym symbol = 0; Status status = 0;
        int count = input_context ? Xutf8LookupString_fn(input_context, &event, buffer, sizeof(buffer), &symbol, &status) : lookup(&event, buffer, sizeof(buffer), &symbol, nullptr);
        if (count < 0 || count > int(sizeof(buffer))) count = 0;
        const bool ctrl = event.state & ControlMask, shift = event.state & ShiftMask;
        if (ctrl && (symbol == 'v' || symbol == 'V') && !ui.focusedField().empty()) { paste_target = ui.focusedField(); XConvertSelection_fn(display, clipboard, utf8, paste_property, window, event.time); return; }
        if (ctrl && (symbol == 'c' || symbol == 'C')) { const auto text = ui.selectedText(); if (!text.empty()) { clipboard_text = text; XSetSelectionOwner_fn(display, clipboard, window, event.time); } return; }
        Key key = Key::Text;
        if (ctrl && (symbol == 'a' || symbol == 'A')) key = Key::SelectAll;
        else switch (symbol) {
            case 0xff1b: key = Key::Escape; break; case 0xff0d: key = Key::Enter; break; case 0xff08: key = Key::Backspace; break; case 0xffff: key = Key::Delete; break;
            case 0xff51: key = Key::Left; break; case 0xff53: key = Key::Right; break; case 0xff52: key = Key::Up; break; case 0xff54: key = Key::Down; break; case 0xff55: key = Key::PreviousPage; break; case 0xff56: key = Key::NextPage; break; case 0xff50: key = Key::Home; break; case 0xff57: key = Key::End; break;
            case 0xff09: case 0xfe20: key = Key::Tab; break;
            case 0xffbe: key = Key::Page1; break; case 0xffbf: key = Key::Page2; break; case 0xffc0: key = Key::Page3; break; case 0xffc1: key = Key::Page4; break; case 0xffc2: key = Key::Page5; break;
            default: if (ctrl) return;
        }
        if (key != Key::Text || count) ui.key(key, std::string(buffer, count), shift, state);
    }
    void selection(const XSelectionRequestEvent& request) {
        const auto property = request.property ? request.property : request.target; XEvent reply{}; auto& e = reply.xselection;
        e.type = SelectionNotify; e.display = display; e.requestor = request.requestor; e.selection = request.selection; e.target = request.target; e.time = request.time; e.property = None;
        if (request.target == targets) { const Atom types[] = {targets, utf8}; XChangeProperty_fn(display, request.requestor, property, atom_type, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(types), 2); e.property = property; }
        else if (request.target == utf8 || request.target == string_type && std::all_of(clipboard_text.begin(), clipboard_text.end(), [](unsigned char c) { return c < 128; })) {
            XChangeProperty_fn(display, request.requestor, property, request.target, 8, PropModeReplace, reinterpret_cast<const unsigned char*>(clipboard_text.data()), clipboard_text.size()); e.property = property;
        }
        XSendEvent_fn(display, request.requestor, False, 0, &reply); XFlush_fn(display);
    }
};
DesktopWindow::DesktopWindow() : impl_(new Impl) { impl_->initialize(); }
DesktopWindow::~DesktopWindow() = default;
bool DesktopWindow::open() const { return impl_->window && impl_->ui.open(); }
void DesktopWindow::paint(const ViewState& state) { impl_->paint(state); }
void DesktopWindow::smokePage(unsigned index) { impl_->ui.pageForSmoke(index); }
std::vector<Command> DesktopWindow::events(const ViewState& state) {
    auto& ui = *impl_;
    while (ui.XPending_fn(ui.display)) {
        XEvent event{}; ui.XNextEvent_fn(ui.display, &event); if (ui.XFilterEvent_fn(&event, ui.window)) continue;
        if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == ui.delete_window) ui.ui.requestClose(state);
        if (event.type == DestroyNotify) { ui.window = 0; }
        if (event.type == FocusIn && ui.input_context) ui.XSetICFocus_fn(ui.input_context); if (event.type == FocusOut && ui.input_context) ui.XUnsetICFocus_fn(ui.input_context);
        if (event.type == ConfigureNotify) { ui.width = std::max(960, event.xconfigure.width); ui.height = std::max(700, event.xconfigure.height); if (ui.width != event.xconfigure.width || ui.height != event.xconfigure.height) ui.XResizeWindow_fn(ui.display, ui.window, ui.width, ui.height); }
        try {
            if (event.type == MotionNotify) ui.ui.pointer(event.xmotion.x, event.xmotion.y);
            if (event.type == KeyPress) ui.key(event.xkey, state);
            if (event.type == ButtonPress && event.xbutton.button == Button1) ui.ui.click(event.xbutton.x, event.xbutton.y, state);
            if (event.type == ButtonPress && (event.xbutton.button == Button4 || event.xbutton.button == Button5)) ui.ui.scroll(event.xbutton.button == Button4 ? -1 : 1, event.xbutton.x, event.xbutton.y, state);
            if (event.type == SelectionRequest) ui.selection(event.xselectionrequest);
            if (event.type == SelectionClear) ui.clipboard_text.clear();
            if (event.type == SelectionNotify && event.xselection.property == ui.paste_property) {
                Atom type = 0; int format = 0; unsigned long count = 0, remaining = 0; unsigned char* value = nullptr;
                if (ui.XGetWindowProperty_fn(ui.display, ui.window, ui.paste_property, 0, 1024, True, ui.utf8, &type, &format, &count, &remaining, &value) == Success && type == ui.utf8 && format == 8 && !remaining && value) ui.ui.paste(std::string(reinterpret_cast<char*>(value), count), ui.paste_target);
                if (value) ui.XFree_fn(value); ui.XDeleteProperty_fn(ui.display, ui.window, ui.paste_property); ui.paste_target.clear();
            }
        } catch (const DemoError& error) { ui.ui.reportError(error.message); }
        catch (const std::exception& error) { ui.ui.reportError(error.what()); }
    }
    return ui.ui.takeCommands();
}
}
