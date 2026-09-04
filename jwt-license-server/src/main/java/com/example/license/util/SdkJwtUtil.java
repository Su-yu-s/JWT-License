package com.example.license.util;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jwts;

import java.security.*;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.time.Instant;
import java.util.Base64;
import java.util.Date;
import java.util.Map;
import java.util.UUID;

/**
 * SDK JWT 工具类（RS256）
 * 使用产品的 RSA 私钥签发 JWT，公钥用于验证
 * JWT payload 包含：iss/aud/sub/jti/iat/nbf/exp/tenant_id/product_id/machine_hash/session_id
 */
public class SdkJwtUtil {

    /**
     * 使用产品 RSA 私钥签发完整 JWT
     */
    public static String sign(
            String privateKeyPem,
            String issuer,
            String audience,
            String licenseKeyId,
            String tenantId,
            String productId,
            String machineHash,
            String sessionId,
            long expirationSeconds) {
        try {
            PrivateKey key = loadPrivateKey(privateKeyPem);
            long nowMs = System.currentTimeMillis();

            return Jwts.builder()
                    .subject(licenseKeyId)
                    .issuer(issuer)
                    .audience().add(audience).and()
                    .id(UUID.randomUUID().toString().replace("-", ""))
                    .issuedAt(new Date(nowMs))
                    .expiration(Date.from(Instant.now().plusSeconds(expirationSeconds)))
                    .claim("tenant_id", tenantId)
                    .claim("product_id", productId)
                    .claim("machine_hash", machineHash)
                    .claim("session_id", sessionId)
                    .signWith(key, Jwts.SIG.RS256)
                    .compact();
        } catch (Exception e) {
            throw new RuntimeException("签发 SDK JWT 失败: " + e.getMessage(), e);
        }
    }

    /**
     * 使用产品 RSA 公钥验证 JWT
     */
    public static Claims verify(String publicKeyPem, String token) {
        try {
            PublicKey key = loadPublicKey(publicKeyPem);
            return Jwts.parser()
                    .verifyWith(key)
                    .build()
                    .parseSignedClaims(token)
                    .getPayload();
        } catch (Exception e) {
            throw new RuntimeException("验证 SDK JWT 失败: " + e.getMessage(), e);
        }
    }

    /**
     * 从 PEM 字符串加载私钥
     */
    private static PrivateKey loadPrivateKey(String pem) throws Exception {
        String base64 = pem
                .replace("-----BEGIN PRIVATE KEY-----", "")
                .replace("-----END PRIVATE KEY-----", "")
                .replaceAll("\\s", "");
        byte[] decoded = Base64.getDecoder().decode(base64);
        KeyFactory kf = KeyFactory.getInstance("RSA");
        return kf.generatePrivate(new PKCS8EncodedKeySpec(decoded));
    }

    /**
     * 从 PEM 字符串加载公钥
     */
    private static PublicKey loadPublicKey(String pem) throws Exception {
        String base64 = pem
                .replace("-----BEGIN PUBLIC KEY-----", "")
                .replace("-----END PUBLIC KEY-----", "")
                .replaceAll("\\s", "");
        byte[] decoded = Base64.getDecoder().decode(base64);
        KeyFactory kf = KeyFactory.getInstance("RSA");
        return kf.generatePublic(new X509EncodedKeySpec(decoded));
    }
}
