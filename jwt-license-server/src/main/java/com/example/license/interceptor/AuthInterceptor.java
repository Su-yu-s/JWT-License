package com.example.license.interceptor;

import com.example.license.config.JwtConfig;
import com.example.license.common.Result;
import com.example.license.util.AdminJwtUtil;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import lombok.RequiredArgsConstructor;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

/**
 * 管理员接口鉴权拦截器
 */
@Component
@RequiredArgsConstructor
public class AuthInterceptor implements HandlerInterceptor {

    private final JwtConfig jwtConfig;
    private final ObjectMapper objectMapper;

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) throws Exception {
        String uri = request.getRequestURI();
        // SDK 接口和登录接口放行
        if (uri.startsWith("/api/sdk/") || uri.startsWith("/api/auth/login")) {
            return true;
        }

        String token = extractToken(request);
        if (token == null || AdminJwtUtil.isAdminTokenExpired(jwtConfig.getAdminSecretKey(), token)) {
            // Token 无效或已过期
            response.setStatus(HttpStatus.UNAUTHORIZED.value());
            response.setContentType(MediaType.APPLICATION_JSON_VALUE);
            response.setCharacterEncoding("UTF-8");
            objectMapper.writeValue(response.getOutputStream(),
                    Result.fail(401, "登录已过期，请重新登录"));
            return false;
        }

        String username = AdminJwtUtil.getUsername(jwtConfig.getAdminSecretKey(), token);
        if (username == null) {
            response.setStatus(HttpStatus.UNAUTHORIZED.value());
            response.setContentType(MediaType.APPLICATION_JSON_VALUE);
            response.setCharacterEncoding("UTF-8");
            objectMapper.writeValue(response.getOutputStream(),
                    Result.fail(401, "Token 无效"));
            return false;
        }

        request.setAttribute("username", username);
        return true;
    }

    private String extractToken(HttpServletRequest request) {
        String authHeader = request.getHeader("Authorization");
        if (authHeader != null && authHeader.startsWith("Bearer ")) {
            return authHeader.substring(7);
        }
        return request.getHeader("x-auth-token");
    }
}
