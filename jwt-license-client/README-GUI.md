# AuthClient GUI — Qt6 Desktop Application

Windows 平台下的 JWT 授权客户端桌面程序，基于 Qt6 Widgets 构建。

## 特性

- **双主题支持** — 暗色/亮色一键切换，极致简约的专业风格
- **JWT 授权管理** — 激活、心跳、在线检查、接管、注销
- **RSA-SHA256 签名验证** — 完整的本地 JWT 校验
- **设备指纹** — 基于 CPU/MAC/BIOS/磁盘的机器哈希
- **多线程架构** — 网络操作在工作线程中执行，UI 永不阻塞
- **启动动画** — 300ms 窗口淡入效果
- **定时刷新** — 30 秒自动刷新机器指纹

## 构建要求

- Windows 10+
- Visual Studio 2022 (Community 及以上)
- CMake 3.16+
- **Qt 6.5+** (Widgets + Network 模块)
- 联网（首次构建时自动下载 nlohmann/json）

## Qt 安装

推荐使用 [Qt Online Installer](https://www.qt.io/download-qt-installer) 安装 Qt 6.5+，确保勾选：
- **Qt 6.x.x** (最新稳定版)
- **Qt Charts** (不需要，但 Qt Widgets 是必须的)
- **MSVC 2022 64-bit** 编译器配套工具

或者使用 vcpkg：
```bash
vcpkg install qtbase:x64-windows
```

## 构建步骤

```bash
cd jwt-license-client
rmdir /s /q build 2>nul
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build . --config Release
```

将 `CMAKE_PREFIX_PATH` 替换为你的实际 Qt 安装路径。

生成的可执行文件位于 `build/Release/auth_client_gui.exe`

## 使用说明

1. **启动程序** — 双击 `auth_client_gui.exe`
2. **填写信息** — 输入服务器地址、产品 ID、授权 Key
3. **激活** — 点击 "Activate" 按钮完成授权
4. **管理授权** — 使用下方操作按钮进行心跳、在线检查、接管、注销等操作
5. **切换主题** — 点击右上角 "Dark"/"Light" 按钮

## 界面布局

```
┌─────────────────────────────────────────────────────────────┐
│  License Client                          [Dark]  │  ← 标题栏
├─────────────────────────────────────────────────────────────┤
│  Server: [____]  Product: [____]  Key: [****]  [Activate] │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  License Status                                         │ │
│  │  Status: ● Active          Expires: 2026-12-31        │ │
│  │  Session ID: xxxx-xxxx-xxxx-xxxx                       │ │
│  │  Fingerprint: a1b2c3d4...                              │ │
│  └─────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│  [Local Check] [Online Check] [Heartbeat] [Takeover] [Deact] [Refresh] │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  [12:00:00] License activated successfully.           │ │  ← 日志面板
│  │  [12:00:01] Token: eyJhbGciOiJSUzI1NiIs...            │ │
│  └─────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│  Ready                               v1.0.0                │  ← 状态栏
└─────────────────────────────────────────────────────────────┘
```

## 项目结构

```
jwt-license-client/
├── CMakeLists.txt          # 主 CMake 配置（仅构建 GUI）
├── README.md               # 本文件
├── include/
│   ├── LicenseClient.h     # 核心客户端接口
│   ├── MachineInfo.h       # 设备指纹生成
│   └── Base64.h            # Base64/Hex 编解码
├── src/
│   ├── LicenseClient.cpp   # HTTP + JWT 验证实现
│   ├── MachineInfo.cpp     # 机器信息收集
│   └── Base64.cpp          # Base64 实现
└── gui/
    ├── main.cpp            # Qt GUI 程序入口
    ├── mainwindow.h/cpp    # 主窗口 UI 实现
    ├── licenseworker.h/cpp # 工作线程（网络操作）
    └── themes.qrc          # (内联 QSS，无需单独资源文件)
```

## 技术架构

### 多线程模型
- **主线程 (GUI)** — 负责界面渲染和用户交互
- **工作线程 (LicenseWorker)** — 封装所有 LicenseClient API 调用
- 通过 Qt 信号槽机制进行线程间通信

### 主题系统
- 暗色主题默认启用（背景 `#1e1e1e`，强调色 `#3daee9`）
- 亮色主题（背景 `#f5f5f5`，强调色 `#0078d4`）
- 通过 QSS 样式表动态切换，200ms 过渡效果

### JWT 验证链
1. 从服务器获取 PEM 公钥
2. 解析 PEM → DER → 提取 RSA 模数/指数
3. 构建 PUBLICKEYBLOB 导入 CryptoAPI
4. `CryptVerifySignature` 验证 RS256 签名
5. 校验 claims: issuer / audience / expiration

## 许可证

MIT
