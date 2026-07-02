<p align="center">
  <img src="resources/logo.png" alt="RacecarTools" width="128">
</p>

<h1 align="center">RacecarTools</h1>
<p align="center">赛车数据分析桌面工具 — 从车载视频中提取速度曲线并进行圈速对比分析</p>

<p align="center">
  <a href="https://github.com/I-Rinka/Racecar-tools-cpp/actions"><img src="https://github.com/I-Rinka/Racecar-tools-cpp/actions/workflows/build-macos.yml/badge.svg" alt="Build macOS"></a>
  <a href="https://github.com/I-Rinka/Racecar-tools-cpp/actions"><img src="https://github.com/I-Rinka/Racecar-tools-cpp/actions/workflows/build-windows.yml/badge.svg" alt="Build Windows"></a>
  <a href="https://github.com/I-Rinka/Racecar-tools-cpp/actions"><img src="https://github.com/I-Rinka/Racecar-tools-cpp/actions/workflows/build-android.yml/badge.svg" alt="Build Android"></a>
</p>

---

C++性能优化版，重构自 [I-Rinka/Racecar-tools](https://github.com/I-Rinka/Racecar-tools)

## 功能

- **视频速度识别** — 拖入车载视频，框选仪表盘速度区域，自动逐帧 OCR 提取速度数据
- **智能 OCR 管线** — Tesseract + 模板匹配 + 数字裁剪，多级兜底识别
- **毛刺自动修复** — 三阶段平滑：中值滤波检测异常帧 → 重新 OCR → 物理加速度约束
- **速度/距离/加速度曲线** — 交互式图表，支持缩放、拖拽、区间选择
- **多圈对比** — 同一图表叠加多条圈速曲线，直观对比不同圈次表现
- **曲线编辑器** — 点击图表跳转数据行，筛选区间，手动修正异常值后实时更新曲线
- **视频同步回放** — 图表悬停时视频自动跳转到对应帧，支持精确帧定位
- **CSV 导入/导出** — 分析结果保存为 CSV，可重新导入继续编辑

## 平台支持

| 平台 | 状态 | 下载 |
|------|------|------|
| macOS | ✅ | [Releases](https://github.com/I-Rinka/Racecar-tools-cpp/releases) |
| Windows | ✅ | [Releases](https://github.com/I-Rinka/Racecar-tools-cpp/releases) |
| Android | ✅ | [Releases](https://github.com/I-Rinka/Racecar-tools-cpp/releases) |

## 快速开始

### 从 Release 下载

前往 [Releases](https://github.com/I-Rinka/Racecar-tools-cpp/releases) 页面下载对应平台的安装包。

### 从源码构建

**依赖：**

- CMake ≥ 3.20
- Qt 6（Widgets, Multimedia, MultimediaWidgets, PrintSupport）
- OpenCV 4（core, imgproc, videoio）
- Tesseract（可选，提供 OCR 能力）

**macOS：**

```bash
brew install qt@6 opencv tesseract cmake
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)
open build/RacecarTools.app
```

**Windows（vcpkg）：**

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```

## 使用方式

1. 启动后切换到「视频解析」标签页
2. 将 `.mp4` 视频文件拖入窗口
3. 按空格暂停播放，用鼠标框选仪表盘上的速度数字区域
4. 按空格继续播放，程序自动逐帧识别速度
5. `Ctrl+S` 保存 — 自动执行毛刺修复并导出 CSV
6. 数据自动加载到「圈速分析」标签页，可叠加多条曲线对比

**快捷键：**

| 按键 | 功能 |
|------|------|
| 空格 | 播放 / 暂停 |
| ← → | 逐帧前进 / 后退 |
| Ctrl+S | 保存并导出 CSV |
| Esc | 清除 ROI 选区 |

## 项目结构

```
src/
├── core/
│   ├── SDAnalyzer        # 圈速数据管理与 CSV 读写
│   ├── VideoWrapper       # OpenCV 视频封装
│   ├── VideoProcessor     # OCR 管线与毛刺修复
│   └── DigitRecognizer    # 模板匹配数字识别
├── widgets/
│   ├── MainWindow         # 主窗口与标签页管理
│   ├── PlotPage           # 圈速分析页面（图表 + 视频 + 编辑器）
│   ├── PlotWidget         # QCustomPlot 交互式图表
│   ├── VideoPlayer        # 视频播放与精确帧定位
│   ├── ROISelector        # 视频 ROI 选择与实时 OCR
│   └── DataEditor         # 数据表格编辑器
└── main.cpp
```

## 许可证

本项目使用 [QCustomPlot](https://www.qcustomplot.com/)（GPL）。
