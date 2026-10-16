# HiFace Windows · 0.0.2 Beta

English | [简体中文](README.zh-CN.md)

Windows 10/11 x64, Intel/AMD CPU. Trial expires **2026-12-24T09:08:42+08:00**.

- [SDK integration](sdk/README.md): public C API, `hiface.dll`, CMake and C/C++ examples.
- [Demo](demo/README.md): executable, application source and AI-generated sample data.

## Run the Demo

Open `demo/`, run `self_test.bat`, then `run_demo.bat` after `RESULT: PASS`.
No development tools or model downloads are needed.

## Build from source

Install Visual Studio 2022 Desktop C++ tools and CMake 3.22.1+, then run here:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./build_demo.ps1
```

Edit `demo/source/`; the rebuilt executable is `build/demo/bin/Release/hiface_demo.exe`.
Validate the GUI and camera on your target device.
Beta use and redistribution are governed by [LICENSE](sdk/LICENSE).
