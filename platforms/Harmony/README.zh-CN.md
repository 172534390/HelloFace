# HiFace HarmonyOS · 0.0.3 Beta

[English](README.md) | 简体中文

HarmonyOS API 20+，ARM64，DevEco Studio 26。

1. 打开本目录并同步依赖。
2. 配置自己的 Bundle Name、签名和设备 Profile，在手机运行 `demo`。

## 接入

将 `libs/hiface-0.0.3.har` 添加为本地依赖；使用前等待 `whenReady()`。接口见 [SDK 指南](SDK.zh-CN.md)。

示例：[相机](demo/src/main/ets/camera/CameraPreviewController.ets) · [业务流程](demo/src/main/ets/pages/RealtimeCameraDemo.ets) · [人脸库](demo/src/main/ets/search/FaceSearchLibraryDemo.ets)。
`demo/src/main/resources/rawfile/face_search/` 为演示数据；网络图片导入需联网。1:1 基准使用 HUKS 加密保存；HF1 和 SDK 人脸库未加密。

Beta 截止 **2027-02-15 12:00:00（北京时间）**；当前模型不支持14周岁以下儿童人脸识别，不能用于金融相关场景。
人脸搜索识别要求摄像头成像清晰；室外环境下，摄像头宽动态范围须大于 100 dB。
