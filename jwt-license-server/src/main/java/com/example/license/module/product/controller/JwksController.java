package com.example.license.module.product.controller;

import com.example.license.common.Result;
import com.example.license.module.product.entity.Product;
import com.example.license.module.product.service.ProductService;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * JWKS 端点
 * 供 SDK 客户端获取产品公钥，用于验证 JWT Token 签名
 * 遵循 RFC 7517 JSON Web Key 格式
 */
@RestController
@RequestMapping("/.well-known")
@RequiredArgsConstructor
public class JwksController {

    private final ProductService productService;

    /**
     * 获取所有启用产品的公钥集合（JWKS 格式）
     */
    @GetMapping("/jwks.json")
    public Map<String, Object> jwks() {
        List<Product> products = productService.list(
                new com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper<Product>()
                        .eq(Product::getStatus, 1)
        );

        Map<String, Object> jwks = new HashMap<>();
        java.util.ArrayList<Map<String, Object>> keys = new java.util.ArrayList<>();

        for (Product product : products) {
            if (product.getRsaPublicKey() != null && !product.getRsaPublicKey().isBlank()) {
                try {
                    // 从 PEM 提取 RSA 参数，转换为 JWK
                    Map<String, Object> jwk = pemToJwk(product.getRsaPublicKey(), product.getProductId());
                    jwk.put("kid", product.getProductId());
                    jwk.put("alg", "RS256");
                    jwk.put("use", "sig");
                    keys.add(jwk);
                } catch (Exception e) {
                    // 跳过无法解析的公钥
                }
            }
        }

        jwks.put("keys", keys);
        return jwks;
    }

    /**
     * 将 PEM 公钥转换为 JWK (RSA)
     */
    private Map<String, Object> pemToJwk(String pem, String kid) throws Exception {
        // 解析 PEM 获取 modulus 和 exponent
        String base64 = pem
                .replace("-----BEGIN PUBLIC KEY-----", "")
                .replace("-----END PUBLIC KEY-----", "")
                .replaceAll("\\s", "");

        java.security.spec.X509EncodedKeySpec keySpec =
                new java.security.spec.X509EncodedKeySpec(java.util.Base64.getDecoder().decode(base64));
        java.security.KeyFactory kf = java.security.KeyFactory.getInstance("RSA");
        java.security.PublicKey publicKey = kf.generatePublic(keySpec);

        java.security.interfaces.RSAPublicKey rsaKey = (java.security.interfaces.RSAPublicKey) publicKey;

        Map<String, Object> jwk = new HashMap<>();
        jwk.put("kty", "RSA");
        jwk.put("kid", kid);

        // modulus: base64url(n)
        jwk.put("n", base64UrlEncode(rsaKey.getModulus()));
        // exponent: base64url(e)
        jwk.put("e", base64UrlEncode(rsaKey.getPublicExponent()));

        return jwk;
    }

    /**
     * BigInteger 转为 base64url 编码字符串
     */
    private String base64UrlEncode(java.math.BigInteger value) {
        byte[] bytes = value.toByteArray();
        // 确保是正数表示
        if (bytes.length > 1 && bytes[0] == 0) {
            bytes = java.util.Arrays.copyOfRange(bytes, 1, bytes.length);
        }
        return java.util.Base64.getUrlEncoder().withoutPadding().encodeToString(bytes);
    }
}
