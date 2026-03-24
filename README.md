# 虚拟鼠标 (Virtual Mouse)

一个使用摄像头识别手部动作来控制鼠标的应用程序，支持移动、点击和拖动操作。

## 功能特点

- **手部检测**：使用MediaPipe库实时检测手部关键点
- **鼠标控制**：通过食指位置控制鼠标光标
- **点击操作**：食指和拇指捏合时执行鼠标左键点击
- **拖动操作**：大拇指和中指捏合时执行拖动操作
- **平滑处理**：对手部关键点和鼠标移动进行平滑过滤，减少抖动
- **坐标映射**：将操作区域内的移动映射到整个屏幕
- **可视化界面**：实时显示手部关键点和骨骼连接

## 技术实现

- **Python版本**：使用OpenCV、MediaPipe和PyAutoGUI库
- **C++版本**：使用OpenCV、MediaPipe和Windows API

## 安装要求

### Python版本

1. Python 3.7+
2. 依赖库：
   - opencv-python
   - mediapipe
   - pyautogui
   - pywin32 (Windows系统)

### C++版本

1. C++11+
2. 依赖库：
   - OpenCV
   - MediaPipe
   - Windows SDK

## 安装步骤

### Python版本

```bash
# 安装依赖
pip install opencv-python mediapipe pyautogui pywin32 -i https://pypi.tuna.tsinghua.edu.cn/simple

# 下载手部检测模型
# 从 https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task 下载模型文件
# 并将其放在项目根目录
```

### C++版本

1. 安装OpenCV和MediaPipe库
2. 配置Visual Studio项目
3. 链接必要的库文件

## 使用方法

### Python版本

```bash
python virtual_mouse.py
```

### C++版本

1. 编译项目
2. 运行可执行文件

## 操作说明

1. **鼠标移动**：将食指移动到窗口中心的白色线框区域内
2. **点击操作**：食指和拇指捏合（靠近）
3. **拖动操作**：大拇指和中指捏合（靠近）
4. **退出程序**：按退格键

## 性能优化

- 摄像头分辨率设置为640x480，提高处理速度
- 窗口大小为屏幕的三分之一，减少绘制开销
- 应用非线性映射算法，提高中心区域精度
- 使用加权平均平滑鼠标移动
- 对手部关键点进行多帧平滑，减少抖动

## 注意事项

- 确保摄像头能够捕捉到您的手部
- 在光线充足的环境下使用效果更好
- 保持手部在摄像头的视野范围内
- 第一次运行时可能会有一些MediaPipe的警告信息，这是正常的

## 项目结构

```
虚拟鼠标/
├── virtual_mouse.py    # Python版本
├── virtual_mouse.cpp   # C++版本
├── hand_landmarker.task # 手部检测模型
├── README.md           # 项目说明
└── .gitignore          # Git忽略文件
```

## 许可证

MIT
