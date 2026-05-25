# ESP32 C3 E-Paper Display Project

ESP32 C3 墨水屏显示项目，基于 jcalendar 项目架构参考实现。

## 功能特性

- **墨水屏驱动**：基于 GxEPD2 库，支持多种屏幕型号
- **局部刷新**：支持局部刷新，减少闪烁和功耗
- **深度休眠**：超低功耗设计，定时或按键唤醒
- **WiFi 连接**：自动连接和配置
- **NTP 同步**：自动同步网络时间
- **电池监测**：ADC 检测电池电压
- **按键交互**：支持单击、双击、长按

## 硬件要求

### ESP32 C3 开发板
- 芯片：ESP32-C3
- 频率：160MHz
- Flash：4MB+

### 墨水屏（支持以下型号）
- 4.2寸三色屏：GxEPD2_420c / GxEPD2_420c_Z21 / GxEPD2_420c_GDEY042Z98
- 其他尺寸：通过修改 `platformio.ini` 配置

### 接线（默认配置）

| 墨水屏 | ESP32 C3 |
|--------|----------|
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO 4 |
| MOSI | GPIO 5 |
| CS | GPIO 6 |
| DC | GPIO 7 |
| RST | GPIO 8 |
| BUSY | GPIO 9 |

### 其他外设

| 功能 | GPIO |
|------|------|
| 按键 | GPIO 2 |
| LED | GPIO 10 |
| 电池 ADC | GPIO 0 |

## 项目结构

```
.
├── platformio.ini          # PlatformIO 配置
├── partitions.csv          # 分区表
├── src/
│   ├── main.cpp            # 主程序
│   ├── hardware/           # 硬件抽象层
│   │   ├── wiring.h        # 引脚定义
│   │   ├── led.cpp/h       # LED 控制
│   │   ├── button.cpp/h    # 按键处理
│   │   └── battery.cpp/h   # 电池检测
│   ├── display/            # 显示层
│   │   ├── epaper_driver.cpp/h    # 墨水屏驱动封装
│   │   ├── display_manager.cpp/h  # 显示管理器
│   │   └── layouts/        # 布局
│   ├── network/            # 网络层
│   │   ├── wifi_manager.cpp/h     # WiFi 管理
│   │   └── ntp_sync.cpp/h         # NTP 同步
│   └── utils/              # 工具层
│       ├── preferences.cpp/h      # 配置存储
│       └── power_manager.cpp/h    # 电源管理
└── include/                # 头文件
    └── fonts/              # 字体文件
```

## 快速开始

### 1. 安装 PlatformIO

```bash
# 安装 PlatformIO Core
pip install platformio

# 或使用 VS Code 插件
```

### 2. 克隆项目

```bash
cd /Users/wuzhenlong/esp32-e-ink-screen
```

### 3. 配置屏幕型号

编辑 `platformio.ini`，选择你的墨水屏型号：

```ini
build_flags = 
    -D EPD_DRIVER=GxEPD2_420c  ; 修改为你的屏幕型号
```

### 4. 编译上传

```bash
# 编译
pio run

# 上传
pio run --target upload

# 监控串口
pio device monitor
```

## 局部刷新使用说明

### 基本用法

```cpp
// 全屏刷新
displayManager.setFullWindow();
displayManager.firstPage();
do {
    displayManager.fillScreen(COLOR_WHITE);
    // 绘制内容...
} while (displayManager.nextPage());
displayManager.fullRefresh();

// 局部刷新
displayManager.setPartialWindow(x, y, w, h);  // x和w必须是8的倍数
displayManager.firstPage();
do {
    // 绘制局部内容...
} while (displayManager.nextPage());
displayManager.partialRefresh();
```

### 注意事项

1. **8字节对齐**：局部刷新的 x 坐标和宽度必须是 8 的倍数
2. **首次刷新**：建议首次使用全屏刷新，之后可用局部刷新
3. **残影问题**：如果局部刷新出现残影，定期执行一次全屏刷新

## 按键操作

| 操作 | 功能 |
|------|------|
| 单击 | 刷新屏幕 |
| 双击 | 进入配置模式 |
| 长按 | 立即进入休眠 |

## 休眠唤醒

- **定时唤醒**：默认 30 分钟自动唤醒刷新
- **按键唤醒**：按功能键唤醒

## 参考项目

- [jcalendar](jcalendar/)：参考的墨水屏日历项目（已添加到 .gitignore）

## 许可证

MIT License
