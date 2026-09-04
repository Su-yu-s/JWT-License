package com.example.license.module.product.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

@Data
@TableName("biz_product")
public class Product {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long tenantId;

    private String productId;

    private String name;

    private String rsaPrivateKey;

    private String rsaPublicKey;

    private Integer status;

    private LocalDateTime createdAt;
}
