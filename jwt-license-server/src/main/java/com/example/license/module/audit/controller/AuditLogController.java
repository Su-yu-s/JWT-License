package com.example.license.module.audit.controller;

import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.example.license.common.Result;
import com.example.license.module.audit.entity.AuditLog;
import com.example.license.module.dashboard.service.DashboardService;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/audit-logs")
@RequiredArgsConstructor
public class AuditLogController {

    private final DashboardService dashboardService;

    /**
     * 审计日志列表（分页）
     */
    @GetMapping
    public Result<Page<AuditLog>> list(
            @RequestParam(defaultValue = "1") int page,
            @RequestParam(defaultValue = "20") int size,
            @RequestParam(required = false) String eventType,
            @RequestParam(required = false) String keyword) {
        Page<AuditLog> result = dashboardService.listAuditLogs(page, size, eventType, keyword);
        return Result.ok(result);
    }
}
