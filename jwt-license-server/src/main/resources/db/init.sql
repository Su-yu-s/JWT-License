-- ============================================================
-- JWT 授权管理系统 - 数据库初始化脚本
-- ============================================================

CREATE DATABASE IF NOT EXISTS `jwt_license` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE `jwt_license`;

-- -----------------------------------------------------------
-- 1. 管理员表
-- -----------------------------------------------------------
DROP TABLE IF EXISTS `sys_admin`;
CREATE TABLE `sys_admin` (
    `id`          BIGINT       NOT NULL AUTO_INCREMENT COMMENT '主键',
    `username`    VARCHAR(128) NOT NULL COMMENT '用户名（邮箱）',
    `password`    VARCHAR(255) NOT NULL COMMENT 'BCrypt 加密密码',
    `role`        VARCHAR(32)  NOT NULL DEFAULT 'admin' COMMENT '角色 admin/operator',
    `created_at`  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_username` (`username`)
) ENGINE=InnoDB COMMENT='管理员表';

-- -----------------------------------------------------------
-- 2. 租户表
-- -----------------------------------------------------------
DROP TABLE IF EXISTS `biz_tenant`;
CREATE TABLE `biz_tenant` (
    `id`         BIGINT      NOT NULL AUTO_INCREMENT COMMENT '主键',
    `code`       VARCHAR(64) NOT NULL COMMENT '租户编码',
    `name`       VARCHAR(128) NOT NULL COMMENT '租户名称',
    `status`     TINYINT     NOT NULL DEFAULT 1 COMMENT '0=禁用 1=启用',
    `created_at` DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_code` (`code`)
) ENGINE=InnoDB COMMENT='租户表';

-- -----------------------------------------------------------
-- 3. 产品表
-- -----------------------------------------------------------
DROP TABLE IF EXISTS `biz_product`;
CREATE TABLE `biz_product` (
    `id`             BIGINT       NOT NULL AUTO_INCREMENT COMMENT '主键',
    `tenant_id`      BIGINT       NOT NULL COMMENT '所属租户',
    `product_id`     VARCHAR(64)  NOT NULL COMMENT '产品ID（JWT Audience）',
    `name`           VARCHAR(128) NOT NULL COMMENT '产品名称',
    `rsa_private_key` TEXT        COMMENT 'RSA 私钥 PEM 格式',
    `rsa_public_key`  TEXT        COMMENT 'RSA 公钥 PEM 格式',
    `status`         TINYINT      NOT NULL DEFAULT 1 COMMENT '0=禁用 1=启用',
    `created_at`     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_product_id` (`product_id`),
    KEY `idx_tenant_id` (`tenant_id`)
) ENGINE=InnoDB COMMENT='产品表';

-- -----------------------------------------------------------
-- 4. 授权 Key 表
-- -----------------------------------------------------------
DROP TABLE IF EXISTS `biz_license_key`;
CREATE TABLE `biz_license_key` (
    `id`           BIGINT        NOT NULL AUTO_INCREMENT COMMENT '主键',
    `product_id`   VARCHAR(64)   NOT NULL COMMENT '产品ID',
    `product_name` VARCHAR(128)  NOT NULL COMMENT '产品名称',
    `key_code`     VARCHAR(64)   NOT NULL COMMENT '授权Key（完整）',
    `key_short`    VARCHAR(32)   NOT NULL COMMENT 'Key短码（展示用）',
    `encrypted_plaintext` TEXT COMMENT '加密后的明文Key',
    `assignee`     VARCHAR(256)  DEFAULT '' COMMENT '分配对象',
    `grip_minutes` INT           NOT NULL DEFAULT 1440 COMMENT '离线宽限期（分钟）',
    `expires_at`   DATETIME      DEFAULT NULL COMMENT '过期时间',
    `status`       TINYINT       NOT NULL DEFAULT 1 COMMENT '0=禁用 1=启用 2=已过期 3=已删除',
    `created_by`   VARCHAR(64)   DEFAULT 'admin' COMMENT '创建人',
    `created_at`   DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_key_code` (`key_code`),
    KEY `idx_product_id` (`product_id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB COMMENT='授权Key表';

-- -----------------------------------------------------------
-- 5. 设备会话表
-- -----------------------------------------------------------
DROP TABLE IF EXISTS `biz_device_session`;
CREATE TABLE `biz_device_session` (
    `id`              BIGINT        NOT NULL AUTO_INCREMENT COMMENT '主键',
    `license_key_id`  BIGINT        NOT NULL COMMENT '授权Key ID',
    `product_id`      VARCHAR(64)   NOT NULL COMMENT '产品ID',
    `machine_hash`    VARCHAR(64)   NOT NULL COMMENT '设备机器码哈希',
    `sdk_version`     VARCHAR(32)   DEFAULT '' COMMENT 'SDK版本号',
    `jwt_token`       TEXT          COMMENT '签发的JWT Token',
    `status`          TINYINT       NOT NULL DEFAULT 1 COMMENT '0=已撤销 1=活跃 2=超时',
    `last_online_at`  DATETIME      DEFAULT NULL COMMENT '最近在线时间',
    `jwt_expires_at`  DATETIME      DEFAULT NULL COMMENT 'JWT过期时间',
    `revoked_at`      DATETIME      DEFAULT NULL COMMENT '撤销时间',
    `revoked_reason`  VARCHAR(200)  DEFAULT NULL COMMENT '撤销原因',
    `created_at`      DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_license_key_id` (`license_key_id`),
    KEY `idx_product_id` (`product_id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB COMMENT='设备会话表';

-- -----------------------------------------------------------
-- 6. 审计日志表
-- -----------------------------------------------------------
DROP TABLE IF EXISTS `sys_audit_log`;
CREATE TABLE `sys_audit_log` (
    `id`           BIGINT         NOT NULL AUTO_INCREMENT COMMENT '主键',
    `event_type`   VARCHAR(64)    NOT NULL COMMENT '事件类型',
    `event_detail` VARCHAR(512)   NOT NULL COMMENT '事件描述',
    `operator`     VARCHAR(64)    NOT NULL COMMENT '操作者',
    `operator_type` VARCHAR(32)   NOT NULL DEFAULT '后台操作' COMMENT '操作者类型',
    `tags`         VARCHAR(1024)  DEFAULT '' COMMENT '标签JSON',
    `created_at`   DATETIME       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_event_type` (`event_type`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB COMMENT='审计日志表';

-- -----------------------------------------------------------
-- 初始化数据
-- -----------------------------------------------------------

-- 租户（初始3个）
INSERT INTO `biz_tenant` (`code`, `name`, `status`) VALUES
('demo_tenant', 'Demo 租户', 1),
('acme_corp', 'Acme Corp', 1),
('globalsoft', 'GlobalSoft Inc', 1);

-- 管理员（密码: admin123，BCrypt 加密）
INSERT INTO `sys_admin` (`username`, `password`, `role`) VALUES
('admin@example.com', '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iAt6Z5EH', 'admin');

-- 产品（初始5个，RSA 密钥对会在创建时动态生成，此处为示例占位）
-- 实际使用时通过 API 创建产品，自动生成密钥对

-- 审计事件类型字典
-- KEY_CREATED / KEY_DISABLED / KEY_EXPIRED / KEY_REVOKED
-- PRODUCT_CREATED / PRODUCT_DELETED
-- SESSION_ACTIVATE / SESSION_REVOKED / SESSION_EXPIRED
-- ONLINE_CHECK
-- TENANT_CREATED
-- ADMIN_LOGIN
