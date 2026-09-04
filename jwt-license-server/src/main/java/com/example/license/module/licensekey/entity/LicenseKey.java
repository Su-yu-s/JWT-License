package com.example.license.module.licensekey.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

@Data
@TableName("biz_license_key")
public class LicenseKey {

    @TableId(type = IdType.AUTO)
    private Long id;

    private String productId;

    private String productName;

    private String keyCode;

    private String keyShort;

    private String encryptedPlaintext;

    private String assignee;

    private Integer gripMinutes;

    private LocalDateTime expiresAt;

    private Integer status;

    private String createdBy;

    private LocalDateTime createdAt;
}
