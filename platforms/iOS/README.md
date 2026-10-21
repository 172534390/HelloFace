# HiFace iOS · 0.0.3 Beta

English | [简体中文](README.zh-CN.md)

iOS 15+, Swift 6.

1. Open `HiFaceDemo.xcodeproj` in Xcode and select `HiFaceDemo`.
2. Set your Team and Bundle ID; run on iPhone and grant camera access.

## Integrate

Add `SDK/` as a local Swift package, select `HiFace`, and retain both HiFace and HiFaceNative binary targets.
Use `HiFaceSDK()` with default configuration and add `NSCameraUsageDescription` to your App Info.plist. See the [API guide](SDK/API.md).

Examples: [camera](HiFaceDemo/Camera/CameraPipeline.swift) · [flows](HiFaceDemo/Camera/CameraDemoModel.swift) · [face library](HiFaceDemo/Search/SearchLibraryView.swift).
Demo data: `HiFaceDemo/Resources/FaceSearch/`; image downloads require internet. The 1:1 reference uses Keychain. HF1 and the SDK face database are unencrypted.

Beta expires **2027-02-15 12:00:00 (UTC+08:00)**. The current model does not support face recognition for people under 14 and must not be used in financial scenarios. Validate camera features on iPhone.
Face search requires clear camera images; outdoor deployments require a camera with a wide dynamic range greater than 100 dB.
