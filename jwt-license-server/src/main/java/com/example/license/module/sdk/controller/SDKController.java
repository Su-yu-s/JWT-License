package com.example.license.module.sdk.controller;

import com.example.license.common.Result;
import com.example.license.module.session.dto.ActivateRequest;
import com.example.license.module.session.service.DeviceSessionService;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

/**
 * SDK 客户端接口
 * 不需要管理员 Token，独立鉴权
 * 提供完整的 SDK 生命周期管理：activate, heartbeat, onlineCheck, takeover, deactivate
 */
@RestController
@RequestMapping("/api/sdk")
@RequiredArgsConstructor
public class SDKController {

    private final DeviceSessionService deviceSessionService;

    /**
     * 设备激活 — 使用 Key + 机器码获取 JWT
     * 一对一限制：同一 Key 只能有一个活跃会话
     * 返回: token, sessionId, activeSeats, publicKeyPem, errorCode(OCCUPIED)
     */
    @PostMapping("/activate")
    public Result<Map<String, Object>> activate(@Valid @RequestBody ActivateRequest req) {
        Map<String, Object> data = deviceSessionService.activate(req);
        return Result.ok(data);
    }

    /**
     * 心跳保活 — 刷新会话，返回新的 JWT
     */
    @PostMapping("/heartbeat")
    public Result<Map<String, Object>> heartbeat(@RequestParam String keyCode) {
        Map<String, Object> data = deviceSessionService.heartbeat(keyCode);
        return Result.ok(data);
    }

    /**
     * 在线检查 — 仅更新最后在线时间，不刷新 JWT
     */
    @PostMapping("/online-check")
    public Result<Map<String, Object>> onlineCheck(@RequestParam String keyCode) {
        Map<String, Object> data = deviceSessionService.onlineCheck(keyCode);
        return Result.ok(data);
    }

    /**
     * 强踢 takeover — 当前设备主动踢走其他所有设备
     */
    @PostMapping("/takeover")
    public Result<Map<String, Object>> takeover(@Valid @RequestBody ActivateRequest req) {
        Map<String, Object> data = deviceSessionService.takeover(req);
        return Result.ok(data);
    }

    /**
     * 主动注销 — 客户端退出时调用
     */
    @PostMapping("/deactivate")
    public Result<Map<String, Object>> deactivate(@RequestParam String keyCode) {
        Map<String, Object> data = deviceSessionService.deactivate(keyCode);
        return Result.ok(data);
    }
}
