package com.example.license.module.auth.controller;

import com.example.license.common.AuditEventType;
import com.example.license.common.Result;
import com.example.license.config.JwtConfig;
import com.example.license.module.auth.entity.Admin;
import com.example.license.module.auth.service.AuthService;
import com.example.license.module.audit.entity.AuditLog;
import com.example.license.module.audit.mapper.AuditLogMapper;
import com.example.license.util.AdminJwtUtil;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import lombok.Data;
import lombok.RequiredArgsConstructor;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;
import java.util.Map;

@RestController
@RequestMapping("/api/auth")
@RequiredArgsConstructor
@Validated
public class AuthController {

    private final AuthService authService;
    private final AuditLogMapper auditLogMapper;
    private final JwtConfig jwtConfig;

    /**
     * 验证当前登录状态（AuthInterceptor 已校验 token 有效性）
     * 用于前端判断是否需要跳转到登录页
     */
    @GetMapping("/me")
    public ResponseEntity<Result<Map<String, Object>>> me(jakarta.servlet.http.HttpServletRequest request) {
        String username = (String) request.getAttribute("username");
        if (username == null) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body(Result.fail(401, "未登录"));
        }
        Admin admin = authService.getByUsername(username);
        if (admin == null) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body(Result.fail(401, "用户不存在"));
        }
        return ResponseEntity.ok(Result.ok(Map.of(
                "username", admin.getUsername(),
                "role", admin.getRole()
        )));
    }

    @PostMapping("/login")
    public ResponseEntity<Result<Map<String, Object>>> login(@Valid @RequestBody LoginRequest req) {
        // 快速校验：空值提前拦截，避免无效数据库查询
        String username = req.getUsername() != null ? req.getUsername().trim() : "";
        String password = req.getPassword() != null ? req.getPassword() : "";

        if (username.isEmpty()) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST)
                    .body(Result.fail(400, "请输入用户名"));
        }
        if (password.isEmpty()) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST)
                    .body(Result.fail(400, "请输入密码"));
        }

        if (!authService.validate(username, password)) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body(Result.fail(401, "用户名或密码错误，请检查后重试"));
        }

        Admin admin = authService.getByUsername(username);
        String token = AdminJwtUtil.generateAdminToken(
                jwtConfig.getAdminSecretKey(),
                username,
                jwtConfig.getAdminExpiration()
        );

        // 记录登录审计日志
        AuditLog log = new AuditLog();
        log.setEventType(AuditEventType.ADMIN_LOGIN.name());
        log.setEventDetail("管理员登录: " + username);
        log.setOperator(username);
        log.setOperatorType("后台操作");
        log.setCreatedAt(LocalDateTime.now());
        auditLogMapper.insert(log);

        return ResponseEntity.ok(Result.ok(Map.of(
                "token", token,
                "username", admin.getUsername(),
                "role", admin.getRole()
        )));
    }

    @Data
    static class LoginRequest {
        @NotBlank(message = "用户名不能为空")
        private String username;
        @NotBlank(message = "密码不能为空")
        private String password;
    }
}
