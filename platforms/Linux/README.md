# HiFace Linux Demo

English | [简体中文](README.zh-CN.md)

SDK **Beta**, x86_64 / ARM64, CPU. Ubuntu 22.04+ (glibc 2.35+); desktop requires X11 or XWayland.
Beta expires **2026-12-24 09:08:42 (UTC+08:00)**. Current models do not support recognition of minors. Target-device acceptance is pending.

## Run

From `platforms/Linux` in HelloFace, or an extracted Demo package:

```bash
./self-test.sh
./run-demo.sh
# Select another camera:
./run-demo.sh /dev/video2
```

HelloFace launchers select `runtimes/x86_64` or `runtimes/arm64` automatically. Keep the directory tree intact.
On Ubuntu, install missing desktop tools with `sudo apt install libx11-6 xwayland v4l-utils`.
The camera must support V4L2 YUYV/MJPEG. Check access to `/dev/video*`; do not run as root.
In VMware, connect the camera to Ubuntu through **VM → Removable Devices → camera → Connect (Disconnect from host)**.

## Use

F1 Camera: detect, capture, liveness, verify, enroll and search. F2 Photos: extract/compare/enroll.
F3 Face library: query, edit, import/export. F4 Settings. F5 Image search: up to ten faces with individual matches.
Choose a captured/photo/library reference before verification. Escape cancels a workflow.
Photo search does not check liveness. Import demo 10k contains **synthetic vectors**, for capacity tests only; enroll your own authorized data to test recognition.

## Build

Requires CMake 3.22.1+ and a C++17 compiler. CMake selects the bundled SDK; use `-DHIFACE_SDK_DIR=/path/to/sdk` to override it.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

For CLI commands in HelloFace (ARM64: `runtime=runtimes/arm64`; single-architecture package: `runtime=.`):

```bash
runtime=runtimes/x86_64
"$runtime/bin/hiface_demo" --help
"$runtime/bin/hiface_demo" search-faces --builtin faces.db photo.jpg
# Run the rebuilt CLI with built-in SDK resources:
./build/hiface_demo self-test --builtin
```

SDK integration: [English](runtimes/x86_64/sdk/README.md) · [中文](runtimes/x86_64/sdk/README.zh-CN.md).
Demo source is in `src/`; SDK binaries and public headers are under each runtime's `sdk/`.
Data defaults to `${XDG_DATA_HOME:-$HOME/.local/share}/hiface-demo`; override with `HIFACE_DATA_DIR`.
SQLite, HF1 and captured images are unencrypted; protect and clear application data as needed.
[Support](https://github.com/FaceSDKPro/HelloFace/issues). Keep the SDK license.
