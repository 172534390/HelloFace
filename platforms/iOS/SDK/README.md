# HiFace iOS SDK 0.0.3

English | [简体中文](README.zh-CN.md)

Requires iOS 15+ and Swift 6. Add this directory as a local Swift Package and select `HiFace`. Retain both HiFace and HiFaceNative binary targets.

The current model does not support face recognition for people under 14 and must not be used in financial scenarios.

Face search requires clear camera images; outdoor deployments require a camera with a wide dynamic range greater than 100 dB.

Initialize with `HiFaceSDK()` and its default model bundle. Add `NSCameraUsageDescription` to your App Info.plist. See the [API guide](API.md) for frames, flows and lifecycle.

Beta expires at **2027-02-15 12:00:00 (UTC+08:00)**.
