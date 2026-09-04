package com.example.license.module.dashboard.controller;

import com.example.license.common.Result;
import com.example.license.module.audit.entity.AuditLog;
import com.example.license.module.dashboard.entity.DashboardStats;
import com.example.license.module.dashboard.service.DashboardService;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/dashboard")
@RequiredArgsConstructor
public class DashboardController {

    private final DashboardService dashboardService;

    /**
     * 获取统计概览
     */
    @GetMapping("/stats")
    public Result<DashboardStats> stats() {
        return Result.ok(dashboardService.getStats());
    }

    /**
     * 最近审计日志
     */
    @GetMapping("/recent-audit")
    public Result<List<AuditLog>> recentAudit(
            @RequestParam(defaultValue = "10") int limit) {
        return Result.ok(dashboardService.recentAudit(limit));
    }
}
