package com.example.license.module.session.controller;

import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.example.license.common.Result;
import com.example.license.module.session.entity.DeviceSession;
import com.example.license.module.session.service.DeviceSessionService;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@RequestMapping("/api/sessions")
@RequiredArgsConstructor
public class DeviceSessionController {

    private final DeviceSessionService deviceSessionService;

    /**
     * 会话列表（分页）
     */
    @GetMapping
    public Result<Page<DeviceSession>> list(
            @RequestParam(defaultValue = "1") int page,
            @RequestParam(defaultValue = "20") int size,
            @RequestParam(required = false) String productId) {
        Page<DeviceSession> result = deviceSessionService.listActiveSessions(page, size, productId);
        return Result.ok(result);
    }

    /**
     * 踢下线（撤销会话）
     */
    @PostMapping("/{id}/revoke")
    public Result<Void> revoke(@PathVariable Long id) {
        deviceSessionService.revoke(id);
        return Result.ok();
    }

    /**
     * 清理超时会话
     */
    @DeleteMapping("/expired")
    public Result<Map<String, Integer>> clearExpired() {
        int count = deviceSessionService.clearExpired();
        return Result.ok(Map.of("cleared", count));
    }
}
