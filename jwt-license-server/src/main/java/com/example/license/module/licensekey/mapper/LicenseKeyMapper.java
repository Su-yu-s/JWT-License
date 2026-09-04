package com.example.license.module.licensekey.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.example.license.module.licensekey.entity.LicenseKey;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Update;

@Mapper
public interface LicenseKeyMapper extends BaseMapper<LicenseKey> {

    /**
     * 撤销指定 Key 的所有活跃会话
     */
    @Update("UPDATE biz_device_session SET status = 0, revoked_at = NOW(), revoked_reason = 'license_key_deleted' "
            + "WHERE license_key_id = #{licenseKeyId} AND status = 1")
    int revokeSessionsByKey(Long licenseKeyId);
}
