package com.example.license.config;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Data
@Component
@ConfigurationProperties(prefix = "jwt")
public class JwtConfig {

    // 管理员 JWT 签名密钥
    private String adminSecretKey = "jwt-license-admin-secret-key-2024-should-be-long-and-secure";

    // 授权 Key 加密主密钥（AES-256-GCM）
    private String keyEncryptionSecret = "jwt-license-key-encryption-secret-change-me";

    // 管理员 Token 过期时间（秒）
    private int adminExpiration = 86400;

    // SDK JWT 默认过期时间（秒）
    private int sdkDefaultExpiration = 15552000;

    // Seat 超时时间（分钟），超过此时间无心跳视为离线
    private int seatTimeoutMinutes = 5;
}
