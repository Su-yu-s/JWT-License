# JWT 授权管理系统

基于 Spring Boot + MyBatis-Plus 的 JWT 授权管理后台服务端。

## 技术栈

- Spring Boot 3.2.5
- MyBatis-Plus 3.5.5
- MySQL 8.0+
- JJWT 0.12.5（RS256 签名）
- Lombok

## 快速开始

### 1. 初始化数据库

```bash
mysql -u root -p < jwt-license-server/src/main/resources/db/init.sql
```

### 2. 修改配置

编辑 `src/main/resources/application.yml`，修改数据库连接信息：

```yaml
spring:
  datasource:
    url: jdbc:mysql://localhost:3306/jwt_license?...
    username: root
    password: your_password
```

### 3. 启动项目

```bash
cd jwt-license-server
mvn spring-boot:run
```

服务启动在 `http://localhost:8080`

### 4. 管理员登录

默认管理员账号：`admin@example.com` / `admin123`

> 注意：init.sql 中的 BCrypt 密码是占位值，首次运行需手动更新。

```bash
# 登录后获取 Token
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin@example.com","password":"admin123"}'

# 使用 Token 调用其他接口
curl http://localhost:8080/api/dashboard/stats \
  -H "Authorization: Bearer <your-token>"
```

## 接口文档

| 模块 | 前缀 | 说明 |
|------|------|------|
| 认证 | `/api/auth/*` | 登录 |
| 仪表盘 | `/api/dashboard/*` | 统计概览、最近审计 |
| 产品管理 | `/api/products/*` | 产品 CRUD |
| 授权 Key | `/api/license-keys/*` | Key 生成、管理 |
| 设备会话 | `/api/sessions/*` | 会话列表、踢下线、清理超时 |
| 审计日志 | `/api/audit-logs/*` | 审计记录查询 |
| SDK 接口 | `/api/sdk/*` | 设备激活、心跳、验证（无需管理员 Token） |

## 项目结构

```
jwt-license-server/
├── src/main/java/com/example/license/
│   ├── config/          # 配置类（MyBatis-Plus、CORS、JWT）
│   ├── common/          # 公共层（Result、异常、枚举）
│   ├── interceptor/     # 拦截器（管理员鉴权）
│   ├── module/
│   │   ├── auth/        # 认证模块
│   │   ├── dashboard/   # 仪表盘模块
│   │   ├── product/     # 产品模块
│   │   ├── licensekey/  # 授权 Key 模块
│   │   ├── session/     # 设备会话模块
│   │   ├── audit/       # 审计日志模块
│   │   ├── sdk/         # SDK 客户端接口
│   │   └── tenant/      # 租户模块
│   └── util/            # 工具类（JWT、RSA、Key 生成）
└── src/main/resources/
    ├── application.yml
    └── db/init.sql      # 数据库初始化脚本
```
