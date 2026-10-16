# HiFace Android · 0.0.2 Beta

English | [简体中文](README.zh-CN.md)

Android 6.0+, ARM64/ARMv7. Build with JDK 17 and Android SDK 36 / Build Tools 36.0.0; no NDK required.

## Run

Open this directory in Android Studio and run `demo`, or build:

```sh
./gradlew :demo:assembleDebug
```

APK: `demo/build/outputs/apk/debug/`. Use your own certificate for Release builds.

## Integrate

Copy `maven-repository/` into your project. Follow the [SDK guide](docs/SDK.md) for dependencies, `noCompress` and camera permissions.

Examples: [camera](demo/src/main/java/com/hiface/sdk/demo/camera/CameraPreviewFragment.java) · [search](demo/src/main/java/com/hiface/sdk/demo/search/FaceSearchActivity.java) · [face library](demo/src/main/java/com/hiface/sdk/demo/search/FaceSearchLibraryActivity.java).
Demo data is in `demo/src/main/assets/face_search/`; image downloads require internet. The 1:1 reference uses MMKV: replace the demo key. HF1 and the SDK face database are unencrypted.

Beta expires **2026-12-24 09:08:42 (UTC+08:00)**; recognition of minors is unsupported.
