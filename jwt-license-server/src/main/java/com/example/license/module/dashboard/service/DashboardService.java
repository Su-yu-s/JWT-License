package com.example.license.module.dashboard.service;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.example.license.module.audit.entity.AuditLog;
import com.example.license.module.audit.mapper.AuditLogMapper;
import com.example.license.module.dashboard.entity.DashboardStats;
import com.example.license.module.licensekey.entity.LicenseKey;
import com.example.license.module.licensekey.mapper.LicenseKeyMapper;
import com.example.license.module.product.entity.Product;
import com.example.license.module.product.mapper.ProductMapper;
import com.example.license.module.session.entity.DeviceSession;
import com.example.license.module.session.mapper.DeviceSessionMapper;
import com.example.license.module.tenant.entity.Tenant;
import com.example.license.module.tenant.mapper.TenantMapper;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
@RequiredArgsConstructor
public class DashboardService {

    private final TenantMapper tenantMapper;
    private final ProductMapper productMapper;
    private final LicenseKeyMapper licenseKeyMapper;
    private final DeviceSessionMapper deviceSessionMapper;
    private final AuditLogMapper auditLogMapper;

    /**
     * 获取仪表盘统计数据
     */
    public DashboardStats getStats() {
        long tenantCount = tenantMapper.selectCount(null);
        long productCount = productMapper.selectCount(null);
        long keyCount = licenseKeyMapper.selectCount(null);
        long activeSessionCount = deviceSessionMapper.selectCount(
                new LambdaQueryWrapper<DeviceSession>().eq(DeviceSession::getStatus, 1)
        );

        return new DashboardStats(tenantCount, productCount, keyCount, activeSessionCount);
    }

    /**
     * 最近 N 条审计日志
     */
    public List<AuditLog> recentAudit(int limit) {
        LambdaQueryWrapper<AuditLog> wrapper = new LambdaQueryWrapper<>();
        wrapper.orderByDesc(AuditLog::getCreatedAt);
        wrapper.last("LIMIT " + limit);
        return auditLogMapper.selectList(wrapper);
    }

    /**
     * 审计日志列表（分页）
     */
    public Page<AuditLog> listAuditLogs(int page, int size, String eventType, String keyword) {
        Page<AuditLog> pageParam = new Page<>(page, size);
        LambdaQueryWrapper<AuditLog> wrapper = new LambdaQueryWrapper<>();
        if (eventType != null && !eventType.isBlank()) {
            wrapper.eq(AuditLog::getEventType, eventType);
        }
        if (keyword != null && !keyword.isBlank()) {
            wrapper.and(w -> w
                    .like(AuditLog::getEventDetail, keyword)
                    .or().like(AuditLog::getOperator, keyword));
        }
        wrapper.orderByDesc(AuditLog::getCreatedAt);
        return auditLogMapper.selectPage(pageParam, wrapper);
    }
}
