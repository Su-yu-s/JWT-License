package com.example.license.module.session.service;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.example.license.common.AuditEventType;
import com.example.license.common.BusinessException;
import com.example.license.config.JwtConfig;
import com.example.license.module.audit.entity.AuditLog;
import com.example.license.module.audit.mapper.AuditLogMapper;
import com.example.license.module.licensekey.entity.LicenseKey;
import com.example.license.module.licensekey.service.LicenseKeyService;
import com.example.license.module.product.entity.Product;
import com.example.license.module.product.service.ProductService;
import com.example.license.module.session.dto.ActivateRequest;
import com.example.license.module.session.entity.DeviceSession;
import com.example.license.module.session.mapper.DeviceSessionMapper;
import com.example.license.util.SdkJwtUtil;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * 设备会话服务
 * 实现了完整的 SDK 生命周期管理：
 * - activate: 设备激活（一对一绑定，自动踢旧设备）
 * - heartbeat: 心跳保活（返回刷新后的 JWT）
 * - onlineCheck: 在线检查（类似心跳，但用于第三方验签）
 * - takeover: 强踢 takeover（当前设备主动踢走其他所有设备）
 * - deactivate: 主动注销
 * - revoke: 管理后台踢下线
 * - clearExpired: 清理超时会话
 */
@Slf4j
@Service
@RequiredArgsConstructor
public class DeviceSessionService extends ServiceImpl<DeviceSessionMapper, DeviceSession> {

    private final LicenseKeyService licenseKeyService;
    private final ProductService productService;
    private final JwtConfig jwtConfig;
    private final AuditLogMapper auditLogMapper;

    // ==================== SDK 接口 ====================

    /**
     * 设备激活
     * 1. 校验 Key 有效性
     * 2. 一对一限制：同 Key 只能有一个活跃会话
     * 3. 用产品 RSA 私钥签发 JWT（含完整 payload）
     * 4. 记录会话和审计日志
     */
    @Transactional
    public Map<String, Object> activate(ActivateRequest req) {
        // 1. 查找并校验 Key
        LicenseKey key = licenseKeyService.getByKeyCode(req.getKeyCode());
        if (key == null) {
            throw new BusinessException(404, "授权 Key 不存在");
        }
        if (key.getStatus() == 0) {
            throw new BusinessException(403, "授权 Key 已被禁用");
        }
        if (key.getStatus() == 2 || (key.getExpiresAt() != null && key.getExpiresAt().isBefore(LocalDateTime.now()))) {
            if (key.getStatus() != 2) {
                key.setStatus(2);
                licenseKeyService.updateById(key);
            }
            throw new BusinessException(403, "授权 Key 已过期");
        }

        // 2. 查找产品
        Product product = productService.getEnabledByProductId(key.getProductId());
        if (product.getRsaPrivateKey() == null || product.getRsaPrivateKey().isBlank()) {
            throw new BusinessException(500, "产品 \"" + product.getProductId() + "\" 缺少 RSA 私钥，请在管理后台重新生成");
        }

        // 3. 检查是否有其他设备的活跃会话（一对一限制）
        LocalDateTime cutoff = LocalDateTime.now().minusMinutes(jwtConfig.getSeatTimeoutMinutes());
        LambdaQueryWrapper<DeviceSession> activeWrapper = new LambdaQueryWrapper<>();
        activeWrapper.eq(DeviceSession::getLicenseKeyId, key.getId())
                .eq(DeviceSession::getStatus, 1)
                .gt(DeviceSession::getLastOnlineAt, cutoff);
        DeviceSession existingSession = getOne(activeWrapper);

        if (existingSession != null && !existingSession.getMachineHash().equals(req.getMachineHash())) {
            // 有其他设备在使用，拒绝激活
            String existHash = existingSession.getMachineHash() != null ? existingSession.getMachineHash() : "unknown";
            Map<String, Object> data = new HashMap<>();
            data.put("success", false);
            data.put("errorCode", "OCCUPIED");
            data.put("message", "授权 Key 正在被其他设备使用");
            data.put("capacity", 1);
            data.put("activeSeats", countActiveSeats(key.getId()));
            data.put("occupied", Map.of(
                    "sessionId", existingSession.getId(),
                    "machineHashPrefix", existHash.substring(0, Math.min(12, existHash.length())),
                    "lastSeenAt", existingSession.getLastOnlineAt() != null ? existingSession.getLastOnlineAt().toString() : ""
            ));
            return data;
        }

        // 4. 复用旧会话或创建新会话
        if (existingSession != null && existingSession.getMachineHash().equals(req.getMachineHash())) {
            existingSession.setStatus(1);
            existingSession.setLastOnlineAt(LocalDateTime.now());
            existingSession.setRevokedAt(null);
            existingSession.setRevokedReason(null);
            updateById(existingSession);
        }

        // 5. 签发 JWT（如果还没有 session 则创建）
        DeviceSession session;
        if (existingSession != null) {
            session = existingSession;
        } else {
            session = new DeviceSession();
            session.setLicenseKeyId(key.getId());
            session.setProductId(key.getProductId());
            if (!save(session)) {
                throw new BusinessException(500, "创建设备会话失败");
            }
            if (session.getId() == null || session.getId() <= 0) {
                throw new BusinessException(500, "创建设备会话失败：ID 未回填");
            }
        }

        // 6. 计算 JWT 过期时间
        long expirationSeconds = key.getExpiresAt() != null
                ? java.time.Duration.between(LocalDateTime.now(), key.getExpiresAt()).getSeconds()
                : jwtConfig.getSdkDefaultExpiration();
        if (expirationSeconds <= 0) expirationSeconds = jwtConfig.getSdkDefaultExpiration();

        // 7. 签发 JWT
        String jwtToken;
        try {
            jwtToken = SdkJwtUtil.sign(
                    product.getRsaPrivateKey(),
                    "jwt-license-server",
                    product.getProductId(),
                    key.getId().toString(),
                    key.getProductId(),
                    product.getProductId(),
                    req.getMachineHash(),
                    session.getId().toString(),
                    expirationSeconds
            );
        } catch (Exception e) {
            log.error("签发 SDK JWT 失败，产品: {}, 错误: {}", product.getProductId(), e.getMessage());
            throw new BusinessException(500, "签发授权 Token 失败，请联系管理员检查产品 RSA 密钥配置");
        }

        session.setMachineHash(req.getMachineHash());
        session.setSdkVersion(req.getSdkVersion() != null ? req.getSdkVersion() : "");
        session.setJwtToken(jwtToken);
        session.setStatus(1);
        session.setLastOnlineAt(LocalDateTime.now());
        session.setJwtExpiresAt(LocalDateTime.now().plusSeconds(expirationSeconds));
        session.setRevokedAt(null);
        session.setRevokedReason(null);
        updateById(session);

        // 8. 审计日志
        logEvent(AuditEventType.SESSION_ACTIVATE,
                "设备激活成功 — 机器码: " + req.getMachineHash(),
                "sdk", "客户端",
                key.getProductId(), key.getKeyCode());

        // 9. 返回结果
        Map<String, Object> result = new HashMap<>();
        result.put("success", true);
        result.put("token", jwtToken);
        result.put("issuedAt", LocalDateTime.now().toString());
        result.put("expiresAt", session.getJwtExpiresAt().toString());
        result.put("sessionId", session.getId());
        result.put("capacity", 1);
        result.put("activeSeats", countActiveSeats(key.getId()));
        result.put("publicKeyPem", product.getRsaPublicKey());
        result.put("issuer", "jwt-license-server");
        return result;
    }

    /**
     * 心跳保活 — 刷新 lastOnlineAt，返回新的 JWT
     */
    @Transactional
    public Map<String, Object> heartbeat(String keyCode) {
        LicenseKey key = licenseKeyService.getByKeyCode(keyCode);
        if (key == null) {
            throw new BusinessException(404, "Key 不存在");
        }

        // 查找活跃会话
        LambdaQueryWrapper<DeviceSession> wrapper = new LambdaQueryWrapper<>();
        wrapper.eq(DeviceSession::getLicenseKeyId, key.getId())
                .eq(DeviceSession::getStatus, 1);
        DeviceSession session = getOne(wrapper);
        if (session == null) {
            throw new BusinessException(404, "无活跃会话");
        }

        // 检查是否超时
        LocalDateTime cutoff = LocalDateTime.now().minusMinutes(jwtConfig.getSeatTimeoutMinutes());
        if (session.getLastOnlineAt() != null && session.getLastOnlineAt().isBefore(cutoff)) {
            throw new BusinessException(408, "会话已超时，请重新激活");
        }

        // 查找产品
        Product product = productService.getEnabledByProductId(key.getProductId());

        // 刷新心跳时间
        session.setLastOnlineAt(LocalDateTime.now());
        updateById(session);

        // 签发新 JWT
        long expirationSeconds = key.getExpiresAt() != null
                ? java.time.Duration.between(LocalDateTime.now(), key.getExpiresAt()).getSeconds()
                : jwtConfig.getSdkDefaultExpiration();
        if (expirationSeconds <= 0) expirationSeconds = jwtConfig.getSdkDefaultExpiration();

        String jwtToken;
        try {
            jwtToken = SdkJwtUtil.sign(
                    product.getRsaPrivateKey(),
                    "jwt-license-server",
                    product.getProductId(),
                    key.getId().toString(),
                    key.getProductId(),
                    product.getProductId(),
                    session.getMachineHash(),
                    session.getId().toString(),
                    expirationSeconds
            );
        } catch (Exception e) {
            log.error("签发心跳 JWT 失败，产品: {}, 错误: {}", product.getProductId(), e.getMessage());
            throw new BusinessException(500, "签发授权 Token 失败，请联系管理员检查产品 RSA 密钥配置");
        }

        session.setJwtToken(jwtToken);
        session.setJwtExpiresAt(LocalDateTime.now().plusSeconds(expirationSeconds));
        updateById(session);

        logEvent(AuditEventType.ONLINE_CHECK,
                "心跳保活，Key: " + key.getKeyCode(),
                "sdk", "客户端",
                key.getProductId(), key.getKeyCode());

        Map<String, Object> result = new HashMap<>();
        result.put("success", true);
        result.put("token", jwtToken);
        result.put("expiresAt", session.getJwtExpiresAt().toString());
        result.put("sessionId", session.getId());
        result.put("activeSeats", countActiveSeats(key.getId()));
        result.put("publicKeyPem", product.getRsaPublicKey());
        result.put("issuer", "jwt-license-server");
        return result;
    }

    /**
     * 在线检查（类似心跳，但不刷新 JWT，仅更新 lastOnlineAt）
     */
    @Transactional
    public Map<String, Object> onlineCheck(String keyCode) {
        LicenseKey key = licenseKeyService.getByKeyCode(keyCode);
        if (key == null) {
            throw new BusinessException(404, "Key 不存在");
        }

        LambdaQueryWrapper<DeviceSession> wrapper = new LambdaQueryWrapper<>();
        wrapper.eq(DeviceSession::getLicenseKeyId, key.getId())
                .eq(DeviceSession::getStatus, 1);
        DeviceSession session = getOne(wrapper);
        if (session == null) {
            throw new BusinessException(404, "无活跃会话");
        }

        // 检查是否超时
        LocalDateTime cutoff = LocalDateTime.now().minusMinutes(jwtConfig.getSeatTimeoutMinutes());
        if (session.getLastOnlineAt() != null && session.getLastOnlineAt().isBefore(cutoff)) {
            throw new BusinessException(408, "会话已超时");
        }

        session.setLastOnlineAt(LocalDateTime.now());
        updateById(session);

        logEvent(AuditEventType.ONLINE_CHECK,
                "在线检查，Key: " + key.getKeyCode(),
                "sdk", "客户端",
                key.getProductId(), key.getKeyCode());

        Map<String, Object> result = new HashMap<>();
        result.put("success", true);
        result.put("sessionId", session.getId());
        result.put("activeSeats", countActiveSeats(key.getId()));
        return result;
    }

    /**
     * 强踢 takeover — 当前设备主动踢走其他所有设备
     */
    @Transactional
    public Map<String, Object> takeover(ActivateRequest req) {
        LicenseKey key = licenseKeyService.getByKeyCode(req.getKeyCode());
        if (key == null) {
            throw new BusinessException(404, "授权 Key 不存在");
        }
        if (key.getStatus() == 0) {
            throw new BusinessException(403, "授权 Key 已被禁用");
        }

        Product product = productService.getEnabledByProductId(key.getProductId());

        // 踢走所有其他设备的活跃会话
        LocalDateTime cutoff = LocalDateTime.now().minusMinutes(jwtConfig.getSeatTimeoutMinutes());
        LambdaQueryWrapper<DeviceSession> wrapper = new LambdaQueryWrapper<>();
        wrapper.eq(DeviceSession::getLicenseKeyId, key.getId())
                .eq(DeviceSession::getStatus, 1)
                .gt(DeviceSession::getLastOnlineAt, cutoff);
        List<DeviceSession> activeSessions = list(wrapper);

        for (DeviceSession s : activeSessions) {
            if (!s.getMachineHash().equals(req.getMachineHash())) {
                s.setStatus(0);
                s.setRevokedAt(LocalDateTime.now());
                s.setRevokedReason("client_takeover");
                updateById(s);
                logEvent(AuditEventType.SESSION_REVOKED,
                        "被新设备强踢 — 机器码: " + s.getMachineHash(),
                        "sdk", "客户端",
                        key.getProductId(), key.getKeyCode());
            }
        }

        // 复用 activate 逻辑创建当前设备会话
        return activate(req);
    }

    /**
     * 主动注销 deactivate
     */
    @Transactional
    public Map<String, Object> deactivate(String keyCode) {
        LicenseKey key = licenseKeyService.getByKeyCode(keyCode);
        if (key == null) {
            throw new BusinessException(404, "Key 不存在");
        }

        LambdaQueryWrapper<DeviceSession> wrapper = new LambdaQueryWrapper<>();
        wrapper.eq(DeviceSession::getLicenseKeyId, key.getId())
                .eq(DeviceSession::getStatus, 1);
        List<DeviceSession> sessions = list(wrapper);

        int revokedCount = 0;
        Long lastSessionId = null;
        for (DeviceSession session : sessions) {
            session.setStatus(0);
            session.setRevokedAt(LocalDateTime.now());
            session.setRevokedReason("client_deactivate");
            updateById(session);
            revokedCount++;
            lastSessionId = session.getId();

            logEvent(AuditEventType.SESSION_REVOKED,
                    "客户端主动注销",
                    "sdk", "客户端",
                    key.getProductId(), key.getKeyCode());
        }

        Map<String, Object> result = new HashMap<>();
        result.put("success", true);
        result.put("sessionId", lastSessionId);
        result.put("revokedCount", revokedCount);
        result.put("activeSeats", countActiveSeats(key.getId()));
        return result;
    }

    // ==================== 管理后台接口 ====================

    /**
     * 会话列表（分页）
     */
    public Page<DeviceSession> listActiveSessions(int page, int size, String productId) {
        Page<DeviceSession> pageParam = new Page<>(page, size);
        LambdaQueryWrapper<DeviceSession> wrapper = new LambdaQueryWrapper<>();
        if (productId != null && !productId.isBlank()) {
            wrapper.eq(DeviceSession::getProductId, productId);
        }
        wrapper.orderByDesc(DeviceSession::getLastOnlineAt);
        return page(pageParam, wrapper);
    }

    /**
     * 踢下线（撤销会话）
     */
    @Transactional
    public void revoke(Long id) {
        DeviceSession session = getById(id);
        if (session == null) {
            throw new BusinessException(404, "会话不存在");
        }
        session.setStatus(0);
        session.setRevokedAt(LocalDateTime.now());
        session.setRevokedReason("admin_revoke");
        updateById(session);

        LicenseKey key = licenseKeyService.getById(session.getLicenseKeyId());
        String keyCode = key != null ? key.getKeyCode() : "unknown";
        logEvent(AuditEventType.SESSION_REVOKED,
                "踢下线设备会话，Key: " + keyCode,
                "admin", "后台操作",
                session.getProductId(), keyCode);
    }

    /**
     * 清理超时会话
     */
    @Transactional
    public int clearExpired() {
        LocalDateTime threshold = LocalDateTime.now().minusMinutes(jwtConfig.getSeatTimeoutMinutes());
        LambdaQueryWrapper<DeviceSession> wrapper = new LambdaQueryWrapper<>();
        wrapper.eq(DeviceSession::getStatus, 1)
                .and(w -> w.lt(DeviceSession::getLastOnlineAt, threshold)
                        .or()
                        .lt(DeviceSession::getJwtExpiresAt, LocalDateTime.now()));
        List<DeviceSession> expired = list(wrapper);
        if (expired.isEmpty()) {
            return 0;
        }
        expired.forEach(s -> {
            s.setStatus(0);
            s.setRevokedAt(LocalDateTime.now());
            s.setRevokedReason("timeout_cleared");
        });
        updateBatchById(expired);

        logEvent(AuditEventType.SESSION_CLEARED,
                "清理超时会话，共 " + expired.size() + " 个",
                "system", "系统", "", "");
        return expired.size();
    }

    /**
     * 统计 Key 的活跃座位数（超时时间内的心跳）
     */
    private int countActiveSeats(Long licenseKeyId) {
        LocalDateTime cutoff = LocalDateTime.now().minusMinutes(jwtConfig.getSeatTimeoutMinutes());
        LambdaQueryWrapper<DeviceSession> wrapper = new LambdaQueryWrapper<>();
        wrapper.eq(DeviceSession::getLicenseKeyId, licenseKeyId)
                .eq(DeviceSession::getStatus, 1)
                .gt(DeviceSession::getLastOnlineAt, cutoff);
        return Math.toIntExact(count(wrapper));
    }

    /**
     * 记录审计日志
     */
    private void logEvent(AuditEventType type, String detail, String operator, String operatorType,
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
