# HiFace Windows · 0.0.3 Beta

English | [简体中文](README.zh-CN.md)

Windows 10/11 x64, Intel/AMD CPU. The SDK is in iterative public Beta testing. Please keep the SDK version up to date.
The current model does not support face recognition for people under 12 and must not be used in financial scenarios. Face search requires clear camera images; outdoor deployments require a camera with a wide dynamic range greater than 100 dB.

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
