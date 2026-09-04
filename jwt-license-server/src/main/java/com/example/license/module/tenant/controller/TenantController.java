package com.example.license.module.tenant.controller;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.example.license.common.BusinessException;
import com.example.license.common.Result;
import com.example.license.module.tenant.entity.Tenant;
import com.example.license.module.tenant.mapper.TenantMapper;
import jakarta.validation.constraints.NotBlank;
import lombok.Data;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;
import java.util.List;

@RestController
@RequestMapping("/api/tenants")
@RequiredArgsConstructor
public class TenantController {

    private final TenantMapper tenantMapper;

    /**
     * 租户列表（只返回启用状态的）
     */
    @GetMapping
    public Result<List<Tenant>> list() {
        LambdaQueryWrapper<Tenant> wrapper = new LambdaQueryWrapper<>();
        wrapper.eq(Tenant::getStatus, 1);
        wrapper.orderByAsc(Tenant::getId);
        return Result.ok(tenantMapper.selectList(wrapper));
    }

    /**
     * 创建租户
     */
    @PostMapping
    public Result<Tenant> create(@RequestBody TenantCreateDTO dto) {
        long count = tenantMapper.selectCount(
                new LambdaQueryWrapper<Tenant>().eq(Tenant::getCode, dto.getCode()));
        if (count > 0) {
            throw new BusinessException(400, "租户编码已存在: " + dto.getCode());
        }

        Tenant tenant = new Tenant();
        tenant.setCode(dto.getCode());
        tenant.setName(dto.getName() != null ? dto.getName() : dto.getCode());
        tenant.setStatus(1);
        tenant.setCreatedAt(LocalDateTime.now());
        tenantMapper.insert(tenant);
        return Result.ok(tenant);
    }

    @Data
    static class TenantCreateDTO {
        @NotBlank(message = "租户编码不能为空")
        private String code;
        private String name;
    }
}
