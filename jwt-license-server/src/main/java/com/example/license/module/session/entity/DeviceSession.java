package com.example.license.module.session.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

@Data
@TableName("biz_device_session")
public class DeviceSession {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long licenseKeyId;

    private String productId;

    private String machineHash;

    private String sdkVersion;

    private String jwtToken;

    private Integer status;

    private LocalDateTime lastOnlineAt;

    private LocalDateTime jwtExpiresAt;

    private LocalDateTime revokedAt;

    private String revokedReason;

    private LocalDateTime createdAt;
}
