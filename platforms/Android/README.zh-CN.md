# HiFace Android · 0.0.3 Beta

[English](README.md) | 简体中文

Android 6.0+，ARM64/ARMv7。使用 JDK 17、Android SDK 36 / Build Tools 36.0.0；构建 Demo 无需 NDK。

## 运行

Android Studio 打开本目录，运行 `demo`，或执行：

```sh
./gradlew :demo:assembleDebug
```

APK 位于 `demo/build/outputs/apk/debug/`。Release 使用自己的签名证书。

## 接入

复制 `maven-repository/` 到自己的工程，按 [SDK 指南](docs/SDK.zh-CN.md) 配置依赖、`noCompress` 和相机权限。

示例：[相机](demo/src/main/java/com/hiface/sdk/demo/camera/CameraPreviewFragment.java) · [搜索](demo/src/main/java/com/hiface/sdk/demo/search/FaceSearchActivity.java) · [人脸库](demo/src/main/java/com/hiface/sdk/demo/search/FaceSearchLibraryActivity.java)。
`demo/src/main/assets/face_search/` 为演示数据；网络图片导入需联网。1:1 基准使用 MMKV，业务接入时替换演示密钥；HF1 和 SDK 人脸库未加密。

Beta 截止 **2027-02-15 12:00:00（北京时间）**；当前模型不支持14周岁以下儿童人脸识别，不能用于金融相关场景。
人脸搜索识别要求摄像头成像清晰；室外环境下，摄像头宽动态范围须大于 100 dB。
