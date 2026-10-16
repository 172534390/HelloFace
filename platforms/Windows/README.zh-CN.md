# HiFace Windows · 0.0.2 Beta

[English](README.md) | 简体中文

支持 Windows 10/11 x64、Intel/AMD CPU。试用截止 **2026-12-24T09:08:42+08:00**。

- [SDK 接入](sdk/README.zh-CN.md)：公共 C API、`hiface.dll`、CMake 和 C/C++ 示例。
- [Demo 使用](demo/README.md)：可执行程序、应用源码和 AI 合成人脸演示数据。

## 运行 Demo

进入 `demo/`，先运行 `self_test.bat`，看到 `RESULT: PASS` 后运行 `run_demo.bat`。
无需安装开发工具或下载模型。

## 编译 Demo

安装 Visual Studio 2022 的 C++ 桌面工具和 CMake 3.22.1+，在本目录执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./build_demo.ps1
```

修改 `demo/source/` 后重新编译，程序位于 `build/demo/bin/Release/hiface_demo.exe`。
分发条款见 [LICENSE](sdk/LICENSE)；界面与相机请在目标设备验证。
