# HiFace HarmonyOS · 0.0.3 Beta

English | [简体中文](README.zh-CN.md)

HarmonyOS API 20+, ARM64, DevEco Studio 26.

1. Open this directory and sync dependencies.
2. Configure your Bundle Name, signing and device profile; run `demo` on a phone.

## Integrate

Add `libs/hiface-0.0.3.har` as a local dependency. Await `whenReady()` before use. See the [SDK guide](SDK.md).

Examples: [camera](demo/src/main/ets/camera/CameraPreviewController.ets) · [flows](demo/src/main/ets/pages/RealtimeCameraDemo.ets) · [face library](demo/src/main/ets/search/FaceSearchLibraryDemo.ets).
Demo data: `demo/src/main/resources/rawfile/face_search/`; image downloads require internet. The 1:1 reference uses HUKS encryption. HF1 and the SDK face database are unencrypted.

Beta expires **2027-02-15 12:00:00 (UTC+08:00)**. The current model does not support face recognition for people under 14 and must not be used in financial scenarios.
Face search requires clear camera images; outdoor deployments require a camera with a wide dynamic range greater than 100 dB.
