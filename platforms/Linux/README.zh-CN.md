# HiFace Linux Demo

[English](README.md) | 简体中文

SDK **Beta**，x86_64 / ARM64，CPU。Ubuntu 22.04+（glibc 2.35+）；桌面需要 X11 或 XWayland。
SDK正在Beta 迭代公测中，请及时升级SDK版本。当前模型不支持12周岁以下儿童人脸识别，不能用于金融相关场景。目标设备验收待完成。
人脸搜索识别要求摄像头成像清晰；室外环境下，摄像头宽动态范围须大于 100 dB。

## 运行

在 HelloFace 的 `platforms/Linux` 或解压后的 Demo 目录执行：

```bash
./self-test.sh
./run-demo.sh
# 指定其他摄像头：
./run-demo.sh /dev/video2
```

HelloFace 启动脚本自动选择 `runtimes/x86_64` 或 `runtimes/arm64`，请保留完整目录结构。
Ubuntu 缺少桌面工具时执行 `sudo apt install libx11-6 xwayland v4l-utils`。
摄像头需支持 V4L2 YUYV/MJPEG；检查 `/dev/video*` 访问权限，无需 root 运行。
VMware 中通过 **虚拟机 → 可移动设备 → 摄像头 → 连接（断开与主机的连接）** 将摄像头交给 Ubuntu。

## 使用

F1 相机：检测、抓拍、活体、验证、录入与搜索。F2 图片：提取、比对和录入。
F3 人脸库：查询、修改、导入导出。F4 设置。F5 图片搜索：最多十张脸，逐脸显示匹配结果。
验证前先选抓拍、图片或库内参考；Esc 取消流程。
图片搜索不做活体检测。Import demo 10k 为**合成特征**，仅用于容量测试；验证识别效果请自行录入获授权的数据。

## 编译

需要 CMake 3.22.1+ 和 C++17 编译器。CMake 自动选择 SDK，也可传 `-DHIFACE_SDK_DIR=/path/to/sdk`。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

HelloFace 中的 CLI 用法（ARM64 改为 `runtime=runtimes/arm64`；单架构压缩包用 `runtime=.`）：

```bash
runtime=runtimes/x86_64
"$runtime/bin/hiface_demo" --help
"$runtime/bin/hiface_demo" search-faces --builtin faces.db photo.jpg
# 新编译的 CLI 直接初始化 SDK：
./build/hiface_demo self-test --builtin
```

SDK 接入：[English](runtimes/x86_64/sdk/README.md) · [中文](runtimes/x86_64/sdk/README.zh-CN.md)。
`src/` 是 Demo 源码；各架构的 `sdk/` 包含 SDK 二进制和公共头。
数据默认位于 `${XDG_DATA_HOME:-$HOME/.local/share}/hiface-demo`，可通过 `HIFACE_DATA_DIR` 修改。
SQLite、HF1 和抓拍图片未加密，请按业务要求保护和清理数据。
[问题反馈](https://github.com/FaceSDKPro/HelloFace/issues)。保留 SDK 许可。
