package com.example.license.module.licensekey.service;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.example.license.common.BusinessException;
import com.example.license.common.AuditEventType;
import com.example.license.config.JwtConfig;
import com.example.license.module.licensekey.dto.KeyGenerateDTO;
import com.example.license.module.licensekey.entity.LicenseKey;
import com.example.license.module.licensekey.mapper.LicenseKeyMapper;
import com.example.license.module.product.entity.Product;
import com.example.license.module.product.service.ProductService;
import com.example.license.module.audit.entity.AuditLog;
import com.example.license.module.audit.mapper.AuditLogMapper;
import com.example.license.util.KeyEncryptionUtil;
import com.example.license.util.LicenseKeyUtil;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

@Service
@Slf4j
@RequiredArgsConstructor
public class LicenseKeyService extends ServiceImpl<LicenseKeyMapper, LicenseKey> {

    private static final DateTimeFormatter FORMATTER = DateTimeFormatter.ofPattern("yyyy-MM-dd");

    private final ProductService productService;
    private final AuditLogMapper auditLogMapper;
    private final JwtConfig jwtConfig;

    /**
     * 生成授权 Key
     */
    @Transactional
    public LicenseKey generate(KeyGenerateDTO dto) {
        // 校验产品存在且启用
        Product product = productService.getEnabledByProductId(dto.getProductId());

        // 生成唯一 Key，碰撞时重试
        String keyCode;
        int retries = 0;
        do {
            keyCode = LicenseKeyUtil.generate();
            retries++;
            if (retries > 10) {
                throw new BusinessException("生成 Key 失败，碰撞次数过多");
            }
        } while (count(new LambdaQueryWrapper<LicenseKey>().eq(LicenseKey::getKeyCode, keyCode)) > 0);

        // 解析过期时间（兼容多种格式：ISO、datetime-local、yyyy-MM-dd）
        LocalDateTime expiresAt = null;
        if (dto.getExpiresAt() != null && !dto.getExpiresAt().isBlank()) {
            String raw = dto.getExpiresAt().trim();
            try {
                if (raw.contains("T")) {
                    expiresAt = LocalDateTime.parse(raw);
                } else if (raw.matches("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}")) {
                    expiresAt = LocalDateTime.parse(raw.replace("T", " ") + ".000");
                } else {
                    expiresAt = LocalDate.parse(raw, FORMATTER).atStartOfDay();
                }
            } catch (Exception e) {
                log.warn("过期时间解析失败: {}, 将忽略", raw);
            }
        }

        LicenseKey key = new LicenseKey();
        key.setProductId(product.getProductId());
        key.setProductName(product.getName());
        key.setKeyCode(keyCode);
        key.setKeyShort(keyCode.substring(0, 11)); // DEMO-XXXX-XXXX 前缀
        // 加密存储明文 Key（防止数据库泄露后明文暴露）
        try {
            key.setEncryptedPlaintext(KeyEncryptionUtil.encrypt(jwtConfig.getKeyEncryptionSecret(), keyCode));
        } catch (Exception e) {
            log.warn("授权 Key 加密失败，将使用明文存储: {}", e.getMessage());
            key.setEncryptedPlaintext(keyCode); // 降级：明文存储
        }
        key.setAssignee(dto.getAssignee() != null ? dto.getAssignee() : "");
        key.setGripMinutes(dto.getGripMinutes() != null ? dto.getGripMinutes() : 1440);
        key.setExpiresAt(expiresAt);
        // 如果设置了过期时间且已过期，标记为已过期
        if (expiresAt != null && expiresAt.isBefore(LocalDateTime.now())) {
            key.setStatus(2);
        } else {
            key.setStatus(1);
        }
        key.setCreatedBy("admin");
        key.setCreatedAt(LocalDateTime.now());

        save(key);

        // 记录审计日志
        logEvent(AuditEventType.KEY_CREATED,
                "为产品 \"" + product.getName() + "\" 生成授权 Key，分配给\"" + key.getAssignee() + "\"",
                "admin", "后台操作",
                product.getProductId(), keyCode);

        return key;
    }

    /**
     * Key 列表（分页）
     */
    public Page<LicenseKey> listPage(int page, int size, String productId, Integer status, String keyword) {
        Page<LicenseKey> pageParam = new Page<>(page, size);
        LambdaQueryWrapper<LicenseKey> wrapper = new LambdaQueryWrapper<>();
        if (productId != null && !productId.isBlank()) {
            wrapper.eq(LicenseKey::getProductId, productId);
        }
        if (status != null) {
            wrapper.eq(LicenseKey::getStatus, status);
        }
        if (keyword != null && !keyword.isBlank()) {
            wrapper.and(w -> w
                    .like(LicenseKey::getKeyCode, keyword)
                    .or().like(LicenseKey::getAssignee, keyword));
        }
        wrapper.orderByDesc(LicenseKey::getCreatedAt);
        return page(pageParam, wrapper);
    }

    /**
     * 禁用 Key
     */
    @Transactional
    public void disable(Long id) {
        LicenseKey key = getById(id);
        if (key == null) {
            throw new BusinessException(404, "Key 不存在");
        }
        key.setStatus(0);
        updateById(key);

        logEvent(AuditEventType.KEY_DISABLED,
                "管理员禁用了 Key \"" + key.getKeyCode() + "\"",
                "admin", "后台操作",
                key.getProductId(), key.getKeyCode());
    }

    /**
     * 删除 Key
     */
    @Transactional
    public void delete(Long id) {
        LicenseKey key = getById(id);
        if (key == null) {
            throw new BusinessException(404, "Key 不存在");
        }
        removeById(id);

        logEvent(AuditEventType.KEY_DELETED,
                "管理员删除了 Key \"" + key.getKeyCode() + "\"",
                "admin", "后台操作",
                key.getProductId(), key.getKeyCode());
    }

    /**
     * 查询单个 Key 详情
     */
    public LicenseKey getById(Long id) {
        return super.getById(id);
    }

    /**
     * 通过 keyCode 查询
     */
    public LicenseKey getByKeyCode(String keyCode) {
        return getOne(new LambdaQueryWrapper<LicenseKey>().eq(LicenseKey::getKeyCode, keyCode));
    }

    /**
     * 复制 Key（解密明文 Key，供前端展示给用户）
     */
    public String copyKeyCode(Long id) {
        LicenseKey key = getById(id);
        if (key == null) {
            throw new BusinessException(404, "Key 不存在");
        }
        String encrypted = key.getEncryptedPlaintext();
        if (encrypted == null) {
            throw new BusinessException(400, "该 Key 没有保存可解密的明文");
        }
        try {
            return KeyEncryptionUtil.decrypt(jwtConfig.getKeyEncryptionSecret(), encrypted);
        } catch (Exception e) {
            // 可能是旧数据（明文存储）
            if (encrypted.length() > 50) {
                // 看起来像密文但解密失败
                throw new BusinessException(400, "授权 Key 解密失败，请检查加密配置");
            }
            return encrypted; // 明文存储，直接返回
        }
    }

    /**
     * 掩码显示 Key（前端展示用）
     */
    public String maskKeyCode(String fullKeyCode) {
        if (fullKeyCode == null || fullKeyCode.length() <= 8) {
            return fullKeyCode;
        }
        return fullKeyCode.substring(0, 4) + "..." + fullKeyCode.substring(fullKeyCode.length() - 4);
    }

    /**
     * 记录审计日志（公开，供 Controller 调用）
     */
    public void logEvent(AuditEventType type, String detail, String operator, String operatorType,
                         String productId, String keyCode) {
        AuditLog log = new AuditLog();
        log.setEventType(type.name());
        log.setEventDetail(detail);
        log.setOperator(operator);
        log.setOperatorType(operatorType);
        log.setTags("{\"productId\":\"" + productId + "\",\"keyCode\":\"" + keyCode + "\"}");
        log.setCreatedAt(LocalDateTime.now());
        auditLogMapper.insert(log);
    }
}
