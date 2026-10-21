# HiFace Windows Demo

支持 Windows 10/11 x64。先运行 `self_test.bat`，看到 `RESULT: PASS` 后运行 `run_demo.bat`。
无需开发工具或下载模型。

## 使用

左侧切换检测、采集、1:1、活体、1:N/M:N、图片搜索、图片比对和人脸库。
相机流程先选择并打开摄像头；1:1 需先采集基准，搜索需先录入人员。
参数修改后点击“应用修改”，进行中的流程可取消。

- 人脸库“导入 JSON”可导入 `data/features_10000.json` 的 1 万条 AI 合成人脸特征；相同 ID 会覆盖。
- `data/images_1000.json` 是 1000 张 AI 合成人脸图片的公开 URL 清单，供下载参考；图片使用本地多选导入，不要将此文件作为特征 JSON 导入。
- 图片搜索：选照片查看每张人脸的 ID、分数和对齐图；默认全库 Top-1 / 0.80，不执行活体。
  `Ctrl+O` 选图、`F5` 重搜、`Esc` 取消，左右键选择人脸。

数据位于 `%LOCALAPPDATA%\HiFaceDemo`：`faces.db` 为人脸库，`verification.hf1` 为 1:1 基准，`logs/` 为日志与自检报告。
相机打不开时检查 Windows 桌面相机权限、设备占用和 Media Foundation；缺少 `hiface.dll` 时重新完整解压。
Beta 截止 **2027-02-15 12:00:00（北京时间）**；界面与相机请在目标设备验证。
当前模型不支持14周岁以下儿童人脸识别，不能用于金融相关场景。人脸搜索识别要求摄像头成像清晰；室外环境下，摄像头宽动态范围须大于 100 dB。

## 修改与编译

使用完整 SDK/Demo 包时，在平台根目录运行 `build_demo.ps1`。
单独的 Demo 包需另备 SDK、Visual Studio 2022 C++ 桌面工具和 CMake 3.22.1+，在本 Demo 目录执行：

```powershell
cmake -S source -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/HiFace/sdk
cmake --build build --config Release
./build/bin/Release/hiface_demo.exe
```

`source/` 包含界面、相机和业务流程；`source/integration/` 是调用 SDK 公共 C API 的 C++ 示例。
