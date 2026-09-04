/*
Navicat MySQL Data Transfer

Source Server         : localhost_3306
Source Server Version : 50722
Source Host           : localhost:3306
Source Database       : jwt_license

Target Server Type    : MYSQL
Target Server Version : 50722
File Encoding         : 65001

Date: 2026-06-21 18:30:02
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `biz_device_session`
-- ----------------------------
DROP TABLE IF EXISTS `biz_device_session`;
CREATE TABLE `biz_device_session` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `license_key_id` bigint(20) NOT NULL COMMENT '授权Key ID',
  `product_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '产品ID',
  `machine_hash` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '设备机器码哈希',
  `sdk_version` varchar(32) COLLATE utf8mb4_unicode_ci DEFAULT '' COMMENT 'SDK版本号',
  `jwt_token` text COLLATE utf8mb4_unicode_ci COMMENT '签发的JWT Token',
  `status` tinyint(4) NOT NULL DEFAULT '1' COMMENT '0=已撤销 1=活跃 2=超时',
  `last_online_at` datetime DEFAULT NULL COMMENT '最近在线时间',
  `jwt_expires_at` datetime DEFAULT NULL COMMENT 'JWT过期时间',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_license_key_id` (`license_key_id`),
  KEY `idx_product_id` (`product_id`),
  KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='设备会话表';

-- ----------------------------
-- Records of biz_device_session
-- ----------------------------

-- ----------------------------
-- Table structure for `biz_license_key`
-- ----------------------------
DROP TABLE IF EXISTS `biz_license_key`;
CREATE TABLE `biz_license_key` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `product_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '产品ID',
  `product_name` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '产品名称',
  `key_code` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '授权Key（完整）',
  `key_short` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Key短码（展示用）',
  `assignee` varchar(256) COLLATE utf8mb4_unicode_ci DEFAULT '' COMMENT '分配对象',
  `grip_minutes` int(11) NOT NULL DEFAULT '1440' COMMENT '离线宽限期（分钟）',
  `expires_at` datetime DEFAULT NULL COMMENT '过期时间',
  `status` tinyint(4) NOT NULL DEFAULT '1' COMMENT '0=禁用 1=启用 2=已过期 3=已撤销',
  `created_by` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT 'admin' COMMENT '创建人',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `encrypted_plaintext` text COLLATE utf8mb4_unicode_ci COMMENT '加密后的明文Key',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_key_code` (`key_code`),
  KEY `idx_product_id` (`product_id`),
  KEY `idx_status` (`status`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='授权Key表';

-- ----------------------------
-- Records of biz_license_key
-- ----------------------------
INSERT INTO `biz_license_key` VALUES ('1', 'web', 'Web端应用', 'N1S8-ELVH-SG7O-VKIM-ZSY7', 'N1S8-ELVH-S', '苏', '1440', '2026-06-15 19:06:00', '2', 'admin', '2026-06-15 19:06:21', 'uGquVbfwMFqxryT/14CBI7R3Z9kvbp22cX5n/FJOmptieq37zNJMpAO7388aLAsEMgFGTw==');

-- ----------------------------
-- Table structure for `biz_product`
-- ----------------------------
DROP TABLE IF EXISTS `biz_product`;
CREATE TABLE `biz_product` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `tenant_id` bigint(20) NOT NULL COMMENT '所属租户',
  `product_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '产品ID（JWT Audience）',
  `name` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '产品名称',
  `rsa_private_key` text COLLATE utf8mb4_unicode_ci COMMENT 'RSA 私钥 PEM 格式',
  `rsa_public_key` text COLLATE utf8mb4_unicode_ci COMMENT 'RSA 公钥 PEM 格式',
  `status` tinyint(4) NOT NULL DEFAULT '1' COMMENT '0=禁用 1=启用',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_product_id` (`product_id`),
  KEY `idx_tenant_id` (`tenant_id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='产品表';

-- ----------------------------
-- Records of biz_product
-- ----------------------------
INSERT INTO `biz_product` VALUES ('1', '4', 'web', 'Web端应用', '-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCdXufThanPSZNK\nFWfF57yvu6AIiHMF1Mxt+tPg35MaUxhbhJNIwpFLnFN5tp4ZH4g1SFXfnvgagMqA\nzyOvd5uD78Kjb8Khh7F7Yd1Vo46IzzlgEeynY5cMphRZgu6kCksg7A6clpkRXZze\n1/SUheB8PVe3/qWtZ0yun19a5+7ApqKAyi5d1ATo5sjGLe7roKQ1Za/zKbBRygam\nsa3L2InUObE/k7luDQ2dlNVhlVPnjjTiQr0aHiB/N8FJPjS4kDMIL7SBghgcu3ii\nlwxTvaLHToMaqkXHKK4CzzWHvqY9sPdyk4WpZGA6fwnL6InOscPg8ukw2+lNV91N\n46kJLVpZAgMBAAECggEADIn3KHGXffvg9P+vSjkJ3eoHY8v/d4uk6ERv5RSGA4xz\nEzkFL+sffun19WuOvaC3Y8t02tcEnL6oo/UdOkoU8EEb00BAk/ScYyDWl7rATM1P\ncdoBGbXuifmiWHQoG0DzYIGpM7faJWSTRGbqRs/t+ug7rjOSNDatXk02NfznQfPv\nrKAeI8ysjho2Mz9V+1RfMlq9fHMsC0mtGsNVMCnrSUD0FQxyne+XYIuNB1d738XH\nQLnKX3zBdYlSrdLi1EW/YFwKUDUMQI8QLU6Ph87QMhpxJfncF8t/DLETi3ElL2iz\naUWE02tnUA+kJB7CLuK53CYVLlR6ZWsZ7c/ipPJbiQKBgQC/nVRGb021b3oLSpd+\nK1608uhEGsLlWPN/c85z5MLufbc8U4IrKXVhEKQe6HtIEjholodi8erl21RelF2D\nPE19+zaXi0w+t5DyK1HkRlV6HiijwFpwbqkeJXejKMmXwfprZx0cxCECSLWj6swy\nFmTmBHLrfQw8WzwnmMkxM8ejxQKBgQDSP+xxhB3zoP/TGtGgdeLAXU0uLdHY5aZ4\n/iG1+g1CN+5JdAmpq5/ez41mqFYuWcS9fXcrhUzhQOBxEmO6vDOf8LCWr8r3+0PV\nxzwHifh0kl59+hmbwiJsF3q+ZPhHHugDi10kCwaOduyg+WhRAS4mDoIZafa2fHTr\nCuoO4SOBhQKBgFwo7pzF8ek/CcMeiEid0XpUjpQbDvxCUTNO3tBtCbitbJEuVSAv\nW45o85cOGCVs5gafx9Q2KNWPLUAkOenh+h+K0RIIZI6RubxQS53wmjIXCVLhNF55\nSwv911H5TySXnrtDqAPq7TltATgquTWqAbOEFkXfKiBpfUBKQxJQJkhFAoGBAKRk\nnRDJN84l5cH/p0bqxgiZK17fLsSEFB1ov50VTkaniubeFywWaKOD79ED8Ja/VAjB\n0Gs0CD/cRWHD+jypKUh2nuzDuVUanxjJdpOCesVTKRUhd+KE8ftIhI3YTxT1An66\n+nHTOSYtsKnROZKU9KuHDoymgOSA/b5GQ2qs+OEtAoGBAKTdlA7rcgNMD8RrLfJ2\n2F42PVYgxUOj5y+z5shbvtDhzTIyq+KbXXuBl4NQJEptJFgsb/MSIglulZ7btID9\ndADMRm5cGF9Slxe+TPsg16pMw9OuUMVtPsVAJoW8gIJyNsALd1iBgv0E19ti+Zfq\nJlmfveXhw86YFpYX7itbWn7u\n-----END PRIVATE KEY-----', '-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnV7n04Wpz0mTShVnxee8\nr7ugCIhzBdTMbfrT4N+TGlMYW4STSMKRS5xTebaeGR+INUhV3574GoDKgM8jr3eb\ng+/Co2/CoYexe2HdVaOOiM85YBHsp2OXDKYUWYLupApLIOwOnJaZEV2c3tf0lIXg\nfD1Xt/6lrWdMrp9fWufuwKaigMouXdQE6ObIxi3u66CkNWWv8ymwUcoGprGty9iJ\n1DmxP5O5bg0NnZTVYZVT54404kK9Gh4gfzfBST40uJAzCC+0gYIYHLt4opcMU72i\nx06DGqpFxyiuAs81h76mPbD3cpOFqWRgOn8Jy+iJzrHD4PLpMNvpTVfdTeOpCS1a\nWQIDAQAB\n-----END PUBLIC KEY-----', '1', '2026-06-15 18:30:48');

-- ----------------------------
-- Table structure for `biz_tenant`
-- ----------------------------
DROP TABLE IF EXISTS `biz_tenant`;
CREATE TABLE `biz_tenant` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `code` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '租户编码',
  `name` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '租户名称',
  `status` tinyint(4) NOT NULL DEFAULT '1' COMMENT '0=禁用 1=启用',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_code` (`code`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='租户表';

-- ----------------------------
-- Records of biz_tenant
-- ----------------------------
INSERT INTO `biz_tenant` VALUES ('1', 'demo_tenant', 'Demo 租户', '1', '2026-06-14 19:48:48');
INSERT INTO `biz_tenant` VALUES ('2', 'acme_corp', 'Acme Corp', '1', '2026-06-14 19:48:48');
INSERT INTO `biz_tenant` VALUES ('3', 'globalsoft', 'GlobalSoft Inc', '1', '2026-06-14 19:48:48');
INSERT INTO `biz_tenant` VALUES ('4', 'zuhusu', 'zuhusu', '1', '2026-06-15 18:30:34');

-- ----------------------------
-- Table structure for `sys_admin`
-- ----------------------------
DROP TABLE IF EXISTS `sys_admin`;
CREATE TABLE `sys_admin` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `username` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '用户名（邮箱）',
  `password` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'BCrypt 加密密码',
  `role` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'admin' COMMENT '角色 admin/operator',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_username` (`username`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='管理员表';

-- ----------------------------
-- Records of sys_admin
-- ----------------------------
INSERT INTO `sys_admin` VALUES ('1', 'admin@example.com', '$2a$10$5PhfXCx4631JoVKLJgUmneOGIGJFd5ye/ExEUzcSEQs5nGH2cr5ai', 'admin', '2026-06-14 19:48:48', '2026-06-16 13:21:00');
INSERT INTO `sys_admin` VALUES ('2', 'usersu', '$2a$10$qgvdOOZUSgmPoM2t1vqpTOe5aGmUJOidVo4r9WzIUNWfkRC2ZuLie', 'admin', '2026-06-15 12:39:25', '2026-06-16 12:38:44');

-- ----------------------------
-- Table structure for `sys_audit_log`
-- ----------------------------
DROP TABLE IF EXISTS `sys_audit_log`;
CREATE TABLE `sys_audit_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `event_type` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '事件类型',
  `event_detail` varchar(512) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '事件描述',
  `operator` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '操作者',
  `operator_type` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '后台操作' COMMENT '操作者类型',
  `tags` varchar(1024) COLLATE utf8mb4_unicode_ci DEFAULT '' COMMENT '标签JSON',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_event_type` (`event_type`),
  KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB AUTO_INCREMENT=23 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='审计日志表';

-- ----------------------------
-- Records of sys_audit_log
-- ----------------------------
INSERT INTO `sys_audit_log` VALUES ('1', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-15 12:54:40');
INSERT INTO `sys_audit_log` VALUES ('2', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-15 18:00:54');
INSERT INTO `sys_audit_log` VALUES ('3', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-15 18:07:27');
INSERT INTO `sys_audit_log` VALUES ('4', 'KEY_CREATED', '为产品 \"Web端应用\" 生成授权 Key，分配给\"苏\"', 'admin', '后台操作', '{\"productId\":\"web\",\"keyCode\":\"N1S8-ELVH-SG7O-VKIM-ZSY7\"}', '2026-06-15 19:06:21');
INSERT INTO `sys_audit_log` VALUES ('5', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:35:27');
INSERT INTO `sys_audit_log` VALUES ('6', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:35:53');
INSERT INTO `sys_audit_log` VALUES ('7', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:39:02');
INSERT INTO `sys_audit_log` VALUES ('8', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:42:03');
INSERT INTO `sys_audit_log` VALUES ('9', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:44:31');
INSERT INTO `sys_audit_log` VALUES ('10', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:46:58');
INSERT INTO `sys_audit_log` VALUES ('11', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:47:18');
INSERT INTO `sys_audit_log` VALUES ('12', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:48:58');
INSERT INTO `sys_audit_log` VALUES ('13', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:51:14');
INSERT INTO `sys_audit_log` VALUES ('14', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:54:31');
INSERT INTO `sys_audit_log` VALUES ('15', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:55:08');
INSERT INTO `sys_audit_log` VALUES ('16', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:56:38');
INSERT INTO `sys_audit_log` VALUES ('17', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 12:59:19');
INSERT INTO `sys_audit_log` VALUES ('18', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 13:02:39');
INSERT INTO `sys_audit_log` VALUES ('19', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 13:08:43');
INSERT INTO `sys_audit_log` VALUES ('20', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 13:09:29');
INSERT INTO `sys_audit_log` VALUES ('21', 'ADMIN_LOGIN', '管理员登录: usersu', 'usersu', '后台操作', '', '2026-06-16 13:35:29');
INSERT INTO `sys_audit_log` VALUES ('22', 'ADMIN_LOGIN', '管理员登录: admin@example.com', 'admin@example.com', '后台操作', '', '2026-06-16 13:46:48');
