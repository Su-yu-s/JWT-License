package com.example.license.module.licensekey.controller;

import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.example.license.common.Result;
import com.example.license.module.licensekey.dto.KeyGenerateDTO;
import com.example.license.module.licensekey.entity.LicenseKey;
import com.example.license.module.licensekey.service.LicenseKeyService;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/license-keys")
@RequiredArgsConstructor
public class LicenseKeyController {

    private final LicenseKeyService licenseKeyService;

    /**
     * 生成授权 Key
     */
    @PostMapping
    public Result<LicenseKey> generate(@RequestBody KeyGenerateDTO dto) {
        LicenseKey key = licenseKeyService.generate(dto);
        return Result.ok(key);
    }

    /**
     * Key 列表（分页，返回掩码版 displayKey）
     */
    @GetMapping
    public Result<Page<LicenseKey>> list(
            @RequestParam(defaultValue = "1") int page,
            @RequestParam(defaultValue = "20") int size,
            @RequestParam(required = false) String productId,
            @RequestParam(required = false) Integer status,
            @RequestParam(required = false) String keyword) {
        Page<LicenseKey> result = licenseKeyService.listPage(page, size, productId, status, keyword);
        return Result.ok(result);
    }

    /**
     * Key 详情
     */
    @GetMapping("/{id}")
    public Result<LicenseKey> detail(@PathVariable Long id) {
        LicenseKey key = licenseKeyService.getById(id);
        if (key == null) {
            return Result.fail(404, "Key 不存在");
        }
        return Result.ok(key);
    }

    /**
     * 复制 Key（解密明文，仅显示一次）
     */
    @PostMapping("/{id}/copy")
    public Result<Map<String, String>> copyKey(@PathVariable Long id) {
        String keyCode = licenseKeyService.copyKeyCode(id);
        Map<String, String> data = new HashMap<>();
        data.put("keyCode", keyCode);
        data.put("masked", licenseKeyService.maskKeyCode(keyCode));
        return Result.ok(data);
    }

    /**
     * 禁用 Key
     */
    @PatchMapping("/{id}/disable")
    public Result<Void> disable(@PathVariable Long id) {
        licenseKeyService.disable(id);
        return Result.ok();
    }

    /**
     * 删除 Key（软删除：标记为 deleted，同时踢下在线设备）
     */
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        LicenseKey key = licenseKeyService.getById(id);
        if (key == null) {
            return Result.fail(404, "Key 不存在");
        }
        key.setStatus(3); // 3 = deleted
        licenseKeyService.updateById(key);

        // 踢下所有该 Key 的活跃会话
        licenseKeyService.getBaseMapper().revokeSessionsByKey(key.getId());

        licenseKeyService.logEvent(com.example.license.common.AuditEventType.KEY_DELETED,
                "管理员删除了 Key \"" + key.getKeyCode() + "\"",
                "admin", "后台操作",
                key.getProductId(), key.getKeyCode());

        return Result.ok();
    }
}
