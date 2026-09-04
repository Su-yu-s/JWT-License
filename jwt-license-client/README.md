# JWT License Client (C++)

Windows 平台下的 JWT 授权客户端 SDK，用于验证和管理软件许可证。

## 特性

- **零外部依赖** — 仅使用 Windows 内置 API（WinHTTP + CryptoAPI）
- **RSA-SHA256 JWT 验证** — 完整的签名验证和 claims 校验
- **设备指纹** — 基于 CPU/MAC/BIOS/磁盘的机器哈希生成
- **完整的 SDK 协议** — 激活、心跳、在线检查、接管、注销
- **JWKS 支持** — 获取所有产品的 RSA 公钥

## 构建要求

- Windows 10+
- Visual Studio 2022 (Community 及以上)
- CMake 3.16+
- 联网（首次构建时自动下载 nlohmann/json）

## 构建步骤

```bash
cd jwt-license-client
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

生成的可执行文件位于 `build/Release/jwt_license_client.exe`

## 命令行用法

```bash
# 帮助
jwt_license_client.exe --help

# 快速激活（一行命令）
jwt_license_client.exe --server http://localhost:8080 --keyCode DEMO-XXXX-XXXX-XXXX-XXXX

# 指定服务器
jwt_license_client.exe -s http://192.168.1.100:8080
```

## 交互模式

直接运行进入交互式菜单：

```
jwt_license_client.exe
```

可用命令：
1. **Activate** — 使用授权码激活设备
2. **Heartbeat** — 发送心跳，刷新 JWT 令牌
3. **OnlineCheck** — 在线检查（不刷新令牌）
4. **Takeover** — 从其他设备接管授权
5. **Deactivate** — 注销/释放授权
6. **Jwks** — 获取所有产品的公钥
7. **DecodeJwt** — 解码 JWT（不验证）
8. **MachineInfo** — 查看本机硬件信息
9. **ChangeServer** — 更改服务器地址

## API 集成

### 激活流程

```cpp
LicenseClient client("http://localhost:8080");
client.setExpectedProductId("demo_app-v1");

// 生成设备指纹
std::string machineHash = generateMachineHash();

// 激活
ActivateResponse resp = client.activate("DEMO-XXXX-XXXX-XXXX-XXXX", machineHash);

if (resp.success) {
    // 验证 JWT 签名和 claims
    JwtToken token = client.verifyJwt(resp.token, resp.publicKeyPem);
    
    // 检查是否过期
    if (!client.isTokenExpired(token)) {
        std::cout << "授权有效！剩余 " 
                  << std::difftime(token.expiresAt, std::time(nullptr)) / 3600.0 
                  << " 小时\n";
    }
}
```

### 心跳刷新

```cpp
HeartbeatResponse hb = client.heartbeat("DEMO-XXXX-XXXX-XXXX-XXXX");
if (hb.success) {
    JwtToken token = client.verifyJwt(hb.token, hb.publicKeyPem);
}
```

### 在线检查

```cpp
OnlineCheckResponse oc = client.onlineCheck("DEMO-XXXX-XXXX-XXXX-XXXX");
```

### 注销

```cpp
DeactivateResponse dr = client.deactivate("DEMO-XXXX-XXXX-XXXX-XXXX");
```

## 项目结构

```
jwt-license-client/
├── CMakeLists.txt          # CMake 构建配置
├── include/
│   ├── LicenseClient.h     # 核心客户端接口
│   ├── MachineInfo.h       # 设备指纹生成
│   └── Base64.h            # Base64/Hex 编解码
└── src/
    ├── main.cpp            # CLI 程序入口
    ├── LicenseClient.cpp   # HTTP + JWT 验证实现
    ├── MachineInfo.cpp     # 机器信息收集
    └── Base64.cpp          # Base64 实现
```

## 技术细节

### JWT 验证

- 算法：RS256 (RSA + SHA-256)
- 签名验证：Windows CryptoAPI `CryptVerifySignature`
- Claims 校验：issuer、audience、expiration
- 公钥来源：激活响应中的 `publicKeyPem` 或 JWKS 端点

### 设备指纹

机器哈希基于以下硬件信息的 SHA-256：
- CPU ID（Processor Info + Brand String）
- 所有网卡的 MAC 地址
- BIOS 序列号
- 计算机主机名
- 第一块物理硬盘的序列号

### HTTP 通信

- 使用 Windows WinHTTP API
- 支持 HTTP 和 HTTPS
- 自动处理代理设置（系统默认）

## 许可证

MIT
