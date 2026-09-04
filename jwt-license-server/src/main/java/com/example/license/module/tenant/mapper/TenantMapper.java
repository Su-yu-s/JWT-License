package com.example.license.module.tenant.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.example.license.module.tenant.entity.Tenant;
import org.apache.ibatis.annotations.Mapper;

@Mapper
public interface TenantMapper extends BaseMapper<Tenant> {
}
