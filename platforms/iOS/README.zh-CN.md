# HiFace iOS · 0.0.3 Beta

[English](README.md) | 简体中文

iOS 15+，Swift 6。

1. Xcode 打开 `HiFaceDemo.xcodeproj`，选择 `HiFaceDemo`。
2. 配置自己的 Team 和 Bundle ID，在 iPhone 运行并允许相机权限。

## 接入

将 `SDK/` 添加为本地 Swift Package，选择 `HiFace`，保留 HiFace 和 HiFaceNative 两个二进制目标。
使用 `HiFaceSDK()` 默认配置，在 App Info.plist 添加 `NSCameraUsageDescription`。接口见 [API 指南](SDK/API.zh-CN.md)。

示例：[相机](HiFaceDemo/Camera/CameraPipeline.swift) · [业务流程](HiFaceDemo/Camera/CameraDemoModel.swift) · [人脸库](HiFaceDemo/Search/SearchLibraryView.swift)。
`HiFaceDemo/Resources/FaceSearch/` 为演示数据；网络图片导入需联网。1:1 基准使用 Keychain；HF1 和 SDK 人脸库未加密。

Beta 截止 **2027-02-15 12:00:00（北京时间）**；当前模型不支持14周岁以下儿童人脸识别，不能用于金融相关场景。相机功能请在 iPhone 验证。
人脸搜索识别要求摄像头成像清晰；室外环境下，摄像头宽动态范围须大于 100 dB。
