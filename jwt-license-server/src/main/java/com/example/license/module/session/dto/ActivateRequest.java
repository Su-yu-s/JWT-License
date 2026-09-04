package com.example.license.module.session.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class ActivateRequest {

    @NotBlank(message = "授权 Key 不能为空")
    private String keyCode;

    @NotBlank(message = "机器码不能为空")
    private String machineHash;

    private String sdkVersion = "";
}
