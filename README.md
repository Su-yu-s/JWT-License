# JWT License 授权管理系统

基于 JWT（JSON Web Token）的完整软件授权解决方案，包含服务端管理后台和客户端 SDK。

[![Spring Boot](https://img.shields.io/badge/Spring%20Boot-3.2.5-6DB33F?style=for-the-badge)](https://spring.io/projects/spring-boot)
[![Qt6](https://img.shields.io/badge/Qt-6.5+-41CD52?style=for-the-badge)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)](LICENSE)

## 项目简介

JWT License 是一个功能完善的软件授权管理系统，采用前后端分离架构：

| 组件 | 技术栈 | 说明 |
|------|--------|------|
| **服务端** | Spring Boot + MyBatis-Plus + MySQL | 授权管理后台，支持多产品、多设备会话管理 |
| **客户端 (CLI)** | C++17 + WinHTTP + CryptoAPI | 命令行工具，零外部依赖 |
| **客户端 (GUI)** | Qt6 Widgets | 图形界面版本，支持暗色/亮色主题切换 |

## 核心特性

### 服务端

- RS256 JWT 签名，使用 RSA 非对称加密，公钥可安全分发
- 仪表盘统计，实时查看授权分布、激活趋势
- 产品管理，多产品独立配置，每个产品独立密钥对
- 设备会话管理，支持在线踢人、清理超时会话
- 完整审计日志，记录所有授权操作，便于追溯
- SDK 接口，设备激活、心跳、在线检查无需管理员鉴权

### 客户端 (C++ SDK)

- 零外部依赖，仅使用 Windows 内置 API（WinHTTP + CryptoAPI）
- 完整 JWT 验证，本地验证签名、issuer、audience、expiration
- 设备指纹，基于 CPU/MAC/BIOS/磁盘生成唯一机器哈希
- 心跳刷新，定期刷新 JWT 令牌，延长授权有效期
- 离线检查，本地验证已存储的 JWT，无需联网
- SDK 协议，Activate / Heartbeat / OnlineCheck / Takeover / Deactivate

### 客户端 (Qt6 GUI)

- 双主题，暗色/亮色一键切换，专业级 UI 设计
- 多线程，网络操作在工作线程，UI 永不阻塞
- 启动动画，300ms 窗口淡入效果
- 实时日志，操作日志面板，清晰展示每一步状态

## 项目结构

```
JWT/
├── jwt-license-server/          # Spring Boot 服务端
│   ├── src/main/java/
│   │   └── com/example/license/
│   │       ├── config/          # 配置类
│   │       ├── common/          # 公共层
│   │       ├── interceptor/     # 拦截器
│   │       └── module/
│   │           ├── auth/        # 认证模块
│   │           ├── dashboard/   # 仪表盘模块
│   │           ├── product/     # 产品模块
│   │           ├── licensekey/  # 授权 Key 模块
│   │           ├── session/     # 设备会话模块
│   │           ├── audit/       # 审计日志模块
│   │           └── sdk/         # SDK 客户端接口
│   └── src/main/resources/
│       ├── application.yml      # 应用配置
│       ├── db/init.sql          # 数据库初始化
│       └── static/              # 前端管理界面
│
└── jwt-license-client/          # C++ 客户端
    ├── include/                 # 公共头文件
    │   ├── LicenseClient.h
    │   ├── MachineInfo.h
    │   └── Base64.h
    ├── src/                     # CLI 实现
    │   ├── main.cpp
    │   ├── LicenseClient.cpp
    │   ├── MachineInfo.cpp
    │   └── Base64.cpp
    ├── gui/                     # Qt6 GUI 实现
    │   ├── mainwindow.h/cpp
    │   ├── licenseworker.h/cpp
    │   └── ...
    ├── resources/               # 图标资源
    └── CMakeLists.txt           # 构建配置
```

## 快速开始

### 服务端部署

#### 1. 初始化数据库

```bash
mysql -u root -p < jwt-license-server/src/main/resources/db/init.sql
```

#### 2. 修改配置

编辑 `jwt-license-server/src/main/resources/application.yml`：

```yaml
spring:
  datasource:
    url: jdbc:mysql://localhost:3306/jwt_license?useSSL=false&serverTimezone=UTC
    username: root
    password: your_password
```

#### 3. 启动服务

```bash
cd jwt-license-server
mvn spring-boot:run
```

服务默认运行在 `http://localhost:8080`

#### 4. 管理员登录

默认账号：`admin@example.com` / `admin123`

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin@example.com","password":"admin123"}'
```

### 客户端构建

#### 构建要求

- Windows 10+
- Visual Studio 2022 (Community 及以上)
- CMake 3.16+
- Qt 6.5+ (仅 GUI 版本需要)

#### 构建 CLI 版本

```bash
cd jwt-license-client
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

#### 构建 GUI 版本

```bash
cd jwt-license-client
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build . --config Release
```

#### 使用 CLI 客户端

```bash
# 帮助
jwt_license_client.exe --help

# 激活设备
jwt_license_client.exe --server http://localhost:8080 --keyCode DEMO-XXXX-XXXX-XXXX-XXXX

# 交互式菜单
jwt_license_client.exe --server http://localhost:8080
```

## API 接口

| 模块 | 路径 | 说明 |
|------|------|------|
| 认证 | `/api/auth/login` | 管理员登录 |
| 仪表盘 | `/api/dashboard/stats` | 统计数据 |
| 产品管理 | `/api/products/*` | 产品 CRUD |
| 授权 Key | `/api/license-keys/*` | Key 生成与管理 |
| 设备会话 | `/api/sessions/*` | 会话管理、踢下线 |
| 审计日志 | `/api/audit-logs/*` | 操作记录查询 |
| SDK 接口 | `/api/sdk/*` | 设备激活、心跳、验证 |

## 授权流程

```
[Client]     1. 激活请求     [Server]
   |                              |
   |                              |
   |   4. 存储 JWT <-------------|
   |     (返回 JWT + 公钥)
   |
   |   2. 心跳请求 (定期)
   | ---------------------------->
                 3. 刷新 JWT
```

## 技术细节

### JWT 验证链 (客户端)

1. 从服务器获取 PEM 公钥
2. 解析 PEM → DER → 提取 RSA 模数/指数
3. 构建 PUBLICKEYBLOB 导入 CryptoAPI
4. `CryptVerifySignature` 验证 RS256 签名
5. 校验 claims：issuer / audience / expiration

### 设备指纹生成

基于以下硬件信息的 SHA-256 哈希：
- CPU ID（Processor Info + Brand String）
- 所有网卡的 MAC 地址
- BIOS 序列号
- 计算机主机名
- 第一块物理硬盘的序列号

### 安全机制

- RSA-2048 密钥对，每个产品独立密钥对
- JWT RS256 签名，服务端私钥签名，客户端用公钥验证
- Device Binding，JWT 绑定设备指纹，防止令牌盗用
- 完整审计，所有授权操作记录审计日志

## 文档

- [服务端 README](jwt-license-server/README.md)
- [客户端 README](jwt-license-client/README.md)
- [GUI 版本说明](jwt-license-client/README-GUI.md)

## 许可证

MIT License
