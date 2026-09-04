package com.example.license.module.product.controller;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.example.license.common.Result;
import com.example.license.module.product.dto.ProductCreateDTO;
import com.example.license.module.product.dto.ProductUpdateDTO;
import com.example.license.module.product.entity.Product;
import com.example.license.module.product.mapper.ProductMapper;
import com.example.license.module.product.service.ProductService;
import com.example.license.module.tenant.entity.Tenant;
import com.example.license.module.tenant.mapper.TenantMapper;
import lombok.Data;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@RestController
@RequestMapping("/api/products")
@RequiredArgsConstructor
public class ProductController {

    private final ProductService productService;
    private final TenantMapper tenantMapper;

    /**
     * 创建产品
     */
    @PostMapping
    public Result<Product> create(@RequestBody ProductCreateDTO dto) {
        Product product = productService.createProduct(dto);
        return Result.ok(product);
    }

    /**
     * 产品列表（支持租户筛选，返回租户 code）
     */
    @GetMapping
    public Result<List<Map<String, Object>>> list(
            @RequestParam(required = false) Long tenantId) {
        LambdaQueryWrapper<Product> wrapper = new LambdaQueryWrapper<>();
        if (tenantId != null) {
            wrapper.eq(Product::getTenantId, tenantId);
        }
        wrapper.orderByDesc(Product::getCreatedAt);
        List<Product> products = productService.list(wrapper);

        // 批量加载租户映射
        Map<Long, Tenant> tenantMap = tenantMapper.selectList(null)
                .stream().collect(Collectors.toMap(Tenant::getId, t -> t));

        return Result.ok(products.stream().map(p -> {
            Map<String, Object> m = new java.util.HashMap<>();
            m.put("id", p.getId());
            m.put("name", p.getName());
            m.put("productId", p.getProductId());
            Tenant t = tenantMap.get(p.getTenantId());
            m.put("tenantCode", t != null ? t.getCode() : p.getTenantId());
            m.put("status", p.getStatus());
            m.put("createdAt", p.getCreatedAt());
            return m;
        }).collect(Collectors.toList()));
    }

    /**
     * 产品详情
     */
    @GetMapping("/{id}")
    public Result<Product> detail(@PathVariable Long id) {
        Product product = productService.getById(id);
        if (product == null) {
            return Result.fail(404, "产品不存在");
        }
        return Result.ok(product);
    }

    /**
     * 更新产品
     */
    @PutMapping("/{id}")
    public Result<Product> update(@PathVariable Long id, @RequestBody ProductUpdateDTO dto) {
        Product product = productService.updateProduct(id, dto);
        return Result.ok(product);
    }

    /**
     * 删除产品
     */
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        Product product = productService.getById(id);
        if (product == null) {
            return Result.fail(404, "产品不存在");
        }
        productService.removeById(id);
        return Result.ok();
    }

    /**
     * 启用/禁用产品
     */
    @PatchMapping("/{id}/status")
    public Result<Void> toggleStatus(@PathVariable Long id, @RequestParam int status) {
        productService.toggleStatus(id, status);
        return Result.ok();
    }
}
