package com.example.license.common;

/**
 * 审计事件类型枚举
 */
public enum AuditEventType {
    // 产品相关
    PRODUCT_CREATED,
    PRODUCT_DELETED,
    PRODUCT_UPDATED,
    PRODUCT_STATUS_CHANGED,
    // Key 相关
    KEY_CREATED,
    KEY_DISABLED,
    KEY_ENABLED,
    KEY_EXPIRED,
    KEY_REVOKED,
    KEY_DELETED,
    // 会话相关
    SESSION_ACTIVATE,
    SESSION_REVOKED,
    SESSION_EXPIRED,
    SESSION_CLEARED,
    // 在线检查
    ONLINE_CHECK,
    // 租户
    TENANT_CREATED,
    // 管理
    ADMIN_LOGIN,
    ADMIN_LOGOUT;
}
