# HiFace iOS SDK 0.0.3

[English](README.md) | 简体中文

最低 iOS 15，使用 Swift 6。将本目录添加为本地 Swift Package，选择 `HiFace`，保留 HiFace 和 HiFaceNative 两个二进制目标。

当前模型不支持12周岁以下儿童人脸识别，不能用于金融相关场景。

人脸搜索识别要求摄像头成像清晰；室外环境下，摄像头宽动态范围须大于 100 dB。

使用 `HiFaceSDK()` 默认模型配置，在 App Info.plist 添加 `NSCameraUsageDescription`。帧、流程和生命周期要求见 [API 指南](API.zh-CN.md)。

SDK正在Beta 迭代公测中，请及时升级SDK版本。
