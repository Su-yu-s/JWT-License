package com.example.license.module.licensekey.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class KeyGenerateDTO {

    @NotBlank(message = "产品 ID 不能为空")
    private String productId;

    private String assignee = "";

    private Integer gripMinutes = 1440;

    private String expiresAt;
}
