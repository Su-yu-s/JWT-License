package com.example.license.util;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.ExpiredJwtException;
import io.jsonwebtoken.JwtException;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.security.Keys;

import javax.crypto.SecretKey;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Date;

/**
 * 管理员 JWT 工具类
 * 使用 HMAC-SHA256 签发/验证管理后台登录 Token
 */
public class AdminJwtUtil {

    /**
     * 签发管理员 Token
     */
    public static String generateAdminToken(String secretKey, String username, long expirationSeconds) {
        long nowMs = System.currentTimeMillis();
        SecretKey key = Keys.hmacShaKeyFor(secretKey.getBytes(StandardCharsets.UTF_8));
        return Jwts.builder()
                .subject(username)
                .claim("type", "admin")
                .issuedAt(new Date(nowMs))
                .expiration(Date.from(Instant.now().plusSeconds(expirationSeconds)))
                .signWith(key)
                .compact();
    }

    /**
     * 验证并解析管理员 Token
     */
    public static Claims parseAdminToken(String secretKey, String token) {
        SecretKey key = Keys.hmacShaKeyFor(secretKey.getBytes(StandardCharsets.UTF_8));
        try {
            return Jwts.parser()
                    .verifyWith(key)
                    .build()
                    .parseSignedClaims(token)
                    .getPayload();
        } catch (ExpiredJwtException e) {
            // Token 过期但仍可提取 claims（用于判断过期时间）
            return e.getClaims();
        } catch (JwtException | IllegalArgumentException e) {
            // Token 格式错误、签名无效、密钥不匹配等
            return null;
        }
    }

    /**
     * 检查管理员 Token 是否过期
     */
    public static boolean isAdminTokenExpired(String secretKey, String token) {
        Claims claims = parseAdminToken(secretKey, token);
        if (claims == null || claims.getExpiration() == null) {
            return true;
        }
        return claims.getExpiration().before(new Date());
    }

    /**
     * 获取管理员用户名
     */
    public static String getUsername(String secretKey, String token) {
        Claims claims = parseAdminToken(secretKey, token);
        if (claims == null) {
            return null;
        }
        return claims.getSubject();
    }
}
