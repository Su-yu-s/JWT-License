package com.example.license.module.product.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class ProductCreateDTO {

    @NotBlank(message = "所属租户不能为空")
    private String tenantCode;

    @NotBlank(message = "产品名称不能为空")
    private String name;

    private String productId;
}
