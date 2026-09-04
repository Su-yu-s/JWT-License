package com.example.license.module.audit.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

@Data
@TableName("sys_audit_log")
public class AuditLog {

    @TableId(type = IdType.AUTO)
    private Long id;

    private String eventType;

    private String eventDetail;

    private String operator;

    private String operatorType;

    private String tags;

    private LocalDateTime createdAt;
}
