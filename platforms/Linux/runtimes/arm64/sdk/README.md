# HiFace Linux SDK

English | [简体中文](README.zh-CN.md)

**0.0.3 Beta** · x86_64 / ARM64 · CPU · C ABI v1.
Ubuntu 22.04 / glibc 2.35 baseline; Ubuntu 24.04 is a compatibility target.
Beta expires **2027-02-15 12:00:00 (UTC+08:00)**. The current model does not support face recognition for people under 14 and must not be used in financial scenarios. Target-device acceptance is pending.
Face search requires clear camera images; outdoor deployments require a camera with a wide dynamic range greater than 100 dB.

## Integrate

Use the SDK matching your architecture. `include/hiface/` contains public declarations, `lib/` the shared SDK,
and model resources are protected inside the shared library. No standalone model files are distributed.

```cmake
cmake_minimum_required(VERSION 3.22.1)
project(MyApp LANGUAGES CXX)
find_package(HiFace CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE HiFace::HiFace)
```

Configure with `-DCMAKE_PREFIX_PATH=/absolute/path/to/sdk`. Deploy `libhiface.so.1` and its target library with your application; configure RPATH/library search paths. Keep library symlinks intact.

## Run the C example

Download the [Demo example](https://github.com/FaceSDKPro/HelloFace/tree/main/platforms/Linux/examples/image-search). From this SDK directory, with CMake 3.22.1+ and a C compiler:

```bash
sha256sum -c FILES.sha256
cmake -S /path/to/HelloFace/platforms/Linux/examples/image-search -B build -DCMAKE_PREFIX_PATH="$PWD"
cmake --build build --parallel 2
./build/hiface_sdk_image_search --builtin :memory: --blank 256 256
```

[image_search.c](https://github.com/FaceSDKPro/HelloFace/blob/main/platforms/Linux/examples/image-search/image_search.c) initializes the SDK, captures up to ten faces, queries the database and releases resources.
Replace `:memory: --blank 256 256` with `/path/to/faces.db image.rgba WIDTH HEIGHT` to search your own pixels.
Input must be orientation-normalized, tightly packed RGBA8888: even dimensions 2..4096, exactly `WIDTH*HEIGHT*4` bytes.
JPEG/PNG decoding and camera adapters are demonstrated in the separate Demo.

## API essentials

- Set every `struct_size` and, where present, `api_version`. Check returned status and result-validity flags.
- Enable feature/landmark in the config. Configure runtime → create Session → `hiface_linux_load_builtin_models_v1` → process → destroy.
  The Linux initialization declaration is in `include/hiface/hiface_linux.h`.
  Calls are synchronous; serialize Session work and wait for in-flight calls before destruction. Input buffers remain valid until return.
- Capture returns 512-float features and optional aligned previews. Static images do not perform liveness checks.
  Multi-face capture has a separate status per face. Copy borrowed result data before the next call/result destruction.
- Camera input uses `process_camera_yuv420` with real plane strides, even dimensions up to 1920×1080,
  supported rotation and one monotonic millisecond clock. Feed frames until a started flow reaches a terminal outcome, then stop it.
- The search database supports CRUD and Top-K; check `HIFACE_FEATURE_SPACE_ID`. HF1 and SQLite are unencrypted.
  Similarity is not identity probability; a spoof-rejected result must not authorize access.

Full signatures and contracts: `include/hiface/hiface_c_api.h`.
Keep the license and dependency notices. For usage/redistribution authorization or support, [contact the author](https://github.com/FaceSDKPro/HelloFace/issues).
