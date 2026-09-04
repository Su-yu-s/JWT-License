package com.example.license.module.sdk.service;

import com.example.license.module.product.entity.Product;
import com.example.license.module.product.service.ProductService;
import com.example.license.util.SdkJwtUtil;
import io.jsonwebtoken.Claims;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;

@Service
@RequiredArgsConstructor
public class SdkService {

    private final ProductService productService;

    /**
     * 验证 SDK JWT Token
     *
     * @param token     JWT Token
     * @param productId 产品 ID
     * @return Claims（验签通过）
     */
    public Claims validateToken(String token, String productId) {
        Product product = productService.getEnabledByProductId(productId);
        return SdkJwtUtil.verify(product.getRsaPublicKey(), token);
    }
}
